/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-12 15:33:48 macan>
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

#ifndef __GINGKO_H__
#define __GINGKO_H__

#include "gingko_u.h"

#include "tracing.h"
#include "memory.h"
#include "xlock.h"
#include "gingko_common.h"
#include "gingko_const.h"
#include "xhash.h"

#include "field.h"
#include "line.h"
#include "index.h"
#include "page.h"
#include "dfile.h"
#include "su.h"
#include "iapi.h"

/* APIs */
#include "hash.c"

#define __UNUSED__ __attribute__((unused))

#define BITMAP_ROUNDUP(x) (((x + 1) + XTABLE_BITMAP_SIZE - 1) & \
                           ~(XTABLE_BITMAP_SIZE - 1))
#define BITMAP_ROUNDDOWN(x) ((x) & (~((XTABLE_BITMAP_SIZE) - 1)))

#define PAGE_ROUNDUP(x, p) ((((size_t) (x)) + (p) - 1) & ~((p) - 1))

/* export APIs */

#ifdef GINGKO_TRACING
extern u32 gingko_api_tracing_flags;
#endif

#define SU_OPEN_RDONLY          0
#define SU_OPEN_APPEND          1

/* Init and fina a gingko environment */
#define GINGKO_MAX_SUID                 102400
#define GINGKO_GSRH_SIZE                2048
struct gingko_conf
{
    int max_suid;
    int gsrh_size;
};

int gingko_init(struct gingko_conf *);
int gingko_fina(void);

/* Open a SU, return SUID */
int su_open(char *supath, int mode);

/* Get schema info from SU, should open SU firstly */
struct field *su_getschema(int suid);

/* Create a SU */
int su_create(char *supath, struct field schemas[], int schlen);

/* Write a line to SU */
int su_write(int suid, struct line* line, long lid);

/* Sync the memroy cached SU content to disk */
int su_sync(int suid);

/* Close the SU */
int su_close(int suid);

/* Read interfaces for Gingko */

struct range
{
    long begin, end;
};

struct op
{
    struct op *left, *right;
    u16 fld;
    void *args;
};

union ops
{
    struct op op;
    /* ... */
};

struct optree
{
    union ops *root;
};

int su_scan(char *supath, struct range *r, struct optree *opt);

int su_get(char *supath, long lid, char *fields[]);

int su_mget(char *supath, long lids[], char *fields[]);

/* Index exploiting */

struct sustat
{
    long lnr;                   /* line number */
};

int su_statsu(char *supath, struct sustat *r);

struct dfstat
{
    long pnr;                   /* page number */
};

int su_statdf(char *supath, int dfid, struct dfstat *r);

struct pgstat
{
    long lnr;                   /* line number in this page */
    int fldnr;                  /* number of fields */
    void *idx;                  /* index array */
};

int su_statpg(char *supath, int dfid, int pgid, struct pgstat *r);

#endif
