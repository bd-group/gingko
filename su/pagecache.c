/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-11 20:17:01 macan>
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

    sem_t async_page_sync_sem;
    pthread_t *async_threads;
    int *async_page_sync_thread_stop;
    int async_page_sync_thread_nr;

    struct list_head async_page_list;
    xlock_t async_page_lock;
};

static struct page_cache gpc;

struct async_psync
{
    struct list_head list;
    struct page *p;
    struct gingko_su *gs;
};

static void *__async_page_sync_main(void *args);

int pagecache_init(struct gingko_conf *conf)
{
    int err = 0, i;

    memset(&gpc, 0, sizeof(gpc));
    gpc.async_page_sync_thread_nr = conf->async_page_sync_thread_nr;
    if (!gpc.async_page_sync_thread_nr)
        gpc.async_page_sync_thread_nr = GINGKO_ASYNC_PSYNC_TNR;
    gpc.async_page_sync_thread_stop = xzalloc(sizeof(int) *
                                              gpc.async_page_sync_thread_nr);
    if (!gpc.async_page_sync_thread_stop) {
        gingko_err(su, "xzalloc(%d * int) failed\n",
                   gpc.async_page_sync_thread_nr);
        err = -ENOMEM;
        goto out;
    }
    sem_init(&gpc.async_page_sync_sem, 0, 0);

    INIT_LIST_HEAD(&gpc.async_page_list);
    xlock_init(&gpc.async_page_lock);

    xlock_init(&gpc.lock);
    gpc.pcsize = conf->pcrh_size;
    gpc.pcrh = xzalloc(sizeof(*gpc.pcrh) * gpc.pcsize);
    if (!gpc.pcrh) {
        gingko_err(su, "xzalloc(%d * PCRH) failed\n", gpc.pcsize);
        err = -ENOMEM;
        goto out_free;
    }
    for (i = 0; i < gpc.pcsize; i++) {
        INIT_HLIST_HEAD(&gpc.pcrh[i].h);
        xlock_init(&gpc.pcrh[i].lock);
    }

    /* start async threads */
    gpc.async_threads = xmalloc(sizeof(pthread_t) *
                                gpc.async_page_sync_thread_nr);
    if (!gpc.async_threads) {
        gingko_err(su, "xmalloc() pthread_t failed\n");
        err = -ENOMEM;
        goto out_free2;
    }
    for (i = 0; i < gpc.async_page_sync_thread_nr; i++) {
        err = pthread_create(&gpc.async_threads[i], NULL, 
                             &__async_page_sync_main, (void *)(long)i);
        if (err) {
            gingko_err(su, "Create async page sync thread failed w/ %s(%d)\n",
                       gingko_strerror(errno), errno);
            err = -errno;
            goto out_destroy;
        }
    }
    gingko_info(su, "Start %d async page sync thread.\n",
                gpc.async_page_sync_thread_nr);
    
out:
    return err;
out_destroy:
    {
        int j;
        for (j = i; j > 0; j--) {
            gpc.async_page_sync_thread_stop[j] = 1;
        }
        for (j = i; j > 0; j--) {
            sem_post(&gpc.async_page_sync_sem);
        }
        for (j = i; j > 0; j--) {
            pthread_join(gpc.async_threads[j], NULL);
        }
    }
out_free2:
    xfree(gpc.async_threads);
out_free:
    xfree(gpc.async_page_sync_thread_stop);
    goto out;
}

int pagecache_fina()
{
    struct page *p;
    struct hlist_node *pos, *n;
    struct regular_hash *rh;
    int i;
    
    for (i = 0; i < gpc.pcsize; i++) {
        rh = gpc.pcrh + i;
        xlock_lock(&rh->lock);
        hlist_for_each_entry_safe(p, pos, n, &rh->h, hlist) {
            if (p->ph.status == SU_PH_DIRTY) {
                char info[1024];
                sprintf(info, "Dirty page for GS %s still exists.", p->suname); 
                GINGKO_BUGON(info);
            }
        }
        xlock_unlock(&rh->lock);
    }
    xfree(gpc.pcrh);

    return 0;
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
        p = __alloc_page(gs, dfid, SU_PAGE_ALLOC_ADDL2P);
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

void async_page_sync(struct page *p, struct gingko_su *gs)
{
    struct async_psync *ap = xmalloc(sizeof(struct async_psync));
    int added = 0;

    if (!ap)
        return;
    INIT_LIST_HEAD(&ap->list);
    ap->p = p;
    ap->gs = gs;
    
    xrwlock_wlock(&p->rwlock);
    if (p->ph.status == SU_PH_DIRTY) {
        xlock_lock(&gpc.async_page_lock);
        list_add_tail(&gpc.async_page_list, &ap->list);
        xlock_unlock(&gpc.async_page_lock);
        added = 1;
    }
    xrwlock_wunlock(&p->rwlock);
    if (added) {
        sem_post(&gpc.async_page_sync_sem);
    }
}

static void *__async_page_sync_main(void *args)
{
    sigset_t set;
    struct async_psync *ap;
    int err, tid = (int)(long)args;

    /* first, let us block the SIGALRM */
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &set, NULL); /* oh, we do not care about the
                                             * errs */
    /* then, we loop for the page sync events */
    while (!gpc.async_page_sync_thread_stop[tid]) {
        err = sem_wait(&gpc.async_page_sync_sem);
        if (err) {
            if (errno == EINTR)
                continue;
            gingko_err(su, "sem_wait() failed w/ %s\n", strerror(errno));
        }

        ap = NULL;
        xlock_lock(&gpc.async_page_lock);
        if (!list_empty(&gpc.async_page_list)) {
            ap = list_first_entry(&gpc.async_page_list, 
                                  struct async_psync, list);
        }
        xlock_unlock(&gpc.async_page_lock);
        if (ap) {
            xrwlock_wlock(&ap->p->rwlock);
            err = page_sync(ap->p, ap->gs);
            xrwlock_wunlock(&ap->p->rwlock);
            if (err) {
                gingko_err(api, "page_sync() SU %s DF %d PGOFF %lx "
                           "failed w/ %s(%d)\n",
                           ap->p->suname, ap->p->dfid, ap->p->pgoff,
                           gingko_strerror(err), err);
            }
        }
    }

    gingko_debug(su, "Hooo, I am exiting...\n");
    pthread_exit(0);
}

