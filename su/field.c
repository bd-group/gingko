/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-21 11:08:43 macan>
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

static struct field_t *__find_field_t(struct field_t *root, int id)
{
    struct field_t *target = NULL;
    int i = 0;
    
    if (root->type == FIELD_TYPE_ROOT) {
        /* iterate on root type */
        for (i = 0; i < root->cnr; i++) {
            target = __find_field_t(root->cld[i], id);
            if (target) {
                goto out;
            }
        }
    } else {
        if (root->fld.id == id) {
            target = root;
            goto out;
        }
    }

out:
    return target;
}

void __print_schema(void *arg)
{
    struct field_t *f = (struct field_t *)arg;

    gingko_info(su, "SCHEMA: NTYPE=%d ID=%d PID=%5d FTYPE=%2d\n",
                f->type, f->fld.id, f->fld.pid, f->fld.type);
}

void __free_schema(void *arg)
{
    __free_field_t((struct field_t *)arg);
}

void __pre_trav_schemas(struct field_t *root, tfunc cb)
{
    int i;
    
    cb(root);
    for (i = 0; i < root->cnr; i++) {
        __pre_trav_schemas(root->cld[i], cb);
    }
}

void __post_trav_schemas(struct field_t *root, tfunc cb)
{
    int i;

    for (i = 0; i < root->cnr; i++) {
        __post_trav_schemas(root->cld[i], cb);
    }
    cb(root);
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
    __pre_trav_schemas(root, __print_schema);
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
    __post_trav_schemas(root, __free_schema);
    goto out;
}

int build_lineheaders(struct gingko_su *gs, struct line *line, long lid)
{
    int err = 0;

    /* calculate l1flds and split by dfiles */
    int l1fld[gs->sm.dfnr];

    memset(l1fld, 0, sizeof(l1fld));
    
    
    return err;
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
    gingko_debug(su, "PACK data %ld len %d\n", (u64)data, dlen);
    
out:
    return err;
}

static int __get_headlen(int dlen)
{
    if (dlen < 0) {
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
    
    line->len += hlen + dlen;
    
out:
    return err;
}

static int __UNUSED__ __get_datalen(struct field_2pack *fld)
{
    /* iterate the field array to find all data length, if it is a complex
     * type, recursive calculate */

    switch (fld->type) {
    case GINGKO_INT8:
        return 1;
    case GINGKO_INT16:
        return 2;
    case GINGKO_INT32:
        return 4;
    case GINGKO_INT64:
        return 8;
    case GINGKO_FLOAT:
        return 4;
    case GINGKO_DOUBLE:
        return 8;
    case GINGKO_STRING:
    case GINGKO_BYTES:
        return fld->dlen + __get_headlen(fld->dlen);
    case GINGKO_ARRAY:
    case GINGKO_STRUCT:
    case GINGKO_MAP:
    {
        int i;
        int dlen = 0;

        for (i = 0; i < fld->cidnr; i++) {
            dlen += __get_datalen(fld->clds[i]);
        }

        return dlen;
        break;
    }
    default:
        gingko_err(su, "Invalid data type=%d\n", fld->type);
        GINGKO_BUGON("Invalid data type.");
    }

    return 0;
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


