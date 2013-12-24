/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-24 18:38:44 macan>
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
#include "lib.h"

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
    p->pgoff = SU_PG_MAX_PGOFF;
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

        other = __pcrh_lookup(p->suname, p->dfid, p->pgoff);
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

    /* check if this page can by written */
    switch (p->ph.status) {
    case SU_PH_CLEAN:
    case SU_PH_DIRTY:
        /* it is ok to write now */
        break;
    case SU_PH_WB:
    case SU_PH_WBDONE:
    default:
        return -EAGAIN;
        break;
    }
    
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
    p->ph.status = SU_PH_DIRTY;

out:
    return err;
}

int __page_comp_snappy(struct page *p, struct gingko_su *gs, void *zdata)
{
    int err = 0;

    return err;
}

int __page_comp_lzo(struct page *p, struct gingko_su *gs, void *zdata)
{
    void *workmem;
    lzo_uint zlen = 0, inlen;
    int err = 0;

    workmem = xmalloc(LZO1X_1_MEM_COMPRESS + (sizeof(lzo_align_t) - 1));
    if (!workmem) {
        gingko_err(su, "LZO work memory lost?!\n");
        return -EFAULT;
    }

    inlen = p->coff;
    err = lzo1x_1_compress(p->data, inlen, zdata, &zlen, workmem);
    if (err == LZO_E_OK) {
        err = 0;
    } else {
        gingko_err(su, "LZO compress failed w/ %d\n", err);
        err = -EINTERNAL;
        goto out;
    }

    if (zlen >= inlen) {
        gingko_err(su, "Page %p is impossible to compress.\n", p);
        p->ph.flag = SU_PH_COMP_NONE;
        goto out;
    }
    p->ph.zip_len = zlen;
    
out:
    return err;
}

int __page_comp_zlib(struct page *p, struct gingko_su *gs, void *zdata)
{
    int err = 0;

    return err;
}

static inline char *__get_algo(struct page *p)
{
    switch (p->ph.flag) {
    case SU_PH_COMP_NONE:
    default:
        return "none";
        break;
    case SU_PH_COMP_SNAPPY:
        return "snappy";
        break;
    case SU_PH_COMP_LZO:
        return "lzo";
        break;
    case SU_PH_COMP_ZLIB:
        return "zlib";
        break;
    }
}

int __write_page(struct gingko_su *gs, struct dfile *df, struct page *p,
                 void *zdata)
{
    char path[4096];
    off_t pgoff;
    int err = 0, bl, bw, zlen;

    /* get df fd now */
    xlock_lock(&df->lock);
    if (df->fds[SU_DFILE_ID] == 0) {
        sprintf(path, "%s/%s", gs->path, SU_DFILE_FILENAME);
        df->fds[SU_DFILE_ID] = open(path, O_RDWR);
        if (df->fds[SU_DFILE_ID] < 0) {
            gingko_err(su, "open(%s) failed w/ %s(%d)\n",
                       path, strerror(errno), errno);
            err = -errno;
            goto out_unlock;
        }
    }

    /* Step 0: get current pgoff */
    pgoff = lseek(df->fds[SU_DFILE_ID], 0, SEEK_END);
    if (pgoff == (off_t)-1) {
        gingko_err(su, "lseek() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        err = -errno;
        close(df->fds[SU_DFILE_ID]);
        df->fds[SU_DFILE_ID] = 0;
        goto out_unlock;
    }
    p->pgoff = pgoff;
    gingko_debug(su, "Got page %p offset %ld\n", p, p->pgoff);
    
    /* Step 1: write the page header to DFILE */
    bl = 0;
    do {
        bw = write(df->fds[SU_DFILE_ID], (void *)&p->ph + bl,
                   sizeof(p->ph) - bl);
        if (bw < 0) {
            gingko_err(su, "write page header failed w/ %s(%d)\n",
                       strerror(errno), errno);
            err = -errno;
            goto out_unlock;
        }
        bl += bw;
    } while (bl < sizeof(p->ph));
    
    /* Step 2: write the page data to DFILE */
    if (!zdata) {
        zdata = p->data;
        zlen = p->coff;
    } else
        zlen = p->ph.zip_len;

    bl = 0;
    do {
        bw = write(df->fds[SU_DFILE_ID], zdata + bl, zlen - bl);
        if (bw < 0) {
            gingko_err(su, "write page data failed w/ %s(%d)\n",
                       strerror(errno), errno);
            err = -errno;
            goto out_unlock;
        }
        bl += bw;
    } while (bl < zlen);
    
out_unlock:
    xlock_unlock(&df->lock);
    
    return err;
}

/* Sync a page to storage device, do compress if need, do pre-write
 * operations, do post-write operations after successfully written.
 *
 * Caution: this function must be called with write lock
 */
int page_sync(struct page *p, struct gingko_su *gs)
{
    void *zdata;
    int err = 0;

    switch (p->ph.status) {
    case SU_PH_CLEAN:
    case SU_PH_WBDONE:
        /* no need to to anything */
        return 0;
        break;
    case SU_PH_DIRTY:
        p->ph.status = SU_PH_WB;
        break;
    case SU_PH_WB:
        /* pending write back? it is impossible! */
        GINGKO_BUGON("In page_sync() has pending write back?!");
        break;
    default:
        return -ESTATUS;
    }

    /* do pre-compress operations */
    p->ph.orig_len = p->coff;
    p->ph.crc32 = crc32c(0, p->data, p->coff);
    zdata = alloca(p->coff);

    /* do compress */
    switch (p->ph.flag) {
    case SU_PH_COMP_NONE:
        p->ph.zip_len = p->ph.orig_len;
        break;
    case SU_PH_COMP_SNAPPY:
        err = __page_comp_snappy(p, gs, zdata);
        break;
    case SU_PH_COMP_LZO:
        err = __page_comp_lzo(p, gs, zdata);
        break;
    case SU_PH_COMP_ZLIB:
        err = __page_comp_zlib(p, gs, zdata);
        break;
    default:
        gingko_err(su, "Invalid page header flag 0x%x\n", p->ph.flag);
        err = -EINVAL;
        goto out_err;
    }
    /* do post-compress operations */
    gingko_info(su, "ZIP(%s) page data from %uB to %uB, CRC 0x%x.\n",
                __get_algo(p),
                p->ph.orig_len, p->ph.zip_len, p->ph.crc32);

    /* do pre-write operations */
    /* do write */
    err = __write_page(gs, &gs->files[p->dfid], p, zdata);
    if (err) {
        gingko_err(su, "__write_page(%p) failed w/ %s(%d)\n",
                   p, gingko_strerror(err), err);
        goto out_err;
    }
    
    /* do post-write operations */
    /* TODO: update page's pgoff, remove it from hash table */

    return err;

out_err:
    p->ph.status = SU_PH_DIRTY;
    return err;
}
