/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-31 04:56:02 macan>
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

struct page *__alloc_page(struct gingko_su *gs, int dfid, int flags)
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
    if (flags & SU_PAGE_ALLOC_ADDL2P) {
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
    xfree(p->data);

    xfree(p);
}

void dump_page(struct page *p)
{
    gingko_info(su, "Page %p %s dfid %d ref %d ph.flag %d stline %u lnr %u\n",
                p, p->suname, p->dfid,
                atomic_read(&p->ref),
                p->ph.flag, p->ph.startline, p->ph.lnr);
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
        return -EAGAIN;
        break;
    default:
        return -ESTATUS;
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

    if (p->ph.status == SU_PH_CLEAN) {
        gs->files[p->dfid].dfh->l2p.l2pa[
            gs->files[p->dfid].dfh->l2p.ph.nr - 1].lid = lid;
        gingko_err(su, "SET lid %ld @ %d\n", lid, gs->files[p->dfid].dfh->l2p.ph.nr - 1);
        p->ph.status = SU_PH_DIRTY;
    }

out:
    return err;
}

int __page_decomp_snappy(struct page *p, struct gingko_su *gs, void *zdata)
{
    int err = 0;

    return err;
}

int __page_decomp_lzo(struct page *p, struct gingko_su *gs, void *zdata)
{
    void *data = NULL;
    lzo_uint outlen;
    int err = 0;

    data = xmalloc(p->ph.orig_len);
    if (!data) {
        gingko_err(su, "xmalloc() original buffer failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }

    err = lzo1x_decompress(zdata, p->ph.zip_len, data, &outlen, NULL);
    if (err == LZO_E_OK && outlen == p->ph.orig_len) {
        err = 0;
    } else {
        gingko_err(su, "LZO decompress failed w/ %d\n", err);
        xfree(data);
        err = -EINTERNAL;
        goto out;
    }
    p->data = data;
    xfree(zdata);
    
out:
    return err;
}

int __page_decomp_zlib(struct page *p, struct gingko_su *gs, void *zdata)
{
    int err = 0;

    return err;
}

/* Read a page to page cache
 *
 * Caution: we should own the lock at begining
 */
struct page *page_load(struct gingko_su *gs, int dfid, u64 pgoff)
{
    char path[4096];
    struct page *p;
    struct dfile *df;
    void *data;
    int err = 0, bl, br;

    df = &gs->files[dfid];
    p = __alloc_page(gs, dfid, 0);
    if (IS_ERR(p)) {
        err = PTR_ERR(p);
        gingko_err(su, "__alloc_page() for pgoff 0x%lx failed w/ %s(%d)\n",
                   pgoff, gingko_strerror(err), err);
        return p;
    }

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

    /* Step 1: read the page header from DFILE */
    bl = 0;
    do {
        br = pread(df->fds[SU_DFILE_ID], (void *)&p->ph + bl,
                   sizeof(p->ph) - bl,
                   pgoff + bl);
        if (br < 0) {
            gingko_err(su, "pread page header failed w/ %s(%d)\n",
                      strerror(errno), errno);
            err = -errno;
            goto out_unlock;
        } else if (br == 0) {
            gingko_err(su, "pread page header EOF\n");
            err = -EBADF;
            goto out_unlock;
        }
        bl += br;
    } while (bl < sizeof(p->ph));
    
    /* Step 2: read the page data */
    data = xmalloc(p->ph.zip_len);
    if (!data) {
        gingko_err(su, "xmalloc() page data buffer failed, no memory.\n");
        err = -ENOMEM;
        goto out_unlock;
    }
    bl = 0;
    do {
        br = pread(df->fds[SU_DFILE_ID], data + bl,
                   p->ph.zip_len - bl, pgoff + sizeof(p->ph) + bl);
        if (br < 0) {
            gingko_err(su, "pread page data failed w/ %s(%d)\n",
                       strerror(errno), errno);
            err = -errno;
            xfree(data);
            goto out_unlock;
        } else if (br == 0) {
            gingko_err(su, "pread page data EOF\n");
            err = -EBADF;
            xfree(data);
            goto out_unlock;
        }
        bl += br;
    } while (bl < p->ph.zip_len);

    /* Step 3: unzip if compressed */
    switch (p->ph.flag) {
    case SU_PH_COMP_NONE:
        p->data = data;
        break;
    case SU_PH_COMP_SNAPPY:
        err = __page_decomp_snappy(p, gs, data);
        break;
    case SU_PH_COMP_LZO:
        err = __page_decomp_lzo(p, gs, data);
        break;
    case SU_PH_COMP_ZLIB:
        err = __page_decomp_zlib(p, gs, data);
        break;
    default:
        gingko_err(su, "Invalid page header flag 0x%x\n", p->ph.flag);
        err = -EINVAL;
        xfree(data);
        goto out_unlock;
    }
    if (err) {
        gingko_err(su, "__page_decomp_* failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        xfree(data);
        goto out_unlock;
    }

    /* Step 4: insert into page cache */
    p->pgoff = pgoff;
    p->pi->lha = p->data + p->ph.lhoff;

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
    }


out_unlock:
    xlock_unlock(&df->lock);

    if (err) {
        __page_put(p);
        __page_put(p);          /* double put as free */
        p = ERR_PTR(err);
    }

    return p;
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
                 void *zdata, off_t *pgoff)
{
    char path[4096];
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
    *pgoff = lseek(df->fds[SU_DFILE_ID], 0, SEEK_END);
    if (*pgoff == (off_t)-1) {
        gingko_err(su, "lseek() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        err = -errno;
        close(df->fds[SU_DFILE_ID]);
        df->fds[SU_DFILE_ID] = 0;
        goto out_unlock;
    }
    gingko_debug(su, "Got page %p offset %ld\n", p, *pgoff);
    
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
    void *zdata = NULL;
    off_t pgoff;
    int err = 0, i;

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
    p->ph.startline = p->pi->startline;
    p->ph.lnr = p->pi->linenr;
    p->ph.lhoff = p->coff;
    for (i = 0; i < p->ph.lnr; i++) {
        memcpy(p->data + p->coff, p->pi->lharray[i], 
               (gs->files[p->dfid].dfh->md.l1fldnr + 1) * 
               sizeof(struct lineheader));
        p->coff += (gs->files[p->dfid].dfh->md.l1fldnr + 1) * 
            sizeof(struct lineheader);
    }
    p->pi->lha = p->data + p->ph.lhoff;
    p->ph.orig_len = p->coff;
    p->ph.crc32 = crc32c(0, p->data, p->coff);

    zdata = xmalloc(p->coff);
    if (!zdata) {
        gingko_err(su, "xmalloc zdata buffer failed, no memory.\n");
        err = -ENOMEM;
        goto out_err;
    }

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
    gingko_info(su, "ZIP(%s) page data from %uB to %uB, CRC 0x%x LHoff "
                "%uB lnr %d.\n",
                __get_algo(p),
                p->ph.orig_len, p->ph.zip_len, p->ph.crc32,
                p->ph.lhoff, p->ph.lnr);

    /* do pre-write operations */
    /* do write */
    err = __write_page(gs, &gs->files[p->dfid], p, zdata, &pgoff);
    if (err) {
        gingko_err(su, "__write_page(%p) failed w/ %s(%d)\n",
                   p, gingko_strerror(err), err);
        goto out_err;
    }
    
    /* do post-write operations */
    for (i = 0; i < p->ph.lnr; i++) {
        xfree(p->pi->lharray[i]);
    }
    p->pi->lharray = NULL;

    /* TODO: update page's pgoff, remove it from hash table */
    if (!hlist_unhashed(&p->hlist)) {
        __pcrh_remove(p);
    }
    p->pgoff = pgoff;
    err = __pcrh_insert(p);
    if (err) {
        GINGKO_BUGON("Page cache conflicts?!");
    }
    gs->files[p->dfid].dfh->l2p.l2pa[
        gs->files[p->dfid].dfh->l2p.ph.nr - 1].pgoff = p->pgoff;

    err = df_append_l2p(gs, &gs->files[p->dfid]);
    if (err) {
        gingko_warning(su, "df_append_l2p() failed w/ %s(%d)\n",
                       gingko_strerror(err), err);
    }

out:
    xfree(zdata);
    
    return err;

out_err:
    p->ph.status = SU_PH_DIRTY;
    goto out;
}

struct field_t *find_field(struct gingko_su *gs, int id)
{
    struct field_t *f = NULL;

    if (gs->root) {
        f = __find_field_t(gs->root, id);
    } else {
        /* FIXME: use df 0 */
    }
    
    return f;
}

struct field_t *find_field_by_name(struct gingko_su *gs, char *name)
{
    struct field_t *f = NULL;

    if (gs->root) {
        f = __find_field_t_by_name(gs->root, name);
    } else {
        /* FIXME: use df 0 */
    }
    
    return f;
}

int page_read(struct gingko_su *gs, struct page *p, long lid, 
              struct field_g fields[], int fldnr, int flag)
{
    struct lineheader0 *lh0;
    struct lineheader *lh;
    struct field_t *f;
    int err = 0, i, j;

    if (lid < p->ph.startline || lid >= p->ph.startline + p->ph.lnr) {
        return -ENODATAR;
    }
    
    /* lookup in the line header array */
    lh = p->pi->lha + (gs->files[p->dfid].dfh->md.l1fldnr + 1) *
        (lid - p->ph.startline);
    lh0 = (void *)lh;
    for (i = 0; i < lh0->len; i++) {
        gingko_debug(su, "For %ld LH %p Got FID %d offset %d\n",
                     lid, lh, lh[i + 1].l1fld, lh[i + 1].offset);
    }

    /* upgrade field get to l1field */
    for (i = 0; i < fldnr; i++) {
        fields[i].orig_id = FLD_MAX_PID;
    }
    for (i = 0; i < fldnr; i++) {
        if (fields[i].id != FLD_MAX_PID) {
            f = find_field(gs, fields[i].id);
            if (!f) {
                gingko_err(su, "Can not find field ID=%d\n", fields[i].id);
                goto out;
            }
        } else {
            f = find_field_by_name(gs, fields[i].name);
            if (!f) {
                gingko_err(su, "Can not find field NAME=%s\n", fields[i].name);
                goto out;
            }
        }
        if (fields[i].orig_id == FLD_MAX_PID)
            fields[i].orig_id = f->fld.id;
        
        if (f->fld.pid != FLD_MAX_PID) {
            /* ok, upgrade it now */
            gingko_debug(su, "Upgrade field from (ID=%d or name=%s) to ID=%d\n", 
                         fields[i].id, fields[i].name, f->fld.pid);
            fields[i].id = f->fld.pid;
            /* Bug-XXXX: we do NOT free user allocated name here
             */
            //xfree(fields[i].name);
            fields[i].name = NULL;
            i--;
            continue;
        }

        switch (flag) {
        case UNPACK_DATAONLY:
        default:
            break;
        case UNPACK_ALL:
            fields[i].id = f->fld.id;
            fields[i].pid = f->fld.pid;
            fields[i].type = f->fld.type;
            if (!fields[i].name)
                fields[i].name = strdup(f->fld.name);
            break;
        }

        /* calc data len */
        for (j = 0; j < gs->files[p->dfid].dfh->md.l1fldnr; j++) {
            long this, next;

            if (f->fld.id == lh[j + 1].l1fld) {
                this = lh[j + 1].offset;
                if (j + 1 < gs->files[p->dfid].dfh->md.l1fldnr)
                    next = lh[j + 2].offset;
                else {
                    struct lineheader0 *lh0 = (void *)lh;
                    next = lh0->llen;
                    if (lh0->len > 0)
                        next += lh[1].offset;
                }

                gingko_debug(su, "J=%d fld.id=%d this=%ld next=%ld\n", 
                             j, f->fld.id, this, next);
                err = lineunpack(p->data + lh[j + 1].offset,
                                 next - this, f, &fields[i]);
                if (err) {
                    gingko_err(su, "lineunpack() fld.id=%d failed w/ %s(%d)\n",
                               f->fld.id, gingko_strerror(err), err);
                    goto out;
                }
                break;
            }
        }

        /* finally, try to parse the field */
        gingko_debug(su, "Compare ID=%d ORIG_ID=%d\n", 
                     fields[i].id, fields[i].orig_id);
        if (fields[i].orig_id != fields[i].id) {
            err = lineparse(&fields[i], f, find_field(gs, fields[i].orig_id));
            if (err) {
                gingko_err(su, "lineparse() ID=%d Orig_ID=%d failed w/ %s(%d)\n",
                           fields[i].id, fields[i].orig_id,
                           gingko_strerror(err), err);
                goto out;
            }
        }
    }
    
out:
    return err;
}

