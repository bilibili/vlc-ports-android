/*****************************************************************************
 * libvlcjni_fflog.inc.c
 *****************************************************************************
 * Copyright © 2013 Zhang Rui <bbcallen@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

static void ffmpeg_debug_log(void* ptr, int level, const char* fmt, va_list vl)
{
    bool *verbose = &verbosity;

    int prio = ANDROID_LOG_DEBUG;
    if      (level <= AV_LOG_ERROR)     prio = ANDROID_LOG_ERROR;
    else if (level <= AV_LOG_WARNING)   prio = ANDROID_LOG_WARN;
    else if (level <= AV_LOG_INFO)      prio = ANDROID_LOG_INFO;
    else if (level <= AV_LOG_VERBOSE)   prio = ANDROID_LOG_DEBUG;
    else                                prio = ANDROID_LOG_DEBUG;

    if (!*verbose)
        return;

    static int print_prefix = 1;
    static int count;
    static char prev[1024];
    char line[1024];
    static int is_atty;
    AVClass* avc = ptr ? *(AVClass **) ptr : NULL;
    line[0] = 0;
    if (print_prefix && avc) {
        if (avc->parent_log_context_offset) {
            AVClass** parent = *(AVClass ***) (((uint8_t *) ptr) +
                                               avc->parent_log_context_offset);
            if (parent && *parent) {
                snprintf(line, sizeof(line), "[%s @ %p] ",
                         (*parent)->item_name(parent), parent);
            }
        }
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "[%s @ %p] ",
                 avc->item_name(ptr), ptr);
    }

    vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    print_prefix = strlen(line) && line[strlen(line) - 1] == '\n';

    if (print_prefix && !strncmp(line, prev, sizeof line)) {
        count++;
        if (is_atty == 1)
            fprintf(stderr, "    Last message repeated %d times\r", count);
        return;
    }
    if (count > 0) {
        fprintf(stderr, "    Last message repeated %d times\n", count);
        count = 0;
    }
    __android_log_write(prio, "VLC-FF", line);
    av_strlcpy(prev, line, sizeof line);
}
