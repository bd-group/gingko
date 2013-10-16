/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-09-22 21:03:00 macan>
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

#ifndef __FIELD_H__
#define __FIELD_H__

#define SU_TYPE_STR_L1B         0
#define SU_TYPE_STR_L2B         1
#define SU_TYPE_STR_L3B         2
#define SU_TYPE_STR_L4B         3

struct l1b 
{
    u8 flag:2;
    u8 len:6;
    u8 data[0];
};

struct l2b
{
    u16 flag:2;
    u16 len:14;
    u8 data[0];
};

struct l3b
{
    u16 flag:2;
    u16 len:14;
    u8 lenhb;
    u8 data[0];
};

struct l4b
{
    u32 flag:2;
    u32 len:30;
    u8 data[0];
};

union string_t 
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

/* len: record the # of array's elements
 */
union array_t
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

/* len: record the # of map's KV pairs
 */
union map_t 
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

union struct_t 
{
    u8 nr; /* record the # of L1 fields */
    u8 data[0];
};

#endif
