/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-17 16:37:33 macan>
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

int df_read_meta(struct gingko_su *gs, struct dfile *df)
{
    char path[4096];
    int br, bl = 0, err = 0, i;

    sprintf(path, "%s/%s", gs->path, SU_DFILE_FILENAME);
    df->fds[SU_META_ID] = open(path, O_RDONLY);
    if (df->fds[SU_META_ID] < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        return -errno;
    }

    /* read in the MD */
    bl = 0;
    do {
        br = read(df->fds[SU_META_ID], (void *)&df->dfh->md + bl, 
                  sizeof(df->dfh->md) - bl);
        if (br < 0) {
            gingko_err(su, "read %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out;
        } else if (br == 0) {
            gingko_err(su, "read DF META EOF\n");
            err = -EBADF;
            goto out;
        }
        bl += br;
    } while (bl < sizeof(df->dfh->md));

    /* read in the schemas */
    df->dfh->schemas = xzalloc(df->dfh->md.fldnr * sizeof(struct field));
    if (!df->dfh->schemas) {
        gingko_err(su, "xzalloc() schemas failed\n");
        err = -ENOMEM;
        goto out;
    }
    
    for (i = 0; i < df->dfh->md.fldnr; i++) {
        struct field_d fdd;
        char *name;
        
        bl = 0;
        do {
            br = read(df->fds[SU_META_ID], (void *)&fdd + bl, 
                      sizeof(fdd) - bl);
            if (br < 0) {
                gingko_err(su, "read FDD from %s failed w/ %s(%d)\n",
                           path, strerror(errno), errno);
                err = -errno;
                goto out_free;
            } else if (br == 0) {
                gingko_err(su, "read FDD EOF\n");
                err = -EBADF;
                goto out_free;
            }
            bl += br;
        } while (bl < sizeof(fdd));

        df->dfh->schemas[i].id = fdd.id;
        df->dfh->schemas[i].pid = fdd.pid;
        df->dfh->schemas[i].type = fdd.type;
        df->dfh->schemas[i].id = fdd.codec;

        name = xmalloc(fdd.namelen + 1);
        if (!name) {
            gingko_err(su, "xmalloc() name failed\n");
            err = -ENOMEM;
            goto out_free;
        }
        bl = 0;
        do {
            br = read(df->fds[SU_META_ID], name + bl, fdd.namelen - bl);
            if (br < 0) {
                gingko_err(su, "read field name from %s failed w/ %s(%d)\n",
                           path, strerror(errno), errno);
                err = -errno;
                goto out_free;
            } else if (br == 0) {
                gingko_err(su, "read field name EOF\n");
                err = -EBADF;
                goto out_free;
            }
            bl += br;
        } while (bl < fdd.namelen);

        df->dfh->schemas[i].name = name;
    }

    /* iterate and compute cidnr */
    {
        int cidnrs[df->dfh->md.fldnr];

        for (i = 0; i < df->dfh->md.fldnr; i++) {
            cidnrs[i] = 0;
        }
        for (i = 0; i < df->dfh->md.fldnr; i++) {
            if (df->dfh->schemas[i].pid != FLD_MAX_PID &&
                df->dfh->schemas[i].pid < df->dfh->md.fldnr) {
                cidnrs[df->dfh->schemas[i].pid]++;
            }
        }
        for (i = 0; i < df->dfh->md.fldnr; i++) {
            df->dfh->schemas[i].cidnr = cidnrs[i];
        }
    }

    gingko_debug(su, "Read dfh.md (v%d, st 0x%x, flag 0x%x, ca %d, flen %ld "
                 "lnr %ld fldnr %d l1fldnr %d\n",
                 df->dfh->md.version,
                 df->dfh->md.status,
                 df->dfh->md.flag,
                 df->dfh->md.comp_algo,
                 df->dfh->md.file_len,
                 df->dfh->md.lnr,
                 df->dfh->md.fldnr,
                 df->dfh->md.l1fldnr);

    gingko_debug(su, "Schemas: \n");

    for (i = 0; i < df->dfh->md.fldnr; i++) {
        gingko_debug(su, " F: %s id %d pid %d type %d codec %d, cidnr %d\n", 
                     df->dfh->schemas[i].name,
                     df->dfh->schemas[i].id,
                     df->dfh->schemas[i].pid,
                     df->dfh->schemas[i].type,
                     df->dfh->schemas[i].codec,
                     df->dfh->schemas[i].cidnr);
    }

out:
    return err;

out_free:
    for (i = 0; i < df->dfh->md.fldnr; i++) {
        xfree(df->dfh->schemas[i].name);
    }
    xfree(df->dfh->schemas);
    
    goto out;
}

/* Write df_header.meta and schemas
 */
int df_write_meta(struct gingko_su *gs, struct dfile *df, int write_schema)
{
    char path[4096];
    int err = 0, bw, bl;

    sprintf(path, "%s/%s", gs->path, SU_DFILE_FILENAME);
    df->fds[SU_DFILE_ID] = open(path, O_RDWR | O_CREAT, 
                                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (df->fds[SU_DFILE_ID] < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        err = -errno;
        goto out;
    }

    /* ok, write the meta to disk */
    bl = 0;
    do {
        bw = write(df->fds[SU_DFILE_ID], (void *)&df->dfh->md + bl, 
                   sizeof(df->dfh->md) - bl);
        if (bw < 0) {
            gingko_err(su, "write %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out;
        }
        bl += bw;
    } while (bl < sizeof(df->dfh->md));

    /* next, write schemas to disk */
    if (write_schema) {
        int i = 0;

        for (i = 0; i < df->dfh->md.fldnr; i++) {
            struct field_d fdd = {
                .id = df->dfh->schemas[i].id,
                .pid = df->dfh->schemas[i].pid,
                .type = df->dfh->schemas[i].type,
                .codec = df->dfh->schemas[i].codec,
                .namelen = strlen(df->dfh->schemas[i].name),
            };

            bl = 0;
            do {
                bw = write(df->fds[SU_DFILE_ID], (void *)&fdd + bl, 
                           sizeof(fdd) - bl);
                if (bw < 0) {
                    gingko_err(su, "write FDD to %s failed w/ %s(%d)\n",
                               path, strerror(errno), errno);
                    err = -errno;
                    goto out;
                }
                bl += bw;
            } while (bl < sizeof(fdd));

            bl = 0;
            do {
                bw = write(df->fds[SU_DFILE_ID], df->dfh->schemas[i].name + bl, 
                           fdd.namelen - bl);
                if (bw < 0) {
                    gingko_err(su, "write field name to %s failed w/ %s(%d)\n",
                               path, strerror(errno), errno);
                    err = -errno;
                    goto out;
                }
                bl += bw;
            } while (bl < fdd.namelen);
        }
    }

    gingko_debug(su, "Write df->dfh.md (v%d, st 0x%x, flag 0x%x, ca %d, flen %ld "
                 "lnr %ld fldnr %d l1fldnr %d\n",
                 df->dfh->md.version,
                 df->dfh->md.status,
                 df->dfh->md.flag,
                 df->dfh->md.comp_algo,
                 df->dfh->md.file_len,
                 df->dfh->md.lnr,
                 df->dfh->md.fldnr,
                 df->dfh->md.l1fldnr);

out:
    return err;
}

int df_write_l2p(struct gingko_su *gs, struct dfile *df)
{
    char path[4096];
    int err = 0, bw, bl;

    sprintf(path, "%s/%s", gs->path, SU_L2P_FILENAME);
    df->fds[SU_L2P_ID] = open(path, O_RDWR | O_CREAT, 
                              S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (df->fds[SU_L2P_ID] < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        err = -errno;
        goto out;
    }

    /* ok, write pi l2p header to disk */
    bl = 0;
    do {
        bw = write(df->fds[SU_L2P_ID], (void *)&df->dfh->l2p.ph + bl, 
                   sizeof(df->dfh->l2p.ph) - bl);
        if (bw < 0) {
            gingko_err(su, "write %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out;
        }
        bl += bw;
    } while (bl < sizeof(df->dfh->l2p.ph));

    /* l2p_entries should append to this file */
out:

    return err;
}

int df_append_l2p(struct gingko_su *gs, struct l2p_entry *e)
{
    return 0;
}

int alloc_dfile(struct dfile *df)
{
    int err = 0;
    
    df->dfh = xzalloc(sizeof(*df->dfh));
    if (!df->dfh) {
        gingko_err(api, "xzalloc DFH failed\n");
        err = -ENOMEM;
        goto out;
    }
    df->fds = xzalloc(SU_PER_DFILE_MAX * sizeof(int));
    if (!df->fds) {
        gingko_err(su, "allocate DF fds failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }

out:
    return err;
}

int init_dfile(struct gingko_su *gs, struct field schemas[], int schelen, 
               struct dfile *df)
{
    int err = 0, i, l1fldnr = 0;
    
    /* alloc and init a dfile header */
    df->dfh = xzalloc(sizeof(*df->dfh));
    if (!df->dfh) {
        gingko_err(su, "allocaate df_header failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    df->fds = xzalloc(SU_PER_DFILE_MAX * sizeof(int));
    if (!df->fds) {
        gingko_err(su, "allocate DF fds failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }

    for (i = 0; i < schelen; i++) {
        if (schemas[i].pid == FLD_MAX_PID) {
            l1fldnr++;
        }
    }
    
    df->dfh->md.version = DF_META_VERSION;
    df->dfh->md.status = DF_META_STATUS_INCREATE;
    df->dfh->md.comp_algo = DF_META_COMP_ALGO_NONE;
    df->dfh->md.fldnr = schelen;
    df->dfh->md.l1fldnr = l1fldnr;

    df->dfh->schemas = schemas;
    
    /* write the df_header.meta and schemas */
    err = df_write_meta(gs, df, 1);
    if (err) {
        gingko_err(su, "df_write_meta failed w/ %s(%d)\n",
                   strerror(-err), -err);
        goto out_free;
    }
    /* write the page l2p array */
    err = df_write_l2p(gs, df);
    if (err) {
        gingko_err(su, "df_write_l2p failed w/ %s(%d)\n",
                   strerror(-err), -err);
        goto out_free;
    }

    /* finally, update df_header.meta to DONE create! */
    df->dfh->md.status = DF_META_STATUS_INITED;
    err = df_write_meta(gs, df, 0);
    if (err) {
        gingko_err(su, "df_write_meta failed w/ %s(%d)\n",
                   strerror(-err), -err);
        goto out_free;
    }

out_free:
    xfree(df->dfh);

out:
    return err;
}

void fina_dfile(struct dfile *df)
{
    int j = 0;
    
    if (df->fds) {
        for (j = 0; j < SU_PER_DFILE_MAX; j++) {
            if (df->fds[j] > 0) {
                fsync(df->fds[j]);
                close(df->fds[j]);
            }
        }
    }
    xfree(df->dfh);
    xfree(df->fds);
}
