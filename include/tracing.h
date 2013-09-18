/**
 * Copyright (c) 2009 Ma Can <ml.macana@gmail.com>
 *                           <macan@ncic.ac.cn>
 *
 * Armed with EMACS.
 * Time-stamp: <2013-09-18 14:13:50 macan>
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

#ifndef __TRACING_H__
#define __TRACING_H__

#include <sys/timeb.h>
#include <time.h>

/* gingko tracing flags */
#define GINGKO_INFO       0x80000000
#define GINGKO_WARN       0x40000000
#define GINGKO_ERR        0x20000000
#define GINGKO_PLAIN      0x10000000

#define GINGKO_ENTRY      0x00000008
#define GINGKO_VERBOSE    0x00000004 /* more infos than debug mode */
#define GINGKO_PRECISE    0x00000002
#define GINGKO_DEBUG      0x00000001

#define GINGKO_DEFAULT_LEVEL      0xf0000000
#define GINGKO_DEBUG_ALL          0x0000000f

#ifdef __KERNEL__
#define PRINTK printk
#define FFLUSH
#else  /* !__KERNEL__ */
#define PRINTK printf
#define FPRINTK fprintf
#define FFLUSH(f) fflush(f)
#define KERN_INFO       "[INFO] "
#define KERN_ERR        "[ERR ] "
#define KERN_WARNING    "[WARN] "
#define KERN_DEBUG      "[DBG ] "
#define KERN_VERB       "[VERB] "
#define KERN_PLAIN      ""
#endif

#ifdef GINGKO_TRACING
#define gingko_tracing(mask, flag, lvl, f, a...) do {                     \
        if (unlikely(mask & flag)) {                                    \
            struct timeval __cur;                                       \
            struct tm __tmp;                                            \
            char __ct[32];                                              \
                                                                        \
            gettimeofday(&__cur, NULL);                                 \
            if (!localtime_r(&__cur.tv_sec, &__tmp)) {                  \
                PRINTK(KERN_ERR f, ## a);                               \
                FFLUSH(stdout);                                         \
                break;                                                  \
            }                                                           \
            strftime(__ct, 64, "%G-%m-%d %H:%M:%S", &__tmp);            \
            if (mask & GINGKO_PRECISE) {                                  \
                PRINTK("%s.%03ld " lvl "GINGKO (%16s, %5d): %s[%lx]: " f, \
                       __ct, (long)(__cur.tv_usec / 1000),              \
                       __FILE__, __LINE__, __func__,                    \
                       pthread_self(), ## a);                           \
                FFLUSH(stdout);                                         \
            } else if (mask & GINGKO_PLAIN) {                             \
                PRINTK("" lvl f, ## a);                                 \
                FFLUSH(stdout);                                         \
            } else {                                                    \
                PRINTK("%s.%03ld " lvl f,                               \
                       __ct, (long)(__cur.tv_usec / 1000), ## a);       \
                FFLUSH(stdout);                                         \
            }                                                           \
        }                                                               \
    } while (0)
#else
#define gingko_tracing(mask, flag, lvl, f, a...)
#endif  /* !GINGKO_TRACING */

#define IS_GINGKO_DEBUG(module) ({                            \
            int ret;                                        \
            if (gingko_##module##_tracing_flags & GINGKO_DEBUG) \
                ret = 1;                                    \
            else                                            \
                ret = 0;                                    \
            ret;                                            \
        })

#define IS_GINGKO_VERBOSE(module) ({                              \
            int ret;                                            \
            if (gingko_##module##_tracing_flags & GINGKO_VERBOSE)   \
                ret = 1;                                        \
            else                                                \
                ret = 0;                                        \
            ret;                                                \
        })

#define gingko_info(module, f, a...)                          \
    gingko_tracing(GINGKO_INFO, gingko_##module##_tracing_flags,  \
                 KERN_INFO, f, ## a)

#define gingko_plain(module, f, a...)                         \
    gingko_tracing(GINGKO_PLAIN, gingko_##module##_tracing_flags, \
                 KERN_PLAIN, f, ## a)

#define gingko_verbose(module, f, a...)           \
    gingko_tracing((GINGKO_VERBOSE | GINGKO_PRECISE), \
                 gingko_##module##_tracing_flags, \
                 KERN_VERB, f, ## a)

#define gingko_debug(module, f, a...)             \
    gingko_tracing((GINGKO_DEBUG | GINGKO_PRECISE),   \
                 gingko_##module##_tracing_flags, \
                 KERN_DEBUG, f, ## a)

#define gingko_entry(module, f, a...)             \
    gingko_tracing((GINGKO_ENTRY | GINGKO_PRECISE),   \
                 gingko_##module##_tracing_flags, \
                 KERN_DEBUG, "entry: " f, ## a)

#define gingko_exit(module, f, a...)              \
    gingko_tracing((GINGKO_ENTRY | GINGKO_PRECISE),   \
                 gingko_##module##_tracing_flags, \
                 KERN_DEBUG, "exit: " f, ## a)

#define gingko_warning(module, f, a...)           \
    gingko_tracing((GINGKO_WARN | GINGKO_PRECISE),    \
                 gingko_##module##_tracing_flags, \
                 KERN_WARNING, f, ##a)

#define gingko_err(module, f, a...)               \
    gingko_tracing((GINGKO_ERR | GINGKO_PRECISE),     \
                 gingko_##module##_tracing_flags, \
                 KERN_ERR, f, ##a)

#define SET_TRACING_FLAG(module, flag) do {     \
        gingko_##module##_tracing_flags |= flag;  \
    } while (0)
#define CLR_TRACING_FLAG(module, flag) do {     \
        gingko_##module##_tracing_flags &= ~flag; \
    } while (0)

#define TRACING_FLAG(name, v) u32 gingko_##name##_tracing_flags = v
#define TRACING_FLAG_DEF(name) extern u32 gingko_##name##_tracing_flags

#ifdef __KERNEL__
#define ASSERT(i, m) BUG_ON(!(i))
#else  /* !__KERNEL__ */
#define ASSERT(i, m) do {                               \
        if (!(i)) {                                     \
            gingko_err(m, "Assertion " #i " failed!\n");  \
            exit(-EINVAL);                              \
        }                                               \
    } while (0)
#endif

#define GINGKO_VV PRINTK
/* Use GINGKO_BUG() to get the SIGSEGV signal to debug in the GDB */
#define GINGKO_BUG() do {                         \
        GINGKO_VV(KERN_PLAIN "GINGKO BUG :(\n");    \
        (*((int *)0) = 1);                      \
    } while (0)
#define GINGKO_BUGON(str) do {                        \
        GINGKO_VV(KERN_PLAIN "Bug on '" #str "'\n");  \
        GINGKO_BUG();                                 \
    } while (0)

#endif  /* !__TRACING_H__ */
