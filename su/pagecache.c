/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-24 15:03:53 macan>
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

#include "gingko.h"

struct page_cache
{
    xlock_t lock;               /* lock to protect hash table */
    int pcsize;                 /* page cache hash table size */
    struct regular_hash *pcrh;
};

static struct page_cache gpc;

int pagecache_init(struct gingko_conf *conf)
{
    int err = 0, i;

    memset(&gpc, 0, sizeof(gpc));

    xlock_init(&gpc.lock);
    gpc.pcsize = conf->pcrh_size;
    gpc.pcrh = xzalloc(sizeof(*gpc.pcrh) * gpc.pcsize);
    if (!gpc.pcrh) {
        gingko_err(api, "xzalloc(%d * PCRH) failed\n", gpc.pcsize);
        err = -ENOMEM;
        goto out;
    }
    for (i = 0; i < gpc.pcsize; i++) {
        INIT_HLIST_HEAD(&gpc.pcrh[i].h);
        xlock_init(&gpc.pcrh[i].lock);
    }
    
out:
    return err;
}

static inline int __calc_pcrh_slot(char *suname, int dfid, u64 pgoff)
{
    char fstr[SU_NAME_LEN + 32];
    int len = sprintf(fstr, "%s%d%lx", suname, dfid, pgoff);
    
    return RSHash(fstr, len) % gpc.pcsize;
}

static inline void __page_get(struct page *p)
{
    atomic_inc(&p->ref);
}

void __free_page(struct page *p);

static inline void __page_put(struct page *p)
{
    if (atomic_dec_return(&p->ref) < 0) {
        __free_page(p);
    }
}

struct page *__pcrh_lookup(char *suname, int dfid, u64 pgoff)
{
    int idx = __calc_pcrh_slot(suname, dfid, pgoff);
    int found = 0;
    struct hlist_node *pos;
    struct regular_hash *rh;
    struct page *p;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(p, pos, &rh->h, hlist) {
        if (strncmp(p->suname, suname, SU_NAME_LEN) == 0 &&
            p->dfid == dfid && p->pgoff == pgoff) {
            /* ok, found it */
            found = 1;
            __page_get(p);
            break;
        }
    }
    xlock_unlock(&rh->lock);
    if (!found) {
        p = NULL;
    }

    return p;
}

/* Return value:
 *  0 => inserted
 *  1 => not inserted
 */
int __pcrh_insert(struct page *p)
{
    int idx = __calc_pcrh_slot(p->suname, p->dfid, p->pgoff);
    int found = 0;
    struct regular_hash *rh;
    struct hlist_node *pos;
    struct page *tpos;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(tpos, pos, &rh->h, hlist) {
        if (strncmp(tpos->suname, p->suname, SU_NAME_LEN) == 0 &&
            tpos->dfid == p->dfid && tpos->pgoff == p->pgoff) {
            /* this means we have found the same page in hash table, do NOT
             * insert it */
            found = 1;
            break;
        }
    }
    if (!found) {
        /* ok, insert it to hash table */
        hlist_add_head(&p->hlist, &rh->h);
    }
    xlock_unlock(&rh->lock);

    return found;
}

void __pcrh_remove(struct page *p)
{
    int idx = __calc_pcrh_slot(p->suname, p->dfid, p->pgoff);
    struct regular_hash *rh;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_del_init(&p->hlist);
    xlock_unlock(&rh->lock);
}

/* Get a page, alloc a new one if need
 */
struct page *get_page(struct gingko_su *gs, int dfid, u64 pgoff)
{
    struct page *p = ERR_PTR(-ENOENT);

    /* lookup in the page cache */
    p = __pcrh_lookup(gs->sm.name, dfid, pgoff);
    if (!p) {
        /* hoo, we need alloc a new page and init it */
        p = __alloc_page(gs, dfid);
        if (IS_ERR(p)) {
            gingko_err(su, "__alloc_page() failed w/ %s(%ld)\n",
                       gingko_strerror(PTR_ERR(p)), PTR_ERR(p));
            p = NULL;
            goto out;
        }
    }
    
    /* the page->ref has already inc-ed */
out:
    return p;
}

void put_page(struct page *p)
{
    __page_put(p);
}

