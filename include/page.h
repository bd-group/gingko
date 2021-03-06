/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-22 08:54:05 macan>
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

#ifndef __PAGE_H__
#define __PAGE_H__

#include "index.h"

struct pageheader 
{
#define SU_PH_ROW               0
#define SU_PH_COLUMN            1
    u8 mode;
#define SU_PH_COMP_NONE         0
#define SU_PH_COMP_SNAPPY       1
#define SU_PH_COMP_LZO          2
#define SU_PH_COMP_ZLIB         3
#define SU_PH_COMP_LZ4          4
    u8 flag;
#define SU_PH_CLEAN             0
#define SU_PH_DIRTY             1
#define SU_PH_WB                2
#define SU_PH_WBDONE            3
    u16 status;
    u32 orig_len;
    u32 zip_len;
    u32 crc32;

    u32 lhoff;

    /* dup info from index */
    u32 startline;
    u32 lnr;
};

#define IS_PAGE_FREEABLE(p) ({int __res = 0;                            \
            if ((p)->ph.status == SU_PH_CLEAN ||                        \
                (p)->ph.status == SU_PH_WBDONE) __res = 1; else __res = 0; \
            __res;})

struct page 
{
    char *suname;
    int dfid;
    
    atomic_t ref;

    struct hlist_node hlist;
    struct list_head list;
    struct pageheader ph;
    struct pageindex *pi;
    void *data;
#define SU_PG_MAX_PGOFF         0xffffffffffffffff
    u64 pgoff;                  /* offset in dfile, can by used as unique
                                 * identity in one dfile */

    xrwlock_t rwlock;           /* synchronize on read/write */
    u64 coff;                   /* current write offset */

    u32 pid;                    /* page id */
};

#endif
