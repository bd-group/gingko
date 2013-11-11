/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-11 17:14:32 macan>
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

int df_read_meta(struct gingko_su *gs, struct df_header *dfh)
{
    char path[4096];
    int br, bl = 0, fd, err = 0, i;

    sprintf(path, "%s/%s", gs->path, SU_DFILE_FILENAME);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        return -errno;
    }

    /* read in the MD */
    bl = 0;
    do {
        br = read(fd, (void *)&dfh->md + bl, sizeof(dfh->md) - bl);
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
    } while (bl < sizeof(dfh->md));

    /* read in the schemas */
    dfh->schemas = xzalloc(dfh->md.fldnr * sizeof(struct field));
    if (!dfh->schemas) {
        gingko_err(su, "xzalloc() schemas failed\n");
        err = -ENOMEM;
        goto out;
    }
    
    for (i = 0; i < dfh->md.fldnr; i++) {
        struct field_d fdd;
        char *name;
        
        bl = 0;
        do {
            br = read(fd, (void *)&fdd + bl, sizeof(fdd) - bl);
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

        dfh->schemas[i].id = fdd.id;
        dfh->schemas[i].pid = fdd.pid;
        dfh->schemas[i].type = fdd.type;
        dfh->schemas[i].id = fdd.codec;

        name = xmalloc(fdd.namelen + 1);
        if (!name) {
            gingko_err(su, "xmalloc() name failed\n");
            err = -ENOMEM;
            goto out_free;
        }
        bl = 0;
        do {
            br = read(fd, name + bl, fdd.namelen - bl);
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

        dfh->schemas[i].name = name;
    }

    /* iterate and compute cidnr */
    {
        int cidnrs[dfh->md.fldnr];

        for (i = 0; i < dfh->md.fldnr; i++) {
            cidnrs[i] = 0;
        }
        for (i = 0; i < dfh->md.fldnr; i++) {
            if (dfh->schemas[i].pid != FLD_MAX_PID &&
                dfh->schemas[i].pid < dfh->md.fldnr) {
                cidnrs[dfh->schemas[i].pid]++;
            }
        }
        for (i = 0; i < dfh->md.fldnr; i++) {
            dfh->schemas[i].cidnr = cidnrs[i];
        }
    }

    gingko_debug(su, "Read dfh.md (v%d, st 0x%x, flag 0x%x, ca %d, flen %ld "
                 "lnr %ld fldnr %d l1fldnr %d\n",
                 dfh->md.version,
                 dfh->md.status,
                 dfh->md.flag,
                 dfh->md.comp_algo,
                 dfh->md.file_len,
                 dfh->md.lnr,
                 dfh->md.fldnr,
                 dfh->md.l1fldnr);

    gingko_debug(su, "Schemas: \n");

    for (i = 0; i < dfh->md.fldnr; i++) {
        gingko_debug(su, " F: %s id %d pid %d type %d codec %d, cidnr %d\n", 
                     dfh->schemas[i].name,
                     dfh->schemas[i].id,
                     dfh->schemas[i].pid,
                     dfh->schemas[i].type,
                     dfh->schemas[i].codec,
                     dfh->schemas[i].cidnr);
    }

out:
    close(fd);

    return err;
out_free:
    for (i = 0; i < dfh->md.fldnr; i++) {
        xfree(dfh->schemas[i].name);
    }
    xfree(dfh->schemas);
    
    goto out;
}

/* Write df_header.meta and schemas
 */
int df_write_meta(struct gingko_su *gs, struct df_header *dfh)
{
    char path[4096];
    int err = 0, fd, bw, bl;

    sprintf(path, "%s/%s", gs->path, SU_DFILE_FILENAME);
    fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (fd < 0) {
        gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                   path, strerror(errno), errno);
        err = -errno;
        goto out;
    }

    /* ok, write the meta to disk */
    bl = 0;
    do {
        bw = write(fd, (void *)&dfh->md + bl, sizeof(dfh->md) - bl);
        if (bw < 0) {
            gingko_err(su, "write %s failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out_close;
        }
        bl += bw;
    } while (bl < sizeof(dfh->md));

    /* next, write schemas to disk */
    {
        int i = 0;

        for (i = 0; i < dfh->md.fldnr; i++) {
            struct field_d fdd = {
                .id = dfh->schemas[i].id,
                .pid = dfh->schemas[i].pid,
                .type = dfh->schemas[i].type,
                .codec = dfh->schemas[i].codec,
                .namelen = strlen(dfh->schemas[i].name),
            };

            bl = 0;
            do {
                bw = write(fd, (void *)&fdd + bl, sizeof(fdd) - bl);
                if (bw < 0) {
                    gingko_err(su, "write FDD to %s failed w/ %s(%d)\n",
                               path, strerror(errno), errno);
                    err = -errno;
                    goto out_close;
                }
                bl += bw;
            } while (bl < sizeof(fdd));

            bl = 0;
            do {
                bw = write(fd, dfh->schemas[i].name + bl, fdd.namelen - bl);
                if (bw < 0) {
                    gingko_err(su, "write field name to %s failed w/ %s(%d)\n",
                               path, strerror(errno), errno);
                    err = -errno;
                    goto out_close;
                }
                bl += bw;
            } while (bl < fdd.namelen);
        }
    }

    gingko_debug(su, "Write dfh.md (v%d, st 0x%x, flag 0x%x, ca %d, flen %ld "
                 "lnr %ld fldnr %d l1fldnr %d\n",
                 dfh->md.version,
                 dfh->md.status,
                 dfh->md.flag,
                 dfh->md.comp_algo,
                 dfh->md.file_len,
                 dfh->md.lnr,
                 dfh->md.fldnr,
                 dfh->md.l1fldnr);

out_close:
    close(fd);
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

    for (i = 0; i < schelen; i++) {
        if (schemas[i].pid == -1) {
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
    err = df_write_meta(gs, df->dfh);
    if (err) {
        gingko_err(su, "df_write_meta failed w/ %s(%d)\n",
                   strerror(-err), -err);
        goto out_free;
    }
    /* write the page skip */

out_free:
    xfree(df->dfh);

out:
    return err;
}

