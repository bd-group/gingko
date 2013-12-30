/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-12-27 15:44:46 macan>
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

void dump_pageindex(struct pageindex *pi)
{
    gingko_info(index, "PI: startline %u linenr %u\n",
                pi->startline, pi->linenr);
}

int build_pageindex(struct page *p, struct line *line, long lid,
                    struct gingko_su *gs)
{
    void *tmp, *lh = NULL;
    int err = 0;

    if (!p->pi)
        return -EINTERNAL;

    p->pi->linenr++;

    /* FIXME: update fldstat array */

    /* FIXME: lineheaders are ready, save them now */
    lh = xmalloc((gs->files[p->dfid].dfh->md.l1fldnr + 1) * 
                  sizeof(struct lineheader));
    if (!lh) {
        gingko_err(index, "xmalloc() lineheader array entry failed, "
                   "no memory.\n");
        goto out;
    }

    tmp = xrealloc(p->pi->lharray, p->pi->linenr * sizeof(struct lineheader *));
    if (!tmp) {
        gingko_err(index, "xrealloc() lineheaders array failed, no memory.\n");
        p->pi->linenr--;
        xfree(lh);
        goto out;
    }
    p->pi->lharray = tmp;
    memcpy(lh, line->lh, sizeof(struct lineheader) *
           (gs->files[p->dfid].dfh->md.l1fldnr + 1));
    p->pi->lharray[p->pi->linenr - 1] = lh;

    if (p->pi->linenr == 1)
        p->pi->startline = lid;

out:
    return err;
}
