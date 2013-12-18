/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-24 10:55:00 macan>
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

#ifndef __DFILE_H__
#define __DFILE_H__

#define SU_META_FILENAME        "meta"
#define SU_DFILE_FILENAME       "dfile"
#define SU_L2P_FILENAME        "l2p"

#define SU_META_ID                      0
#define SU_DFILE_ID                     1
#define SU_L2P_ID                       3
#define SU_PER_DFILE_MAX                8

struct df_meta 
{
#define DF_META_VERSION                 1
    u8 version;
#define DF_META_STATUS_INCREATE         0
#define DF_META_STATUS_INITED           1
#define DF_META_STATUS_RDONLY           2
    u8 status;
    u16 reserved1;
    u32 flag;
#define DF_META_COMP_ALGO_NONE          0x00
#define DF_META_COMP_ALGO_LZO           0x01
#define DF_META_COMP_ALGO_ZLIB          0x02
    u32 comp_algo;
    u64 file_len;
    u64 lnr;
    u16 fldnr;
    u16 l1fldnr;
};

struct l2p_header
{
    u32 nr;                     /* # of l2p entries*/
};

struct l2p_entry
{
#define L2P_LID_MAX             0xffffffffffffffff
#define L2P_PGOFF_MAX           0xffffffffffffffff
    u64 lid;
    u64 pgoff;
};

struct pageindex_l2p
{
    struct l2p_header ph;
    struct l2p_entry *l2pa;
};

struct df_header 
{
    struct df_meta md;
    struct field *schemas;      /* field array, # = md.fldnr */
    u32 pgnr;
    struct pageindex_l2p l2p;
    void *lid_lookup_tree;
    u32 *pg_addr;
};
    
struct dfile 
{
    struct df_header *dfh;

    xrwlock_t rwlock;           /* protect page allocation */

    /* region for files' fds */
    int *fds;                   /* SU_PER_DFILE_MAX */
};

struct index 
{
    struct df_header *dfh;
    struct pageindex *pgi;
    struct pageheader *pgh;
    struct lineheader *lh;
};

#endif
