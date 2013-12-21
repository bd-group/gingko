/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-21 20:53:41 macan>
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

static inline int __calc_pcrh_slot(char *suname, int dfid)
{
    char fstr[SU_NAME_LEN + 32];
    int len = sprintf(fstr, "%s%d", suname, dfid);
    
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

struct page *__pcrh_lookup(char *suname, int dfid)
{
    int idx = __calc_pcrh_slot(suname, dfid);
    int found = 0;
    struct hlist_node *pos;
    struct regular_hash *rh;
    struct page *p;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(p, pos, &rh->h, hlist) {
        if (strncmp(p->suname, suname, SU_NAME_LEN) == 0 &&
            p->dfid == dfid) {
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
    int idx = __calc_pcrh_slot(p->suname, p->dfid);
    int found = 0;
    struct regular_hash *rh;
    struct hlist_node *pos;
    struct page *tpos;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(tpos, pos, &rh->h, hlist) {
        if (strncmp(tpos->suname, p->suname, SU_NAME_LEN) == 0 &&
            tpos->dfid == p->dfid) {
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
    int idx = __calc_pcrh_slot(p->suname, p->dfid);
    struct regular_hash *rh;

    rh = gpc.pcrh + idx;
    xlock_lock(&rh->lock);
    hlist_del_init(&p->hlist);
    xlock_unlock(&rh->lock);
}

struct page *__alloc_page(struct gingko_su *gs, int dfid)
{
    struct page *p;
    int err = 0;

    p = xzalloc(sizeof(*p));
    if (!p) {
        gingko_err(su, "xzalloc() PAGE failed, no memory.\n");
        return ERR_PTR(-ENOMEM);
    }

    /* init page struct */
    p->suname = gs->sm.name;
    p->dfid = dfid;
    INIT_HLIST_NODE(&p->hlist);
    xrwlock_init(&p->rwlock);
    
    /* TODO: init the page meta data */
    {
        /* init df_header's l2p array */
        struct l2p_entry *le;

        le = xrealloc(gs->files[dfid].dfh->l2p.l2pa, sizeof(*le) *
                      (gs->files[dfid].dfh->l2p.ph.nr + 1));
        if (!le) {
            gingko_err(su, "xrealloc l2p array entry failed, no memory.\n");
            p = ERR_PTR(-ENOMEM);
            goto out;
        }
        gs->files[dfid].dfh->l2p.l2pa = le;
        le[gs->files[dfid].dfh->l2p.ph.nr].lid = L2P_LID_MAX;
        le[gs->files[dfid].dfh->l2p.ph.nr].lid = L2P_PGOFF_MAX;
        gs->files[dfid].dfh->l2p.ph.nr++;
    }

    /* insert it to page cache */
    __page_get(p);
    
retry:
    switch (__pcrh_insert(p)) {
    case 0:
        /* inserted, it is ok */
        break;
    case 1:
    {
        /* this means someone has already allocated this page, return that
         * page */
        struct page *other;

        other = __pcrh_lookup(p->suname, p->dfid);
        if (!other) {
            goto retry;
        }
        __page_put(p);
        __page_put(p);          /* double put as free */
        p = other;
        break;
    }
    default:
        err = -EINTERNAL;
        __page_put(p);
        goto out_free;
    }

out:
    return p;
out_free:
    __page_put(p);
    p = ERR_PTR(err);
    goto out;
}

/* Get a page, alloc a new one if need
 */
struct page *get_page(struct gingko_su *gs, int dfid)
{
    struct page *p = ERR_PTR(-ENOENT);

    /* lookup in the page cache */
    p = __pcrh_lookup(gs->sm.name, dfid);
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

void __free_page(struct page *p)
{
    if (!p)
        return;

    gingko_verbose(su, "Free PAGE %p\n", p);

    if (atomic_read(&p->ref) >= 0) {
        GINGKO_BUGON("Invalid PAGE reference.");
    }

    /* TODO: free any resource */
}

/* Write a line to page, just append the data to page's data region, only
 * caching in memory. When user calls sync, we do page flush.
 *
 * Caution: this function must be called with write lock
 */
int page_write(struct page *p, struct line *line, long lid, 
               struct gingko_su *gs)
{
    int err = 0;

    if (!p->data) {
        void *x = xzalloc(gs->conf.page_size);
        if (!x) {
            gingko_err(su, "xzalloc() PAGE data region %dB failed, "
                       "no memory.\n", gs->conf.page_size);
            err = -ENOMEM;
            goto out;
        }
        p->data = x;
    }

    /* copy line data to page data, record current offset */
    memcpy(p->data + p->coff, line->data, line->len);

    /* build lineheader */
    err = build_lineheaders(gs, line, lid, p->coff);
    if (err) {
        gingko_err(su, "build_lineheader failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out;
    }

    p->coff += line->len;

out:
    return err;
}

