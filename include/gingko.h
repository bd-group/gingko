/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-17 18:37:19 macan>
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

#define __UNUSED__ __attribute__((unused))

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
#define GINGKO_PCRH_SIZE                (1024 * 1024) /* 64GB? */
#define GINGKO_ASYNC_PSYNC_TNR          4
#define GINGKO_PC_MEMORY                (2 * 1024 * 1024 * 1024L)
struct gingko_conf
{
    int max_suid;
    int gsrh_size;
    int pcrh_size;
    int async_page_sync_thread_nr;

    int pc_memory;
};

int gingko_init(struct gingko_conf *);
int gingko_fina(void);

/* Open a SU, return SUID */
int su_open(char *supath, int mode, void *arg);

/* Get schema info from SU, should open SU firstly */
struct field *su_getschema(int suid);

/* Create a SU */
int su_create(struct su_conf *sc, char *supath, struct field schemas[], 
              int schlen);

/* Build a line */
int su_linepack(struct line *line, struct field_2pack *flds[], int l1fldnr);

/* Build a field */
int su_fieldpack(struct field_2pack *pfld, struct field_2pack *cfld);

/* Build a l1field array */
struct field_2pack **su_l1fieldpack(struct field_2pack **fld, int *fldnr,
                                    struct field_2pack *new);

/* New a field */
struct field_2pack *su_new_field(u8 type, void *data, int dlen);

/* Free fields' resources */
void su_free_field_2pack(struct field_2pack **fld, int fldnr);

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

int su_get(int suid, long lid, struct field_g fields[], int fldnr);

int su_mget(int suid, long lids[], char *fields[]);

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

char *gingko_type(int type);

#endif
