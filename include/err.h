/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2014-01-12 01:41:56 macan>
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

#ifndef __ERR_H__
#define __ERR_H__

#define MAX_ERRNO	4095

#define EBADSU          1025
#define EINCREATE       1026
#define ERDONLY         1027
#define EINTERNAL       1028
#define ECONFLICT       1029
#define ESTATUS         1030
#define ENODATAR        1031
#define EBADSCHEMA      1032
#define ENOTIMP         1033
#define ENEWPAGE        1034

#define IS_ERR_VALUE(x) unlikely((x) >= (unsigned long)-MAX_ERRNO)

static inline void *ERR_PTR(long error)
{
	return (void *) error;
}

static inline long PTR_ERR(const void *ptr)
{
	return (long) ptr;
}

static inline long IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((unsigned long)ptr);
}

/**
 * ERR_CAST - Explicitly cast an error-valued pointer to another pointer type
 * @ptr: The pointer to cast.
 *
 * Explicitly cast an error-valued pointer to another pointer type in such a
 * way as to make it clear that's what's going on.
 */
static inline void *ERR_CAST(const void *ptr)
{
	/* cast away the const */
	return (void *) ptr;
}

static char __UNUSED__ *gingko_strerror(int err)
{
    if (err < 0) {
        switch (-err) {
        case EBADSU:
            return "Bad Store Unit";
        case EINCREATE:
            return "In IN CREATE State";
        case ERDONLY:
            return "In Read Only State";
        case EINTERNAL:
            return "Internal Error";
        case ECONFLICT:
            return "Conflict With Other";
        case ESTATUS:
            return "Object Status Error";
        case ENODATAR:
            return "No Such Data Record";
        case EBADSCHEMA:
            return "Bad Schema";
        case ENOTIMP:
            return "Not Implement Yet";
        default:
            err = -err;
            goto other;
        }
    } else {
    other:
        return strerror(err);
    }

    return "Unknown Error";
}

#endif
