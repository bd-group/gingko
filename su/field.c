/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-11-17 20:36:47 macan>
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
    gs->root = root;

out:
    return err;
out_free:
    /* free the tree */
    __post_trav_schemas(root, __free_schema);
    goto out;
}

