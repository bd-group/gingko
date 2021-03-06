/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-21 17:07:34 macan>
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

/* Define Type Const */
#define GINGKO_INT8             1
#define GINGKO_INT16            2
#define GINGKO_INT32            3
#define GINGKO_INT64            4
#define GINGKO_FLOAT            5
#define GINGKO_DOUBLE           6
#define GINGKO_STRING           7
#define GINGKO_BYTES            8
#define GINGKO_ARRAY            9
#define GINGKO_STRUCT           10
#define GINGKO_MAP              11

/* Define Length Type */
#define SU_TYPE_L1B             0
#define SU_TYPE_L2B             1
#define SU_TYPE_L3B             2
#define SU_TYPE_L4B             3

#define SU_LEN_L1B_MAX          64
#define SU_LEN_L2B_MAX          8192
#define SU_LEN_L3B_MAX          (4 * 1024 * 1024)
#define SU_LEN_L4B_MAX          (1024 * 1024 * 1024)

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

union string_d
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

/* len: record the # of array's elements
 */
union array_d
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

/* len: record the # of map's KV pairs
 */
union map_d
{
    struct l1b l1;
    struct l2b l2;
    struct l3b l3;
    struct l4b l4;
};

union struct_d
{
    u8 nr; /* record the # of L1 fields */
    u8 data[0];
};

typedef struct __string_t
{
    u32 len;
    char *str;
} string_t;

struct field
{
    char *name;
    u16 id;
#define FLD_MAX_PID             0xffff
    u16 pid;                    /* INVALID: MAX_PID */
    u8 type;
#define FLD_CODEC_NONE          0x00
#define FLD_CODEC_RUNLENGTH     0x01
#define FLD_CODEC_DELTA         0x02
#define FLD_CODEC_DICT          0x03
    u8 codec;
    int cidnr;                  /* # of child fields */
};

struct field_2pack
{
    u8 type;
    int cidnr;                  /* # of appended child fields */
    void *data;
    int dlen;
    struct field_2pack **clds;
};

/* for storage use */
struct field_d 
{
    u16 id;
    u16 pid;
    u8 type;
    u8 codec;
    u8 namelen;
    char name[0];
};

/* for tree use */
struct field_t
{
#define FIELD_TYPE_ROOT         0
#define FIELD_TYPE_INTER        1
#define FIELD_TYPE_LEAF         2
    int type;
    int cnr;                    /* child number */
    struct field fld;
    struct field_t **cld;
    void *ptr;                  /* private data */
};

/* for query use */
struct field_g
{
    u16 id;                     /* query can by on ID */
    u16 orig_id;
    u16 pid;
    u8 type;
#define UNPACK_ALL              0x00
#define UNPACK_DATAONLY         0x01
#define UNPACK_FNAME            0x02
    u8 flags;

    char *name;                 /* query can by on NAME */
    void *content;
    int dlen;
};

/* for field cache use */
struct field_c
{
    struct field_t *ft;
};

#endif
