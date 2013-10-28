/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-10-27 08:46:19 macan>
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
#define SU_PH_COMP_NONE         0
#define SU_PH_COMP_SNAPPY       1
#define SU_PH_COMP_LZO          2
#define SU_PH_COMP_ZLIB         3
    u32 flag;
    u32 orig_len;
    u32 zip_len;
    u32 crc32;
};

struct page 
{
    struct pageheader ph;
    struct pageindex *pi;
    void *data;
};

#endif
