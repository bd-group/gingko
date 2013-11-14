/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-11 16:57:25 macan>
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

struct df_meta 
{
#define DF_META_VERSION                 1
    u8 version;
#define DF_META_STATUS_INCREATE         0
#define DF_META_STATUS_RDONLY           1
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

struct pageindex_skiplist
{
    u32 nr;                     /* # of base offsets */
    u32 skiplines;              /* 0 or >0 */
    u64 *boff;                  /* base off array, # = nr */
};

struct df_header 
{
    struct df_meta md;
    struct field *schemas;      /* field array, # = md.fldnr */
    u32 pgnr;
    struct pageindex_skiplist pilist;
    u32 *pg_addr;
};
    
struct dfile 
{
    struct df_header *dfh;
};

struct index 
{
    struct df_header *dfh;
    struct pageindex *pgi;
    struct pageheader *pgh;
    struct lineheader *lh;
};

#endif
