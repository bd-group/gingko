/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-21 17:03:09 macan>
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

#ifndef __SU_H__
#define __SU_H__

#include "dfile.h"

struct su_meta_d
{
#define SU_META_VERSION         0x01
    u8 version;
#define GSU_META_RDONLY         0x01
#define GSU_META_RW             0x02
#define GSU_META_BAD            0x80
    u8 flags;
    u16 namelen;
    int dfnr;
    char name[0];
};

struct su_meta
{
    u8 version;
    u8 flags;
    int dfnr;
#define SU_NAME_LEN             32
    char *name;
};

struct su_conf
{
#define SU_PAGE_SIZE                    (128 * 1024)
#define SU_PAGE_SIZE_MAX                (64 * 1024 * 1024)
#define SU_FC_SIZE                      (256)
    int page_size;
    int page_algo;
    int fc_size;
};

struct gingko_su
{
    struct su_conf conf;
    char *path;
    
    struct dfile *files;
    struct su_meta sm;

    struct hlist_node hlist;    /* linked into hash table */

    long last_lid;

    atomic_t ref;

    /* schema tree from all dfiles */
    struct field_t *root;
    struct field_c *fc;
    
    /* region for files' fd */
    xlock_t lock;               /* lock to protect fd update */
    int smfd;
};

static inline void __gs_get(struct gingko_su *gs)
{
    atomic_inc(&gs->ref);
}

void __free_su(struct gingko_su *gs);

static inline void __gs_put(struct gingko_su *gs)
{
    if (atomic_dec_return(&gs->ref) < 0) {
        __free_su(gs);
    }
}

/* flags for alloc SU (__alloc_su) */
#define GSU_ALLOC_FREE_ONLY     0
#define GSU_ALLOC_HARD          1

#ifdef GINGKO_TRACING
extern u32 gingko_su_tracing_flags;
#endif

#endif
