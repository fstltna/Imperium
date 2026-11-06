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
 * ImpFifo.h - safely create the named pipe a client uses to hear back
 *      from the server.
 *
 * This replaces the old tempnam() + mknod() pair, which picked a name and
 * created it in two steps (so someone else could grab the name first).
 * mkfifo() either creates a brand new pipe or fails, so we just pick a
 * name and retry if it is already taken.
 */

#ifndef IMP_FIFO_H
#define IMP_FIFO_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * impMakeFifo - create a new named pipe in "dir" whose name starts with
 *      "prefix". Returns the full path in memory from malloc() (free it
 *      when done), or NULL with errno set on failure.
 */
static char *impMakeFifo(const char *dir, const char *prefix, mode_t mode)
{
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static unsigned int counter = 0;
    size_t size;
    char *path;
    int tries, i;
    unsigned long seed;

    size = strlen(dir) + strlen(prefix) + 16;
    if ((path = malloc(size)) == NULL)
    {
        return NULL;
    }
    seed = ((unsigned long) getpid() << 16) ^ (unsigned long) time(NULL);
    for (tries = 0; tries < 100; tries++)
    {
        char suffix[9];

        counter++;
        seed = seed * 1103515245UL + 12345UL + counter;
        for (i = 0; i < 8; i++)
        {
            suffix[i] = chars[(seed >> (i * 4 + 3)) % (sizeof(chars) - 1)];
        }
        suffix[8] = '\0';
        snprintf(path, size, "%s/%s%s", dir, prefix, suffix);
        if (mkfifo(path, mode) == 0)
        {
            /* mkfifo() is limited by the umask, so set the real mode */
            if (chmod(path, mode) != 0)
            {
                int saveErr = errno;

                (void) unlink(path);
                free(path);
                errno = saveErr;
                return NULL;
            }
            return path;
        }
        if (errno != EEXIST)
        {
            break;
        }
    }
    free(path);
    return NULL;
}

#endif /* IMP_FIFO_H */
