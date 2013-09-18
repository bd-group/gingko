/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-09-18 14:15:43 macan>
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

#ifndef __GINGKO_H__
#define __GINGKO_H__

#include "gingko_u.h"

#include "tracing.h"
#include "memory.h"
#include "xlock.h"
#include "gingko_common.h"
#include "gingko_const.h"
#include "xhash.h"

/* APIs */
#include "hash.c"

#define __UNUSED__ __attribute__((unused))

#define BITMAP_ROUNDUP(x) (((x + 1) + XTABLE_BITMAP_SIZE - 1) & \
                           ~(XTABLE_BITMAP_SIZE - 1))
#define BITMAP_ROUNDDOWN(x) ((x) & (~((XTABLE_BITMAP_SIZE) - 1)))

#define PAGE_ROUNDUP(x, p) ((((size_t) (x)) + (p) - 1) & ~((p) - 1))

#endif
