/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-26 23:41:16 macan>
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

#ifndef __INDEX_H__
#define __INDEX_H__

#include "line.h"

struct fld_u8_stat
{
    u16 fld;
    u8 max;
    u8 min;
    u64 count;
    u64 sum;
};

struct fld_u16_stat
{
    u16 fld;
    u16 max;
    u16 min;
    u64 count;
    u64 sum;
};

struct fld_u32_stat
{
    u16 fld;
    u32 max;
    u32 min;
    u64 count;
    u64 sum;
};

struct fld_u64_stat
{
    u16 fld;
    u64 max;
    u64 min;
    u64 count;
    u64 sum;
};

struct fld_double_stat
{
    u16 fld;
    double max;
    double min;
    u64 count;
    double sum;
};

struct fld_float_stat
{
    u16 fld;
    float max;
    float min;
    u64 count;
    double sum;
};

struct fld_array_stat
{
    u16 child_fld;
};

struct fld_struct_stat
{
    u16 fld;
};

struct fld_map_stat
{
    u16 fld;
};

/* this is the general fldstat type, other specific type can convert to this
 * type */
struct fldstat
{
    u16 fld;
};

struct pageindex
{
    u32 startline;
    u32 linenr;
    u32 fldstatlen;
    struct fldstat **stats;
    struct lineheader **lharray;
};

#ifdef GINGKO_TRACING
extern u32 gingko_index_tracing_flags;
#endif

#endif
