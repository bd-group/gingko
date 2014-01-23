/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-22 13:39:22 macan>
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

char *gingko_type(int type)
{
    switch (type) {
    case GINGKO_INT8:
        return "int8";
    case GINGKO_INT16:
        return "int16";
    case GINGKO_INT32:
        return "int32";
    case GINGKO_INT64:
        return "int64";
    case GINGKO_FLOAT:
        return "float";
    case GINGKO_DOUBLE:
        return "double";
    case GINGKO_STRING:
        return "string";
    case GINGKO_BYTES:
        return "bytes";
    case GINGKO_ARRAY:
        return "array";
    case GINGKO_STRUCT:
        return "struct";
    case GINGKO_MAP:
        return "map";
    default:
        return "unknown";
    }
}

static struct field_t *__alloc_field_t(int type)
{
    struct field_t *f = NULL;

    f = xzalloc(sizeof(*f));
    if (!f) {
        gingko_err(su, "xzalloc() field_t failed, no memory.\n");
        return NULL;
    }

    f->type = type;

    return f;
}

struct field_t *alloc_field_t(int type)
{
    return __alloc_field_t(type);
}

static void __free_field_t(struct field_t *f)
{
    xfree(f->cld);
    xfree(f);
}

static int __enlarge_field_t(struct field_t *f)
{
    struct field_t **tmp;

    f->cnr++;
    tmp = xrealloc(f->cld, sizeof(struct field_t **) * f->cnr);
    if (!tmp) {
        gingko_err(su, "xrealloc child fields failed, no memory.\n");
        return -ENOMEM;
    }
    memset(&tmp[f->cnr - 1], 0, sizeof(struct field_t *));
    f->cld = tmp;

    return 0;
}

