/*
 * Imperium
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * ImpStr.h - bounded string helpers used throughout Imperium.
 *
 * Use these instead of strcpy()/strcat()/sprintf() so a long string can
 * never run past the end of a buffer - it is cut short instead.
 */

#ifndef IMP_STR_H
#define IMP_STR_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>

/*
 * IMP_BUFSIZE(buf) - the size of a character array, for passing to
 *      snprintf()/impStrCopy()/impStrCat(). With GCC or clang this
 *      refuses to compile if "buf" is a pointer rather than an array,
 *      since sizeof() of a pointer would silently give the wrong size.
 */
#if defined(__GNUC__)
#define IMP_BUFSIZE(a) (sizeof(a) + 0 * sizeof(struct { \
    int imp_bufsize_needs_an_array : \
        (__builtin_types_compatible_p(__typeof__(a), \
            __typeof__(&(a)[0])) ? -1 : 1); }))
#else
#define IMP_BUFSIZE(a) sizeof(a)
#endif

/*
 * impStrCopy - copy src into dst (which holds "size" bytes), always
 *      leaving dst NUL terminated. Returns the length of src, so a
 *      result >= size means the copy was cut short.
 */
static inline size_t impStrCopy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);

    if (size > 0)
    {
        size_t n = (len >= size) ? size - 1 : len;

        memmove(dst, src, n);
        dst[n] = '\0';
    }
    return len;
}

/*
 * impStrCat - append src onto the string in dst (which holds "size"
 *      bytes), always leaving dst NUL terminated. Returns the length the
 *      combined string would have had.
 */
static inline size_t impStrCat(char *dst, const char *src, size_t size)
{
    size_t dLen = strnlen(dst, size);

    if (dLen == size)
    {
        /* dst was not terminated - don't make things worse */
        return size + strlen(src);
    }
    return dLen + impStrCopy(dst + dLen, src, size - dLen);
}

#endif /* IMP_STR_H */
