/**
 * @file apteryxc.c
 * Client application for Apteryx.
 * Used for accessing the database from the command line.
 *
 * Copyright 2014, Allied Telesis Labs New Zealand, Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "internal.h"
#include "apteryx.h"
#include "apteryx.pb-c.h"

/* Debug enabled */
bool debug = false;

/* Run while true */
static bool running = true;

/* Trap signals to exit cleanly */
void
termination_handler (void)
{
    running = false;
}

/* Display application usage */
void
usage ()
{
    printf ("Usage: apteryx [-h] [-s|-g|-f|-t|-w|-p|-l] [<path>] [<value>]\n"
            "  -h   show this help\n"
            "  -d   debug\n"
            "  -s   set <path>=<value>\n"
            "  -g   get <path>\n"
            "  -f   find <path>\n"
            "  -t   traverse database from <path>\n"
            "  -w   watch changes to the path <path>\n"
            "  -p   provide <value> for <path>\n"
            "  -l   last change <path>\n");
    printf ("\n");
    printf ("  Internal settings\n");
    printf ("    %sdebug\n", APTERYX_SETTINGS);
    printf ("    %scounters\n", APTERYX_SETTINGS);
    printf ("    %swatchers\n", APTERYX_SETTINGS);
    printf ("    %sproviders\n", APTERYX_SETTINGS);
    printf ("    %scache\n", APTERYX_SETTINGS);
    printf ("\n");
}

static bool
watch_callback (const char *path, void *priv, const char *value)
{
    printf ("%s = %s\n", path, value);
    return true;
}

static char*
provide_callback (const char *path, void *priv)
{
    return strdup ((char *) priv);
}

/* Application entry point */
int
main (int argc, char **argv)
{
    APTERYX_MODE mode = -1;
    char *path = NULL;
    char *param = NULL;
    GList * _iter;
    int c;
    uint64_t value;

    /* Handle SIGTERM/SIGINT/SIGPIPE gracefully */
    signal (SIGTERM, (__sighandler_t) termination_handler);
    signal (SIGINT, (__sighandler_t) termination_handler);

    /* Parse options */
    while ((c = getopt (argc, argv, "hdsgftwp")) != -1)
    {
        switch (c)
        {
        case 'd':
            debug = true;
            break;
        case 's':
            mode = MODE_SET;
            break;
        case 'g':
            mode = MODE_GET;
            break;
        case 'f':
            mode = MODE_FIND;
            break;
        case 't':
            mode = MODE_TRAVERSE;
            break;
        case 'w':
            mode = MODE_WATCH;
            break;
        case 'p':
            mode = MODE_PROVIDE;
            break;
        case 'l':
            mode = MODE_TIMESTAMP;
            break;
        case '?':
        case 'h':
        default:
            usage ();
            return 0;
        }
    }

    for (c = optind; c < argc; c++)
    {
        if (path == NULL)
            path = argv[c];
        else if (param == NULL)
            param = argv[c];
        else
        {
            usage ();
            return 0;
        }
    }

    switch (mode)
    {
    case MODE_GET:
        if (!path || param)
        {
            usage ();
            return 0;
        }
        apteryx_init (debug);
        if ((param = apteryx_get (path)))
        {
            printf ("%s\n", param);
            free (param);
        }
        else
            printf ("Not found\n");
        apteryx_shutdown ();
        break;
    case MODE_SET:
        if (!path)
        {
            usage ();
            return 0;
        }
        apteryx_init (debug);
        if (!apteryx_set (path, param))
            printf ("Failed\n");
        apteryx_shutdown ();
        break;
    case MODE_FIND:
        if (!path || param)
        {
            usage ();
            return 0;
        }
        apteryx_init (debug);
        GList *paths = apteryx_search (path);
        for (_iter = paths; _iter; _iter = _iter->next)
            printf ("  %s\n", (char *) _iter->data);
        g_list_free_full (paths, free);
        apteryx_shutdown ();
        break;
    case MODE_TRAVERSE:
        if (param)
        {
            usage ();
            return 0;
        }
        if (!path)
        {
            path = "";
        }
        apteryx_init (debug);
        apteryx_dump (path, stdout);
        apteryx_shutdown ();
        break;
    case MODE_WATCH:
        if (param)
        {
            usage ();
            return 0;
        }
        if (!path)
        {
            path = "/";
        }
        apteryx_init (debug);
        apteryx_watch (path, watch_callback, NULL);
        while (running)
            pause ();
        apteryx_watch (path, NULL, NULL);
        apteryx_shutdown ();
        break;
    case MODE_PROVIDE:
        if (!path || !param)
        {
            usage ();
            return 0;
        }
        apteryx_init (debug);
        apteryx_provide (path, provide_callback, param);
        while (running)
            pause ();
        apteryx_provide (path, NULL, NULL);
        apteryx_shutdown ();
        break;
    case MODE_TIMESTAMP:
        if (!path || param)
        {
            usage ();
            return 0;
        }
        apteryx_init (debug);
        value = apteryx_get_timestamp (path);
        printf ("%"PRIu64"\n", value);
        apteryx_shutdown ();
        break;
    default:
        usage ();
        return 0;
    }

    return 0;
}
