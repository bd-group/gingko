/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-11 17:15:38 macan>
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
u32 gingko_api_tracing_flags;
#endif

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
    int err = 0;

    if (conf == NULL)
        conf = &g_conf;

    g_mgr.gc = conf;

    g_mgr.gsu = xzalloc(sizeof(*g_mgr.gsu) * conf->max_suid);
    if (!g_mgr.gsu) {
        gingko_err(api, "xzalloc(%d * gingko_su) failed", conf->max_suid);
        err = -ENOMEM;
        goto out;
    }
    xlock_init(&g_mgr.gsu_lock);
    
out:
    return err;
}

int gingko_fina(void)
{
    int err = 0;
    
    /* free all resources */

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
    xfree(gs->path);
    xfree(gs->files);
    /*xfree(gs->sm.name);*/

    gs->state = GSU_FREE;
}

/* Open a SU, return SUID (>=0)
 * On error, return (<0)
 */
int su_open(char *supath, int mode) 
{
    struct gingko_su *gs = NULL;
    struct df_header *dfh;
    struct stat st;
    int suid, err = 0, i;

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
    gs->path = strdup(supath);
    err = su_read_meta(gs);
    if (err) {
        goto out_free;
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

    /* Step 4: read the dfiles */
    gs->files = xzalloc(sizeof(struct dfile) * gs->sm.dfnr);
    if (!gs->files) {
        gingko_err(api, "xzalloc DFILEs failed\n");
        err = -ENOMEM;
        goto out_free;
    }
    
    for (i = 0; i < gs->sm.dfnr; i++) {
        dfh = xzalloc(sizeof(*dfh));
        if (!dfh) {
            gingko_err(api, "xzalloc DFH failed\n");
            err = -ENOMEM;
            goto out_free;
        }
        err = df_read_meta(gs, dfh);
        if (err) {
            gingko_err(api, "df_read_meta failed w/ %s(%d)\n",
                       strerror(-err), err);
            goto out_free;
        }
        gs->files[i].dfh = dfh;
    }

    /* ok, we have build the GS, return the suid now */
    err = suid;
    
out:
    return err;

out_free:
    if (gs->files) {
        for (i = 0; i < gs->sm.dfnr; i++) {
            xfree(gs->files[i].dfh);
        }
    }
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
 *
 * Args: parent(supath) should exist, and supath should not exist
 */
int su_create(char *supath, struct field schemas[], int schelen)
{
    struct stat st;
    struct gingko_su *gs;
    int err = 0, suid, i;

    err = stat(supath, &st);
    if (err) {
        if (errno == ENOENT) {
            /* it is ok to create it */
            err = mkdir(supath, 0775);
            if (err) {
                gingko_err(api, "mdir(%s) failed w/ %s(%d)\n",
                           supath, strerror(errno), errno);
                err = -errno;
                goto out;
            }
        } else {
            gingko_err(api, "stat(%s) failed w/ %s(%d)\n",
                       supath, strerror(errno), errno);
            err = -errno;
            goto out;
        }
    } else {
        /* there exists a SU in supath */
        gingko_err(api, "SU exists in your path '%s', remove it if you "
                   "truely want to create at there.\n", supath);
        err = -EEXIST;
        goto out;
    }

    gs = __alloc_su(&g_mgr, GSU_ALLOC_FREE_ONLY);
    if (!gs) {
        gingko_err(api, "allocate SU (FREE_ONLY) faield, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    gs->path = strdup(supath);
    if (!gs->path) {
        gingko_err(api, "allocate supath failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    suid = __calc_suid(&g_mgr, gs);
    gingko_debug(api, "Allocate SU %p idx %d\n", gs, suid);
    
    /* Step 1: write the su.meta */
    gs->sm.version = SU_META_VERSION;
    gs->sm.flags = GSU_META_RW;
    gs->sm.dfnr = 1;
    gs->sm.name = "default_su";
    
    err = su_write_meta(gs);
    if (err) {
        gingko_err(api, "su_write_meta() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        goto out_free;
    }

    /* Step 2: parse field schema to generate a schema tree */
    err = verify_schema(schemas, schelen);
    if (err) {
        gingko_err(api, "verify_schema() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        goto out_free;
    }

    /* Step 3: new a dfile */
    gs->files = xzalloc(sizeof(struct dfile) * gs->sm.dfnr);
    if (!gs->files) {
        gingko_err(api, "xzalloc() dfile failed, no memory.\n");
        err = -ENOMEM;
        goto out_free;
    }

    for (i = 0; i < gs->sm.dfnr; i++) {
        err = init_dfile(gs, schemas, schelen, gs->files + i);
        if (err) {
            gingko_err(api, "init_dfile() failed w/ %s(%d)\n",
                       strerror(-err), err);
            goto out_free;
        }
    }
    
out:
    return err;
out_free:
    __free_su(gs);
    return err;
}

/* Write a line to SU
 */
int su_write(int suid, struct line *line, long lid)
{
    int err = 0;
    
    return err;
}

/* Sync the memroy cached SU content to disk
 */
int su_sync(int suid)
{
    int err = 0;
    
    return err;
}

/* Close the SU
 */
int su_close(int suid)
{
    int err = 0;

    return err;
}
