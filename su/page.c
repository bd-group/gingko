/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-22 21:40:18 macan>
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

static inline void __page_get(struct page *p)
{
    atomic_inc(&p->ref);
}

static inline void __page_put(struct page *p)
{
    if (atomic_dec_return(&p->ref) < 0) {
        __free_page(p);
    }
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

    p->pi = xzalloc(sizeof(*p->pi));
    if (!p->pi) {
        gingko_err(su, "xzalloc() PageIndex failed, no memory.\n");
        xfree(p);
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
        le[gs->files[dfid].dfh->l2p.ph.nr].pgoff = L2P_PGOFF_MAX;
        gs->files[dfid].dfh->l2p.ph.nr++;
    }

    p->ph.flag = gs->conf.page_algo;

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

void __free_page(struct page *p)
{
    if (!p)
        return;

    gingko_verbose(su, "Free PAGE %p\n", p);

    if (atomic_read(&p->ref) >= 0) {
        GINGKO_BUGON("Invalid PAGE reference.");
    }

    /* TODO: free any resource */
    if (!hlist_unhashed(&p->hlist)) {
        __pcrh_remove(p);
    }

    xfree(p);
}

void dump_page(struct page *p)
{
    gingko_info(su, "Page %p %s dfid %d ref %d ph.flag %d\n",
                p, p->suname, p->dfid,
                atomic_read(&p->ref),
                p->ph.flag);
    if (p->pi)
        dump_pageindex(p->pi);
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

    /* MUST at last: update page index */
    err = build_pageindex(p, line, lid, gs);
    if (err) {
        gingko_err(su, "build_pageindex failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out;
    }

    p->coff += line->len;

out:
    return err;
}

/* Sync a page to storage device, do compress if need, do pre-write
 * operations, do post-write operations after successfully written.
 *
 * Caution: this function must be called with write lock
 */
int page_sync(struct page *p, struct gingko_su *gs)
{
    int err = 0;

    return err;
}
