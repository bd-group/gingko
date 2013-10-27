/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-10-27 21:30:38 macan>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "gingko.h"

#ifdef GINGKO_TRACING
static u32 gingko_api_tracing_flags;
#endif

#define GINGKO_MAX_SUID         102400
struct gingko_conf
{
    int max_suid;
};

static struct gingko_conf g_conf = {
    .max_suid = GINGKO_MAX_SUID,
};

struct gingko_manager
{
    struct gingko_conf *gc;

    struct gingko_su *gsu;
    xlock_t gsu_lock;
    int gsu_used;

};

static struct gingko_manager g_mgr = {
    .gc = &g_conf,
    .gsu = NULL,
    .gsu_used = 0,
};

int gingko_init(struct gingko_conf *conf)
{
    struct gingko_su *su;
    int err = 0;

    if (conf == NULL)
        conf = &g_conf;

    g_mgr.gc = conf;

    su = xzalloc(sizeof(*su) * conf->max_suid);
    if (!su) {
        gingko_err(api, "xzalloc(%d * gingko_su) failed", conf->max_suid);
        err = -ENOMEM;
        goto out;
    }
    xlock_init(&g_mgr.gsu_lock);
    
out:
    return err;
}

struct gingko_su *__alloc_su(struct gingko_manager *gm, int flag)
{
    struct gingko_su *gs = NULL;
    int i, found = 0;
    
    if (!gm)
        return NULL;

    xlock_lock(&gm->gsu_lock);
    for (i = 0; i < gm->gc->max_suid; i++) {
        gs = gm->gsu + i;
        if (gs->state == GSU_FREE ||
            (gs->state == GSU_CLOSE && flag == GSU_ALLOC_HARD)) {
            /* ok, this entry is good to use */
            found = 1;
            gs->state = GSU_INIT;
            break;
        }
    }
    xlock_unlock(&gm->gsu_lock);

    if (found) {
        memset((void *)gs + sizeof(gs->state), 0,
               sizeof(*gs) - sizeof(gs->state));
    }

    return gs;
}

static int __calc_suid(struct gingko_manager *gm, struct gingko_su *gs)
{
    if (!gm || !gs)
        return -1;

    return ((void *)gs - (void *)gm->gsu) / sizeof(*gs);
}

void __free_su(struct gingko_su *gs)
{
    if (!gs)
        return;

    /* TODO:free any resource, then set GSU_FREE flag */
    xfree(gs->sm.name);

    gs->state = GSU_FREE;
}

/* Open a SU, return SUID (>=0)
 * On error, return (<0)
 */
int su_open(char *supath, int mode) 
{
    char path[4096];
    struct gingko_su *gs = NULL;
    struct stat st;
    int suid, err = 0, fd;

    /* Step 1: find and allocate the gingko_su */
    gs = __alloc_su(&g_mgr, GSU_ALLOC_FREE_ONLY);
    if (!gs) {
        gingko_err(api, "allocate SU (FREE_ONLY) failed.\n");
        err = -ENOMEM;
        goto out;
    }
    suid = __calc_suid(&g_mgr, gs);
    gingko_debug(api, "Allocate SU %p idx %d\n", gs, suid);

    /* Step 2: check if the path exists and contains content */
    err = stat(supath, &st);
    if (err) {
        gingko_err(api, "stat(%s) failed w/ %s(%d)\n",
                   supath, strerror(errno), errno);
        err = -errno;
        goto out_free;
    }
    if (!S_ISDIR(st.st_mode)) {
        gingko_err(api, "Path '%s' is not a directory.\n",
                   supath);
        err = -ENOTDIR;
        goto out_free;
    }

    /* Step 3: read the su.meta file */
    sprintf(path, "%s/su.meta", supath);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        gingko_err(api, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        err = -errno;
        goto out_free;
    }

    {
        struct su_meta_d smd;
        int br, bl = 0;

        do {
            br = read(fd, (void *)&smd + bl, sizeof(smd) - bl);
            if (br < 0) {
                gingko_err(api, "read %s failed w/ %s(%d)\n",
                           path, strerror(errno), errno);
                err = -errno;
                goto out_free;
            }
            bl += br;
        } while (bl < sizeof(smd));
        
        gs->sm.version = smd.version;
        gs->sm.flags = smd.flags;
        gs->sm.dfnr = smd.dfnr;

        if (smd.namelen > 0) {
            gs->sm.name = xzalloc(smd.namelen + 1);
            if (!gs->sm.name) {
                gingko_err(api, "xzalloc name failed.\n");
                err = -ENOMEM;
                goto out_free;
            }
            
            bl = 0;
            do {
                br = read(fd, gs->sm.name + bl, smd.namelen - bl);
                if (br < 0) {
                    gingko_err(api, "read %s name failed w/ %s(%d)\n",
                               path, strerror(errno), errno);
                    err = -errno;
                    goto out_free;
                }
                bl += br;
            } while (bl < smd.namelen);
        }
        
        gingko_debug(api, "Read su.meta (v %d, 0x%x, dfnr %d, %s)\n",
                     gs->sm.version,
                     gs->sm.flags,
                     gs->sm.dfnr,
                     gs->sm.name);
    }
    
    switch (mode) {
    default:
    case SU_OPEN_RDONLY:
        if (gs->sm.flags & GSU_META_BAD) {
            gingko_debug(api, "Reject to open BAD SU '%s'.\n", supath);
            goto out_free;
        }
        break;
    case SU_OPEN_APPEND:
        if (gs->sm.flags & GSU_META_BAD ||
            gs->sm.flags & GSU_META_RDONLY) {
            gingko_debug(api, "Reject to open BAD or RDONLY SU '%s'.\n",
                         supath);
            goto out_free;
        }
        break;
    }

    /* ok, we have build the GS, return the suid now */
    err = suid;
    
out:
    return err;

out_free:
    __free_su(gs);
    return err;
}

/* Get schema info from SU (dfile), should open SU firstly
 */
struct field *su_getschema(int suid)
{
    return NULL;
}

/* Create a SU
 */
int su_create(char *supath, struct field schemas[], int schelen)
{
    int err = 0;

    return err;
}
