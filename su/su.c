/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-24 17:55:47 macan>
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
u32 gingko_su_tracing_flags;
#endif

int su_read_meta(struct gingko_su *gs)
{
    char path[4096];
    struct su_meta_d smd;
    int br, bl = 0, err = 0, smfd;

    /* We know that at this moment, nobody can conflict with us. So, just set
     * smfd here! */
    sprintf(path, "%s/%s", gs->path, SU_META_FILENAME);
    smfd = open(path, O_RDONLY);
    if (smfd < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        return -errno;
    }

    bl = 0;
    do {
        br = read(smfd, (void *)&smd + bl, sizeof(smd) - bl);
        if (br < 0) {
            gingko_err(su, "read %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out;
        } else if (br == 0) {
            gingko_err(su, "read SU META EOF\n");
            err = -EBADF;
            goto out;
        }
        bl += br;
    } while (bl < sizeof(smd));
    
    gs->sm.version = smd.version;
    gs->sm.flags = smd.flags;
    gs->sm.dfnr = smd.dfnr;
    
    if (smd.namelen > 0) {
        gs->sm.name = xzalloc(smd.namelen + 1);
        if (!gs->sm.name) {
            gingko_err(su, "xzalloc name failed.\n");
            err = -ENOMEM;
            goto out;
        }
        
        bl = 0;
        do {
            br = read(smfd, gs->sm.name + bl, smd.namelen - bl);
            if (br < 0) {
                gingko_err(su, "read %s name failed w/ %s(%d)\n",
                           path, strerror(errno), errno);
                err = -errno;
                goto out;
            } else if (br == 0) {
                gingko_err(su, "read SM name EOF\n");
                err = -EBADF;
                goto out;
            }
            bl += br;
        } while (bl < smd.namelen);
    }
    
    gingko_debug(su, "Read su.meta (v %d, 0x%x, dfnr %d, %s)\n",
                 gs->sm.version,
                 gs->sm.flags,
                 gs->sm.dfnr,
                 gs->sm.name);

out:
    return err;
}

int su_write_meta(struct gingko_su *gs)
{
    char path[4096];
    struct su_meta_d *smd;
    int err = 0, len = 0, bw, bl;

    sprintf(path, "%s/%s", gs->path, SU_META_FILENAME);
    gs->smfd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (gs->smfd < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        err = -errno;
        goto out;
    }

    len = sizeof(gs->sm) + strlen(gs->sm.name);
    smd = xzalloc(len);
    if (!smd) {
        gingko_err(su, "xzalloc() failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    
    smd->version = gs->sm.version;
    smd->flags = gs->sm.flags;
    smd->dfnr = gs->sm.dfnr;
    smd->namelen = strlen(gs->sm.name);
    memcpy(smd->name, gs->sm.name, smd->namelen);

    /* ok, write the meta to disk */
    bl = 0;
    do {
        bw = write(gs->smfd, (void *)smd + bl, len - bl);
        if (bw < 0) {
            gingko_err(su, "write %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out;
        }
        bl += bw;
    } while (bl < len);

    gingko_debug(su, "Write su.meta (v %d, 0x%x, dfnr %d, %s)\n",
                 gs->sm.version,
                 gs->sm.flags,
                 gs->sm.dfnr,
                 gs->sm.name);

out:
    return err;
}