static int __add_to_root(struct field_t *root, struct field_t *node)
{
    int err = 0;

    err = __enlarge_field_t(root);
    if (err) {
        gingko_err(su, "enlarge field_t failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out;
    }
    root->cld[root->cnr - 1] = node;
    if (root->type != FIELD_TYPE_ROOT ||
        node->type != FIELD_TYPE_ROOT) {
        GINGKO_BUGON("Only subtree root can be add to ROOT.");
    }

out:
    return err;
}

static int __add_to_field_t(struct field_t *node, struct field f)
{
    struct field_t *pos;
    int err = 0;

    pos = __alloc_field_t(FIELD_TYPE_LEAF);
    if (!pos) {
        gingko_err(su, "alloc INTER/LEAF field_t failed.\n");
        err = -ENOMEM;
        goto out;
    }

    err = __enlarge_field_t(node);
    if (err) {
        gingko_err(su, "enlarge field_t failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out_free;
    }
    node->cld[node->cnr - 1] = pos;
    pos->fld = f;
    if (node->type == FIELD_TYPE_LEAF)
        node->type = FIELD_TYPE_INTER;

out:
    return err;
out_free:
    __free_field_t(pos);
    goto out;
}

static inline 
struct field_t *__find_field_t(struct field_t *root, int id)
{
    struct field_t *target = NULL;
    int i = 0;

    if (root->type != FIELD_TYPE_ROOT && root->fld.id == id) {
        target = root;
        goto out;
    }

    /* iterate on root type */
    for (i = 0; i < root->cnr; i++) {
        target = __find_field_t(root->cld[i], id);
        if (target) {
            goto out;
        }
    }

out:
    return target;
}

static inline
struct field_t *__find_field_t_by_name(struct field_t *root, char *name)
{
    struct field_t *target = NULL;
    int i = 0;
    
    if (root->type != FIELD_TYPE_ROOT && strcmp(root->fld.name, name) == 0) {
        target = root;
        goto out;
    }

    /* iterate on root type */
    for (i = 0; i < root->cnr; i++) {
        target = __find_field_t_by_name(root->cld[i], name);
        if (target) {
            goto out;
        }
    }

out:
    return target;
}

int __init_fc(struct gingko_su *gs)
{
    int err = 0;
    
    if (!gs->conf.fc_size)
        gs->conf.fc_size = SU_FC_SIZE;
    gs->fc = xzalloc(sizeof(*gs->fc) * gs->conf.fc_size);
    if (!gs->fc) {
        gingko_err(su, "xzalloc(%d * FC) failed\n", 
                   gs->conf.fc_size);
        err = -ENOMEM;
        goto out;
    }

out:
    return err;
}

/* Return value:
 *  0 => inserted
 *  1 => not inserted
 * -1 => error
 */
int __insert_fc(struct field_t *f, struct gingko_su *gs)
{
    if (f->fld.id >= gs->conf.fc_size) {
        /* disable fc */
        return -1;
    }
    gs->fc[f->fld.id].ft = f;

    return 0;
}

void __fina_fc(struct gingko_su *gs)
{
    xfree(gs->fc);
}

struct field_t *find_field_by_id(struct gingko_su *gs, int id)
{
    struct field_t *f = NULL;

    if (likely(gs->fc)) {
        return gs->fc[id].ft;
    }

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

void __print_schema(void *arg, void *arg1)
{
    struct field_t *f = (struct field_t *)arg;

    gingko_info(su, "SCHEMA: NTYPE=%d ID=%d PID=%5d FTYPE=%2d\n",
                f->type, f->fld.id, f->fld.pid, f->fld.type);
}

void __free_schema(void *arg, void *arg1)
{
    __free_field_t((struct field_t *)arg);
}

void __build_fc(void *arg, void *arg1)
{
    struct field_t *f = (struct field_t *)arg;
    struct gingko_su *gs = (struct gingko_su *)arg1;

    if (f->type == FIELD_TYPE_ROOT)
        return;

    if (gs->fc) {
        switch (__insert_fc(f, gs)) {
        case 0:
            break;
        case -1:
        case 1:
            /* free gs->fc now */
            __fina_fc(gs);
            gs->fc = NULL;
            break;
        }
    }
}

void __pre_trav_schemas(struct field_t *root, tfunc cb, void *arg1)
{
    int i;
    
    cb(root, arg1);
    for (i = 0; i < root->cnr; i++) {
        __pre_trav_schemas(root->cld[i], cb, arg1);
    }
}

void __post_trav_schemas(struct field_t *root, tfunc cb, void *arg1)
{
    int i;

    for (i = 0; i < root->cnr; i++) {
        __post_trav_schemas(root->cld[i], cb, arg1);
    }
    cb(root, arg1);
}

static int __find_add_to_field_t(struct field_t *root, struct field f)
{
    struct field_t *target = NULL;
    int err = 0;

    target = __find_field_t(root, f.pid);
    if (!target) {
        gingko_err(su, "find pfield ID=%d failed, no such entry.\n",
                   f.pid);
        err = -EINVAL;
        goto out;
    }

    err = __add_to_field_t(target, f);
    if (err) {
        gingko_err(su, "add field ID=%d to parent field ID=%d "
                   "failed w/ %s(%d).\n",
                   f.id, f.pid, gingko_strerror(err), err);
        goto out;
    }

out:
    return err;
}

int verify_schema(struct gingko_su *gs, struct field schemas[], int schelen)
{
    struct field_t *root;
    int err = 0, i;

    root = __alloc_field_t(FIELD_TYPE_ROOT);
    if (!root) {
        gingko_err(su, "alloc root field_t failed.\n");
        err = -ENOMEM;
        goto out;
    }

    if (schelen > 0) {
        if (schemas[0].pid != FLD_MAX_PID) {
            gingko_err(su, "The first have to be l1field.\n");
            err = -EBADSCHEMA;
            goto out_free;
        }
    }
    
    for (i = 0; i < schelen; i++) {
        /* construct a schema tree and verify it */
        if (schemas[i].pid == FLD_MAX_PID) {
            /* it is a l1 field, add to root node */
            err = __add_to_field_t(root, schemas[i]);
            if (err) {
                gingko_err(su, "add to field_t failed w/ %s(%d)\n",
                           gingko_strerror(err), err);
                goto out_free;
            }
        } else {
            /* it is non-l1 field, add to parent node */
            err = __find_add_to_field_t(root, schemas[i]);
            if (err) {
                gingko_err(su, "find add to field_t failed w/ %s(%d)\n",
                           gingko_strerror(err), err);
                goto out_free;
            }
        }
    }

    /* trav tree and print info */
    __pre_trav_schemas(root, __print_schema, NULL);
    /* FIXME: insert into the field cache */
    __pre_trav_schemas(root, __build_fc, gs);
    
    /* insert this subtree to root tree */
    err = __add_to_root(gs->root, root);
    if (err) {
        gingko_err(su, "__add_to_root for subtree failed w/ %s(%d)\n",
                   gingko_strerror(err), err);
        goto out_free;
    }

out:
    return err;
out_free:
    /* free the tree */
    __post_trav_schemas(root, __free_schema, NULL);
    goto out;
}

int build_lineheaders(struct gingko_su *gs, struct line *line, long lid,
                      u64 coff)
{
    int err = 0, i, j, k = 1;

    /* calculate l1flds and split by dfiles */
    int l1fld[gs->sm.dfnr];

    memset(l1fld, 0, sizeof(l1fld));
    for (i = 0; i < gs->sm.dfnr; i++) {
        l1fld[i] = gs->files[i].dfh->md.l1fldnr;
    }

    /* for each dfile, re-calculate l1 field offset */
    for (i = 0; i < gs->sm.dfnr; i++) {
        /* check if the 0th offset is ZERO. if not, do offset shift left */
        u64 zoff = 0;
        
        if (l1fld[i] > 0)
            zoff = line->lh[1].offset;
        for (j = 0; j < l1fld[i]; j++) {
            /* NOTE-XXX: to support user line reusing, we should automatically
             * dec the offset */
            line->lh[j + 1].offset += (coff - zoff);
        }
    }

    /* set the fldid by parse schemas */
    for (i = 0; i < gs->sm.dfnr; i++) {
        for (j = 0; j < gs->files[i].dfh->md.fldnr; j++) {
            if (gs->files[i].dfh->schemas[j].pid == FLD_MAX_PID) {
                line->lh[k++].l1fld = gs->files[i].dfh->schemas[j].id;
            }
        }
    }
    
    return err;
}

void dump_lineheader(long lid, struct line *line)
{
    struct lineheader0 *lh0;
    int i;

    if (!line->lh) {
        gingko_info(su, "Invalid line header for line %p\n", line);
        return;
    }
    lh0 = (struct lineheader0 *)&line->lh[0];
    gingko_info(su, "Line %ld %p\t LH0: %d l1flds llen %u\n", 
                lid, line, lh0->len, lh0->llen);
    for (i = 0; i < lh0->len; i++) {
        gingko_info(su, "Line %ld %p\t LH%d: l1fld %d poffset %u\n",
                    lid, line, i + 1, 
                    line->lh[i + 1].l1fld, line->lh[i + 1].offset);
    }
}

int linepack_primitive(struct line *line, void *data, int dlen)
{
    void *tmp;
    int err = 0;
    
    tmp = xrealloc(line->data, line->len + dlen);
    if (!tmp) {
        gingko_err(su, "xrealloc() line data failed, no memory.\n");
        err = -ENOMEM;
        goto out;
    }
    line->data = tmp;

    /* data is in *data, not in the pointer */ 
    memcpy(line->data + line->len, &data, dlen);
    gingko_debug(su, "PACK data %ld len %d off %d\n", (u64)data, dlen,
                 line->len);
    line->len += dlen;
    
out:
    return err;
}

static inline int __get_headlen(int dlen)
{
    if (unlikely(dlen < 0)) {
        gingko_err(su, "Negative data length %d", dlen);
        GINGKO_BUGON("Invalid data length.");
    } else if (dlen < SU_LEN_L1B_MAX) {
        return sizeof(struct l1b);
    } else if (dlen < SU_LEN_L2B_MAX) {
        return sizeof(struct l2b);
    } else if (dlen < SU_LEN_L3B_MAX) {
        return sizeof(struct l3b);
    } else if (dlen < SU_LEN_L4B_MAX) {
        return sizeof(struct l4b);
    } else {
        gingko_err(su, "Data length %d is too large", dlen);
        GINGKO_BUGON("Invalid data length.");
    }

    return 0;
}

static inline void SET_VARHEAD(void *head, int hlen, int dlen)
{
    switch (hlen) {
    case 1:
    {
        struct l1b *p = head;
        p->flag = SU_TYPE_L1B;
        p->len = dlen;
        break;
    }
    case 2:
    {
        struct l2b *p = head;
        p->flag = SU_TYPE_L2B;
        p->len = dlen;
        break;
    }
    case 3:
    {
        struct l3b *p = head;
        p->flag = SU_TYPE_L3B;
        p->len = dlen & 0x3fff;
        p->lenhb = (dlen >> 14);
        break;
    }
    case 4:
    {
        struct l4b *p = head;
        p->flag = SU_TYPE_L4B;
        p->len = dlen;
        break;
    }
    default:
        gingko_err(su, "Invalid head length %d\n", hlen);
    }
}

int linepack_string(struct line *line, void *data, int dlen)
{
    void *tmp;
    int err = 0, hlen;

    hlen = __get_headlen(dlen);
    tmp = xrealloc(line->data, line->len + dlen + hlen);
    if (!tmp) {
        goto out;
    }
    line->data = tmp;

    SET_VARHEAD(line->data + line->len, hlen, dlen);
    memcpy(line->data + line->len + hlen, data, dlen);
    gingko_debug(su, "PACK data %.*s len %d off %d\n", dlen, 
                 (char *)data, dlen, line->len);
    
    line->len += hlen + dlen;
    
out:
    return err;
}

int linepack_complex(struct line *line, struct field_2pack *fld)
{
    int err = 0, hlen = 0, i;
    void *pdata = NULL;
    
    hlen = __get_headlen(fld->cidnr);
    
    pdata = xrealloc(line->data, line->len + hlen);
    if (!pdata) {
        gingko_err(su, "xmalloc(%d) array data region failed, "
                   "no memory.\n", hlen);
        err = -ENOMEM;
        goto out;
    }
    line->data = pdata;

    /* pack complex head */
    switch (fld->type) {
    case GINGKO_ARRAY:
    case GINGKO_STRUCT:
    case GINGKO_MAP:
        SET_VARHEAD(line->data + line->len, hlen, fld->cidnr);
        break;
    default:
        gingko_err(su, "Invalid complex data type %d\n",
                   fld->type);
        err = -EINVAL;
        goto out;
    }
    line->len += hlen;
    gingko_debug(su, "PACK vhead len %dB => %d\n", hlen, fld->cidnr);

    /* pack child fields of complex field */
    for (i = 0; i < fld->cidnr; i++) {
        switch ((fld->clds[i])->type) {
        case GINGKO_INT8:
            err = linepack_primitive(line, (fld->clds[i])->data, 1);
            break;
        case GINGKO_INT16:
            err = linepack_primitive(line, (fld->clds[i])->data, 2);
            break;
        case GINGKO_INT32:
        case GINGKO_FLOAT:
            err = linepack_primitive(line, (fld->clds[i])->data, 4);
            break;
        case GINGKO_INT64:
        case GINGKO_DOUBLE:
            err = linepack_primitive(line, (fld->clds[i])->data, 8);
            break;
        case GINGKO_STRING:
        case GINGKO_BYTES:
            err = linepack_string(line, (fld->clds[i])->data,
                                    (fld->clds[i])->dlen);
            break;
        case GINGKO_ARRAY:
        case GINGKO_STRUCT:
        case GINGKO_MAP:
            err = linepack_complex(line, (fld->clds[i]));
            break;
        default:
            gingko_err(api, "Invalid field type=%d\n", (fld->clds[i])->type);
            err = -EINVAL;
            goto out;
        }
    }

out:
    return err;
}

int lineunpack_primitive(void *idata, struct field_t *f, struct field_g *fg)
{
    switch (f->fld.type) {
    case GINGKO_INT8:
        fg->dlen = 1;
        break;
    case GINGKO_INT16:
        fg->dlen = 2;
        break;
    case GINGKO_INT32:
    case GINGKO_FLOAT:
        fg->dlen = 4;
        break;
    case GINGKO_INT64:
    case GINGKO_DOUBLE:
        fg->dlen = 8;
        break;
    default:
        return -EINVAL;
    }
    memcpy(&fg->content, idata, fg->dlen);

    return 0;
}

static inline int GET_VARHEAD(void *head, int *dlen)
{
    struct l1b *t = head;

    switch (t->flag) {
    case SU_TYPE_L1B:
    {
        struct l1b *p = t;
        *dlen = p->len;
        return 1;
    }
    case SU_TYPE_L2B:
    {
        struct l2b *p = head;
        *dlen = p->len;
        return 2;
    }
    case SU_TYPE_L3B:
    {
        struct l3b *p = head;
        *dlen = (p->lenhb << 14) + p->len;
        return 3;
    }
    case SU_TYPE_L4B:
    {
        struct l4b *p = head;
        *dlen = p->len;
        return 4;
    }
    default:
        return -1;
    }
}

static int __get_datalen(struct field_t *f, void *data)
{
    /* iterate the field array to find all data length, if it is a complex
     * type, recursive calculate */

    switch (f->fld.type) {
    case GINGKO_INT8:
        return 1;
    case GINGKO_INT16:
        return 2;
    case GINGKO_INT32:
    case GINGKO_FLOAT:
        return 4;
    case GINGKO_INT64:
    case GINGKO_DOUBLE:
        return 8;
    case GINGKO_STRING:
    case GINGKO_BYTES:
    {
        int dlen, hlen = GET_VARHEAD(data, &dlen);
        
        return dlen + hlen;
    }
    case GINGKO_ARRAY:
    case GINGKO_STRUCT:
    case GINGKO_MAP:
    {
        int i;
        int dlen = 0;

        for (i = 0; i < f->cnr; i++) {
            dlen += __get_datalen(f->cld[i], data + dlen);
        }

        return dlen;
    }
    default:
        gingko_err(su, "Invalid data type=%d\n", f->fld.type);
        GINGKO_BUGON("Invalid data type.");
    }

    return 0;
}

int lineunpack_complex(void *idata, int dlen, struct field_t *f, 
                       struct field_g *fg)
{
    int err = 0, hlen;

    hlen = GET_VARHEAD(idata, &fg->dlen);
    if (hlen < 0) {
        gingko_err(su, "Get VarHead for unpack complex field failed.\n");
        err = -EINTERNAL;
        goto out;
    }
    
    switch (f->fld.type) {
    case GINGKO_ARRAY:
    case GINGKO_STRUCT:
    case GINGKO_MAP:
    {
        /* copy all the data by dlen */
        if (dlen >= hlen) {
            fg->dlen = dlen - hlen;
            fg->content = xmalloc(fg->dlen);
            if (!fg->content) {
                gingko_err(su, "xmalloc() string/bytes buffer failed.\n");
                err = -ENOMEM;
                goto out;
            }
            memcpy(fg->content, idata + hlen, fg->dlen);
        } else {
            /* no lineheader */
            err = -ENOTIMP;
            goto out;
        }
        break;
    }
    case GINGKO_STRING:
    case GINGKO_BYTES:
    {
        /* copy all the data by dlen */
        if (dlen >= hlen) {
            fg->content = xmalloc(fg->dlen);
            if (!fg->content) {
                gingko_err(su, "xmalloc() string/bytes buffer failed.\n");
                err = -ENOMEM;
                goto out;
            }
            memcpy(fg->content, idata + hlen, fg->dlen);
        } else {
            /* no lineheader */
            err = -ENOTIMP;
            goto out;
        }
        break;
    }
    default:
        err = -EINVAL;
        goto out;
    }
out:
    return err;
}

int lineunpack(void *idata, int dlen, struct field_t *f, struct field_g *fg)
{
    int err = 0;
    
    switch (f->fld.type) {
    case GINGKO_INT8:
    case GINGKO_INT16:
    case GINGKO_INT32:
    case GINGKO_FLOAT:
    case GINGKO_INT64:
    case GINGKO_DOUBLE:
        err = lineunpack_primitive(idata, f, fg);
        break;
    case GINGKO_STRING:
    case GINGKO_BYTES:
    case GINGKO_ARRAY:
    case GINGKO_STRUCT:
    case GINGKO_MAP:
        err = lineunpack_complex(idata, dlen, f, fg);
        break;
    default:
        err = -EINVAL;
        goto out;
    }
out:
    return err;
}

string_t lineparse_string(struct field_g *f)
{
    string_t s;

    if (f->type == GINGKO_STRING) {
        s.len = f->dlen;
        s.str = f->content;
    }
    
    return s;
}

void gingko_p_string(string_t s)
{
    gingko_info(su, "string_t %.*s\n", s.len, s.str);
}

int lineparse(struct field_g *f, struct field_t *this, struct field_t *tf)
{
    int err = 0;

    switch (f->type) {
    case GINGKO_INT8:
    {
        u8 t = *(u8 *)f->content;
        xfree(f->content);
        f->content = (void *)(u64)t;
        f->dlen = 1;
        break;
    }
    case GINGKO_INT16:
    {
        u16 t = *(u16 *)f->content;
        xfree(f->content);
        f->content = (void *)(u64)t;
        f->dlen = 2;
        break;
    }
    case GINGKO_INT32:
    case GINGKO_FLOAT:
    {
        u32 t = *(u32 *)f->content;
        xfree(f->content);
        f->content = (void *)(u64)t;
        f->dlen = 4;
        break;
    }
    case GINGKO_INT64:
    case GINGKO_DOUBLE:
    {
        u64 t = *(u64 *)f->content;
        xfree(f->content);
        f->content = (void *)t;
        f->dlen = 8;
        break;
    }
    case GINGKO_STRING:
    case GINGKO_BYTES:
    {
        int hlen = GET_VARHEAD(f->content, &f->dlen);
        memmove(f->content, f->content + hlen, f->dlen);
        break;
    }
    case GINGKO_MAP:
        /* FIXME: only works for one level embed */
    case GINGKO_STRUCT:
    {
        /* FIXME: only works for one level embed */
        int i;

        for (i = 0; i < this->cnr; i++) {
            if (this->cld[i]->fld.id == tf->fld.id) {
                /* ok, parse this field */
                f->id = tf->fld.id;
                f->pid = tf->fld.pid;
                f->type = tf->fld.type;
                xfree(f->name);
                f->name = strdup(tf->fld.name);
                f->dlen = __get_datalen(this->cld[i], f->content);
                break;
            } else {
                /* need to skip this data region */
                int skip = __get_datalen(this->cld[i], f->content);
                gingko_debug(su, "SKIP ID=%d len=%d\n", 
                             this->cld[i]->fld.id, skip);
                memmove(f->content, f->content + skip, f->dlen - skip);
            }
        }
        return lineparse(f, this, tf);
    }
    case GINGKO_ARRAY:
        /* just return the zeroth entry */
        f->id = tf->fld.id;
        f->pid = tf->fld.pid;
        f->type = tf->fld.type;
        xfree(f->name);
        f->name = strdup(tf->fld.name);
        return lineparse(f, this, tf);
    default:;
    }

    return err;
}
