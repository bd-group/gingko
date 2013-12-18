/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-24 21:33:28 macan>
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

#ifdef GINGKO_TRACING
u32 gingko_api_tracing_flags;
#endif

static struct gingko_conf g_conf = {
    .max_suid = GINGKO_MAX_SUID,
    .gsrh_size = GINGKO_GSRH_SIZE,
    .pcrh_size = GINGKO_PCRH_SIZE,
};

static struct su_conf g_sc = {
    .page_size = SU_PAGE_SIZE,
    .page_algo = SU_PH_COMP_NONE,
};

struct gingko_suid
{
#define GSU_FREE        0       /* can use for allocate */
#define GSU_INIT        1       /* inited but not open */
#define GSU_OPEN        2       /* openned for use */
#define GSU_CLOSE       3       /* closed and cached */
    int state;

    int mode;
    struct gingko_su *gs;
};

struct gingko_manager
{
    struct gingko_conf *gc;

    struct gingko_suid *gsu;
    xlock_t gsu_lock;
    int gsu_used;

    struct regular_hash *gsrh;
};

struct gingko_manager g_mgr = {
    .gc = &g_conf,
    .gsu = NULL,
    .gsu_used = 0,
};

int gingko_init(struct gingko_conf *conf)
{
    int err = 0, i;

    if (conf == NULL)
        conf = &g_conf;

    g_mgr.gc = conf;

    g_mgr.gsu = xzalloc(sizeof(*g_mgr.gsu) * conf->max_suid);
    if (!g_mgr.gsu) {
        gingko_err(api, "xzalloc(%d * gingko_su*) failed\n", conf->max_suid);
        err = -ENOMEM;
        goto out;
    }
    xlock_init(&g_mgr.gsu_lock);

    g_mgr.gsrh = xzalloc(sizeof(*g_mgr.gsrh) * conf->gsrh_size);
    if (!g_mgr.gsrh) {
        gingko_err(api, "xzalloc(%d * GSRH) failed\n", conf->gsrh_size);
        err = -ENOMEM;
        goto out;
    }
    for (i = 0; i < conf->gsrh_size; i++) {
        INIT_HLIST_HEAD(&g_mgr.gsrh[i].h);
        xlock_init(&g_mgr.gsrh[i].lock);
    }

    /* init lib */
    lib_init();

    /* init page cache */
    err = pagecache_init(conf);
    if (err) {
        gingko_err(api, "page cache init failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out;
    }
    
out:
    return err;
}

int gingko_fina(void)
{
    int err = 0;
    
    /* free all resources */

    return err;
}

static char *__generate_name()
{
    char name[SU_NAME_LEN + 1];
    int i;

    memset(name, 0, SU_NAME_LEN + 1);
    for (i = 0; i < SU_NAME_LEN; i++) {
        u8 r = lib_random(26);
        name[i] = 'a' + r;
    }

    return strdup(name);
}

static inline int __calc_gsrh_slot(char *suname)
{
    return RSHash(suname, SU_NAME_LEN) % g_mgr.gc->gsrh_size;
}

struct gingko_su *__gsrh_lookup(char *suname)
{
    int idx = __calc_gsrh_slot(suname);
    int found = 0;
    struct hlist_node *pos;
    struct regular_hash *rh;
    struct gingko_su *gs;

    rh = g_mgr.gsrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(gs, pos, &rh->h, hlist) {
        if (strncmp(gs->sm.name, suname, SU_NAME_LEN) == 0) {
            /* ok, found it */
            found = 1;
            __gs_get(gs);
            break;
        }
    }
    xlock_unlock(&rh->lock);
    if (!found) {
        gs = NULL;
    }

    return gs;
}

/* Return value:
 *  0 => inserted
 *  1 => not inserted
 */
int __gsrh_insert(struct gingko_su *gs)
{
    int idx = __calc_gsrh_slot(gs->sm.name);
    int found = 0;
    struct regular_hash *rh;
    struct hlist_node *pos;
    struct gingko_su *tpos;

    rh = g_mgr.gsrh + idx;
    xlock_lock(&rh->lock);
    hlist_for_each_entry(tpos, pos, &rh->h, hlist) {
        if (strncmp(tpos->sm.name, gs->sm.name, SU_NAME_LEN) == 0) {
            /* this means we have found the same entry in hash table, do NOT
             * insert it */
            found = 1;
            break;
        }
    }

    if (!found) {
        /* ok, insert it to hash table */
        hlist_add_head(&gs->hlist, &rh->h);
    }
    xlock_unlock(&rh->lock);

    return found;
}

void __gsrh_remove(struct gingko_su *gs) 
{
    int idx = __calc_gsrh_slot(gs->sm.name);
    struct regular_hash *rh;

    rh = g_mgr.gsrh + idx;
    xlock_lock(&rh->lock);
    hlist_del_init(&gs->hlist);
    xlock_unlock(&rh->lock);
}

struct gingko_suid *__alloc_su(struct gingko_manager *gm, int flag)
{
    struct gingko_suid *gid = NULL;
    struct gingko_su *gs = NULL;
    int i, found = 0;
    
    if (!gm)
        return NULL;

    gs = xzalloc(sizeof(*gs));
    if (!gs)
        return NULL;

    xlock_lock(&gm->gsu_lock);
    for (i = 0; i < gm->gc->max_suid; i++) {
        gid = gm->gsu + i;
        if (gid->state == GSU_FREE ||
            (gid->state == GSU_CLOSE && flag == GSU_ALLOC_HARD)) {
            /* ok, this entry is good to use */
            found = 1;
            gid->state = GSU_INIT;
            break;
        }
    }
    xlock_unlock(&gm->gsu_lock);

    if (found) {
        INIT_HLIST_NODE(&gs->hlist);
        atomic_set(&gs->ref, 0);
        /* init the ROOT of schema tree */
        {
            struct field_t *root;

            root = alloc_field_t(FIELD_TYPE_ROOT);
            if (!root) {
                gingko_err(su, "alloc root field_t failed.\n");
                return NULL;
            }
            gs->root = root;
        }
        gid ->gs = gs;
    }

    return gid;
}

static int __calc_suid(struct gingko_manager *gm, struct gingko_suid *gid)
{
    if (!gm || !gid)
        return -1;

    return ((void *)gid - (void *)gm->gsu) / sizeof(*gid);
}

void __free_suid(struct gingko_suid *gid)
{
    __gs_put(gid->gs);

    memset(gid, 0, sizeof(*gid));
}

void __free_su(struct gingko_su *gs)
{
    int i;

    if (!gs)
        return;

    gingko_verbose(api, "Free SU %p\n", gs);

    if (atomic_read(&gs->ref) >= 0) {
        GINGKO_BUGON("invalid SU reference.");
    }

    /* TODO: free any resource, then set GSU_FREE flag */
    if (!hlist_unhashed(&gs->hlist)) {
        __gsrh_remove(gs);
    }
    
    xfree(gs->path);
    xfree(gs->files);

    if (gs->files) {
        for (i = 0; i < gs->sm.dfnr; i++) {
            fina_dfile(&gs->files[i]);
        }
    }

    if (gs->smfd > 0) {
        fsync(gs->smfd);
        close(gs->smfd);
        gs->smfd = 0;
    }

    if (gs->root) {
        __post_trav_schemas(gs->root, __free_schema);
    }

    xfree(gs->sm.name);

    xfree(gs);
}

static int __su_sync(struct gingko_su *gs)
{
    int err = 0;

    return err;
}

void __recycle_su_fd(struct gingko_su *gs)
{
    int err = 0, i, j;
    
    /* sync the su now */
    err = __su_sync(gs);
    if (err) {
        gingko_err(api, "__su_sync(%s) failed w/ %s(%d)\n",
                   gs->sm.name, gingko_strerror(err), err);
        goto out;
    }
    
    /* then, close the files */
    close(gs->smfd);
    gs->smfd = 0;

    for (i = 0; i < gs->sm.dfnr; i++) {
        for (j = 0; j < SU_PER_DFILE_MAX; j++) {
            if (gs->files[i].fds[j] != 0) {
                close(gs->files[i].fds[j]);
                gs->files[i].fds[j] = 0;
            }
        }
    }
    gingko_info(api, "Recycle fds from SU %s, done.", gs->sm.name);
    
out:
    return;
}

/* Open a SU, return SUID (>=0)
 * On error, return (<0)
 */
int su_open(char *supath, int mode) 
{
    struct gingko_suid *gid = NULL;
    struct gingko_su *gs = NULL;
    struct stat st;
    int suid, err = 0, i;

    /* Step 1: find and allocate the gingko_su */
    gid = __alloc_su(&g_mgr, GSU_ALLOC_FREE_ONLY);
    if (!gid) {
        gingko_err(api, "allocate SU (FREE_ONLY) failed.\n");
        err = -ENOMEM;
        goto out;
    }
    gid->mode = mode;
    gs = gid->gs;
    suid = __calc_suid(&g_mgr, gid);

    /* Step 2: check if the path exists and contains content */
    err = stat(supath, &st);
    if (err) {
        gingko_err(api, "stat(%s) failed w/ %s(%d)\n",
                   supath, strerror(errno), errno);
        err = -errno;
        goto out_free;
    }
    if (!S_ISDIR(st.st_mode)) {
        gingko_err(api, "Path '%s' is not a directory.\n",
                   supath);
        err = -ENOTDIR;
        goto out_free;
    }

    /* Step 3: read the su.meta file */
    gs->path = strdup(supath);
    err = su_read_meta(gs);
    if (err) {
        goto out_free;
    }

    /* find conflicts quickly */
    {
        struct gingko_su *other;

        other = __gsrh_lookup(gs->sm.name);
        if (other) {
            /* ok, check flags and return this SU */
            gingko_debug(api, "Find this SU '%s' in hash table, check it!\n",
                         gs->sm.name);
            switch (mode) {
            default:
            case SU_OPEN_RDONLY:
                if (other->sm.flags & GSU_META_RW) {
                    /* ok, read concurrent w/ write */
                    gingko_debug(api, "SU '%s' might read concurrent w/ "
                                 "write.\n", gs->sm.name);
                }
                break;
            case SU_OPEN_APPEND:
                if (other->sm.flags & GSU_META_RDONLY) {
                    /* ok, upgrade it to RW */
                    gingko_debug(api, "SU '%s' read upgrade to write.\n",
                                 gs->sm.name);
                    other->sm.flags = GSU_META_RW;
                } else if (other->sm.flags & GSU_META_RW) {
                    /* concurrent write? */
                    err = -ECONFLICT;
                    __gs_put(other);
                    __gs_put(gs);
                    goto out;
                }
                break;
            }
            __gs_put(gs);
            gid->gs = other;
            gingko_debug(api, "Reuse SU %p idx %d\n", gid->gs, suid);
            err = suid;
            goto out;
        }
    }
    
    switch (mode) {
    default:
    case SU_OPEN_RDONLY:
        if (gs->sm.flags & GSU_META_BAD) {
            gingko_debug(api, "Reject to open BAD SU '%s'.\n", supath);
            err = -EBADSU;
            goto out_free;
        }
        gs->sm.flags = GSU_META_RDONLY;
        break;
    case SU_OPEN_APPEND:
        if (gs->sm.flags & GSU_META_BAD ||
            gs->sm.flags & GSU_META_RDONLY) {
            gingko_debug(api, "Reject to open BAD or RDONLY SU '%s'.\n",
                         supath);
            err = -EBADSU;
            goto out_free;
        }
        gs->sm.flags = GSU_META_RW;
        break;
    }

    /* Step 4: read the dfiles */
    gs->files = xzalloc(sizeof(struct dfile) * gs->sm.dfnr);
    if (!gs->files) {
        gingko_err(api, "xzalloc DFILEs failed\n");
        err = -ENOMEM;
        goto out_free;
    }
    
    for (i = 0; i < gs->sm.dfnr; i++) {
        err = alloc_dfile(&gs->files[i]);
        if (err) {
            gingko_err(api, "alloc_dfile failed w/ %s(%d)\n",
                       gingko_strerror(err), err);
            goto out_free;
        }
        err = df_read_meta(gs, &gs->files[i]);
        if (err) {
            gingko_err(api, "df_read_meta failed w/ %s(%d)\n",
                       gingko_strerror(err), err);
            goto out_free;
        }
        switch (gs->files[i].dfh->md.status) {
        case DF_META_STATUS_INCREATE:
            /* this means someone hadn't complete su create, reject to open */
            gingko_debug(api, "Reject to open INCREATE SU '%s'.\n",
                         supath);
            err = -EINCREATE;
            goto out_free;
            break;
        case DF_META_STATUS_INITED:
            /* it is ok */
            break;
        case DF_META_STATUS_RDONLY:
            if (mode != SU_OPEN_RDONLY) {
                gingko_debug(api, "Reject to open RDONLY SU '%s'.\n",
                             supath);
                err = -ERDONLY;
                goto out_free;
            }
        default:
            break;
        }
    }

    /* insert it to the hash table and find conflicts */
    __gs_get(gs);
    
retry:
    switch (__gsrh_insert(gs)) {
    case 0:
        /* inserted, it is ok */
        break;
    case 1:
    {
        /* not inserted, lookup then free current entry */
        struct gingko_su *other;

        other = __gsrh_lookup(gs->sm.name);
        if (!other) {
            goto retry;
        }
        __gs_put(gs);
        __gs_put(gs);           /* double put as free */
        gid->gs = other;
        break;
    }
    default:
        err = -EINTERNAL;
        __gs_put(gs);
        goto out_free;
    }

    /* ok, we have build the GS, return the suid now */
    gingko_debug(api, "Allocate SU %p idx %d\n", gs, suid);
    err = suid;
    
out:
    return err;

out_free:
    __free_suid(gid);
    return err;
}

/* Get schema info from SU (dfile), should open SU firstly
 */
struct field *su_getschema(int suid)
{
    return NULL;
}

/* Create a SU
 *
 * Args: parent(supath) should exist, and supath should not exist
 */
int su_create(struct su_conf *sc, char *supath, struct field schemas[], 
              int schelen)
{
    struct stat st;
    struct gingko_suid *gid;
    struct gingko_su *gs;
    int err = 0, suid, i;

    err = stat(supath, &st);
    if (err) {
        if (errno == ENOENT) {
            /* it is ok to create it */
            err = mkdir(supath, 0775);
            if (err) {
                gingko_err(api, "mdir(%s) failed w/ %s(%d)\n",
                           supath, strerror(errno), errno);
                err = -errno;
                goto out;
            }
        } else {
            gingko_err(api, "stat(%s) failed w/ %s(%d)\n",
                       supath, strerror(errno), errno);
            err = -errno;
            goto out;
        }
    } else {
        /* there exists a SU in supath */
        gingko_err(api, "SU exists in your path '%s', remove it if you "
                   "truely want to create at there.\n", supath);
        err = -EEXIST;
        goto out;
    }

    gid = __alloc_su(&g_mgr, GSU_ALLOC_FREE_ONLY);
    if (!gid) {
        gingko_err(api, "allocate SU (FREE_ONLY) faield, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    gid->mode = SU_OPEN_APPEND;
    gs = gid->gs;
    gs->path = strdup(supath);
    if (!gs->path) {
        gingko_err(api, "allocate supath failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }

    if (sc == NULL)
        gs->conf = g_sc;
    else
        gs->conf = *sc;

    suid = __calc_suid(&g_mgr, gid);
    
    /* Step 1: write the su.meta */
    gs->sm.version = SU_META_VERSION;
    gs->sm.flags = GSU_META_RW;
    gs->sm.dfnr = 1;
retry:
    gs->sm.name = __generate_name();

    /* find conflicts quickly */
    {
        struct gingko_su *other;

        other = __gsrh_lookup(gs->sm.name);
        if (other) {
            /* conflicts, then regenerate name */
            __gs_put(other);
            goto retry;
        }
    }
    
    err = su_write_meta(gs);
    if (err) {
        gingko_err(api, "su_write_meta() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        goto out_free;
    }

    /* Step 2: parse field schema to generate a schema tree */
    err = verify_schema(gs, schemas, schelen);
    if (err) {
        gingko_err(api, "verify_schema() failed w/ %s(%d)\n",
                   strerror(errno), errno);
        goto out_free;
    }

    /* Step 3: new dfiles */
    gs->files = xzalloc(sizeof(struct dfile) * gs->sm.dfnr);
    if (!gs->files) {
        gingko_err(api, "xzalloc() dfile failed, no memory.\n");
        err = -ENOMEM;
        goto out_free;
    }

    for (i = 0; i < gs->sm.dfnr; i++) {
        err = init_dfile(gs, schemas, schelen, gs->files + i);
        if (err) {
            gingko_err(api, "init_dfile() failed w/ %s(%d)\n",
                       gingko_strerror(err), err);
            goto out_free;
        }
    }

    /* finally, add this SU to hash table */
    __gs_get(gs);

    switch (__gsrh_insert(gs)) {
    case 0:
        /* inserted, it is ok */
        break;
    case 1:
    {
        /* not inserted, failed and report ERROR */
        err = -ECONFLICT;
        __gs_put(gs);
        goto out_free;
        break;
    }
    default:
        err = -EINTERNAL;
        __gs_put(gs);
        goto out_free;
    }

    /* ok, we have build the GS, return the suid now */    
    gingko_debug(api, "Allocate SU %p idx %d\n", gs, suid);
    err = suid;

out:
    return err;
out_free:
    __free_suid(gid);
    return err;
}

struct field_2pack *su_new_field(u8 type, void *data, int dlen)
{
    struct field_2pack *fld;

    fld = xzalloc(sizeof(*fld));
    if (!fld) {
        gingko_err(api, "xzalloc() field_2pack failed\n");
        fld = ERR_PTR(-ENOMEM);
        goto out;
    }
    fld->type = type;
    fld->data = data;
    fld->dlen = dlen;
    
out:
    return fld;
}

int su_fieldpack(struct field_2pack *pfld, struct field_2pack *cfld)
{
    void *tmp;
    int err = 0;
    
    if (!pfld || !cfld || IS_ERR(cfld) || IS_ERR(pfld))
        return -EINVAL;
    if (pfld->cidnr == 0)
        pfld->clds = NULL;

    tmp = xrealloc(pfld->clds, sizeof(pfld) * (pfld->cidnr + 1));
    if (!tmp) {
        gingko_err(api, "xrealloc() child f2p failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    pfld->clds = tmp;
    pfld->clds[pfld->cidnr] = cfld;
    pfld->cidnr++;
    
out:
    return err;
}

struct field_2pack **su_l1fieldpack(struct field_2pack **fld, int *fldnr,
                                    struct field_2pack *new)
{
    void *p;

    if (IS_ERR(new) || !new || !fldnr)
        return ERR_PTR(-EINVAL);
    
    p = xrealloc(fld, sizeof(*fld) * (*fldnr + 1));
    if (!p) {
        gingko_err(api, "xrealloc field_2pack* array failed, no memory.\n");
        return ERR_PTR(-ENOMEM);
    }
    fld = p;
    fld[*fldnr] = new;
    *fldnr += 1;

    return p;
}

/* Build a line
 */
int su_linepack(struct line *line, struct field_2pack *flds[], int l1fldnr)
{
    int err = 0, i;
    void *x;

    if (!line || !flds)
        return -EINVAL;

    x = xzalloc((l1fldnr + 1) * sizeof(struct lineheader));
    if (!x) {
        gingko_err(api, "xzalloc() %d linehader failed, no memory.\n",
                   l1fldnr);
        err = -ENOMEM;
        goto out;
    }
    line->lh = x;
    line->lh[0].l1fld = l1fldnr;
    line->lh[0].len = l1fldnr;
    
    memset(line, 0, sizeof(*line));

    for (i = 0; i < l1fldnr; i++) {
        switch (flds[i]->type) {
        case GINGKO_INT8:
            err = linepack_primitive(line, flds[i]->data, 1);
            break;
        case GINGKO_INT16:
            err = linepack_primitive(line, flds[i]->data, 2);
            break;
        case GINGKO_INT32:
        case GINGKO_FLOAT:
            err = linepack_primitive(line, flds[i]->data, 4);
            break;
        case GINGKO_INT64:
        case GINGKO_DOUBLE:
            err = linepack_primitive(line, flds[i]->data, 8);
            break;
        case GINGKO_STRING:
        case GINGKO_BYTES:
            err = linepack_string(line, flds[i]->data, flds[i]->dlen);
            break;
        case GINGKO_ARRAY:
        case GINGKO_STRUCT:
        case GINGKO_MAP:
            err = linepack_complex(line, flds[i]);
            break;
        default:
            gingko_err(api, "Invalid field type=%d\n", flds[i]->type);
            err = -EINVAL;
            goto out;
        }
    }

out:
    return err;
}

/* Write a line to SU
 */
int su_write(int suid, struct line *line, long lid)
{
    struct gingko_suid *gid;
    int err = 0;

    if (suid >= g_mgr.gc->max_suid) {
        err = -EINVAL;
        goto out;
    }
    gid = g_mgr.gsu + suid;

    /* check if this gs allows write */
    if (gid->mode != SU_OPEN_APPEND) {
        err = -ERDONLY;
        goto out;
    }

    if (lid != gid->gs->last_lid) {
        gingko_err(api, "Invalid line ID %ld but expect %ld.\n",
                   lid, gid->gs->last_lid);
        err = -EINVAL;
        goto out;
    }

    /* write to page and build lineheaders */
    {
        /* FIXME: in this version, we only allow write to the zero-th dfile */
        struct page *p = get_page(gid->gs, 0);

        if (IS_ERR(p)) {
            err = PTR_ERR(p);
            gingko_err(api, "get_page() failed w/ %s(%d)\n",
                       gingko_strerror(err), err);
            goto out;
        }

        xrwlock_wlock(&p->lock);
        err = page_write(p, line, lid, gid->gs);
        xrwlock_wunlock(&p->lock);
        
        put_page(p);
    }

out:
    return err;
}

/* Sync the memroy cached SU content to disk
 */
int su_sync(int suid)
{
    struct gingko_suid *gid;
    int err = 0;

    if (suid >= g_mgr.gc->max_suid) {
        err = -EINVAL;
        goto out;
    }

    gid = g_mgr.gsu + suid;
    err = __su_sync(gid->gs);
    if (err) {
        gingko_err(api, "__su_sync(%s) failed w/ %s(%d)\n",
                   gid->gs->sm.name, gingko_strerror(err), err);
        goto out;
    }
    
out:
    return err;
}

/* Close the SU
 */
int su_close(int suid)
{
    struct gingko_suid *gid;
    int err = 0;

    if (suid >= g_mgr.gc->max_suid) {
        err = -EINVAL;
        goto out;
    }
    
    gid = g_mgr.gsu + suid;
    gingko_debug(api, "Close SU '%s', id=%d, pre-put:ref=%d\n",
                 gid->gs->sm.name, suid, atomic_read(&gid->gs->ref));

    __free_suid(gid);

out:
    return err;
}
