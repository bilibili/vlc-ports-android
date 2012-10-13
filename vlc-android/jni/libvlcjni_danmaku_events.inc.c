/*****************************************************************************
 * libvlcjni_danmaku_events.inc.c
 *****************************************************************************
 * Copyright (C) 2012 Rui Zhang
 *
 * Author: Rui Zhang <bbcallen _AT_ gmail _DOT_ com>
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

#ifndef LIBVLCJNI_DANMAKU_INCLUDE_INC_C
#define LIBVLCJNI_DANMAKU_INCLUDE_INC_C

/* Pointer to the Java virtual machine
 * Note: It's okay to use a static variable for the VM pointer since there
 * can only be one instance of this shared library in a single VM
 */
extern JavaVM *myVm;

static const libvlc_event_type_t md_events[] = {
//        libvlc_MediaMetaChanged,
//        libvlc_MediaSubItemAdded,
        libvlc_MediaDurationChanged, //
        libvlc_MediaParsedChanged, //
//        libvlc_MediaFreed,
        libvlc_MediaStateChanged, //
        };

static const libvlc_event_type_t mp_events[] = {
//        libvlc_MediaPlayerMediaChanged,
//        libvlc_MediaPlayerNothingSpecial,
        libvlc_MediaPlayerOpening, //
        libvlc_MediaPlayerBuffering, //
        libvlc_MediaPlayerPlaying, //
        libvlc_MediaPlayerPaused, //
        libvlc_MediaPlayerStopped, //
//        libvlc_MediaPlayerForward,
//        libvlc_MediaPlayerBackward,
        libvlc_MediaPlayerEndReached, //
        libvlc_MediaPlayerEncounteredError, //
//        libvlc_MediaPlayerTimeChanged,
//        libvlc_MediaPlayerPositionChanged,
        libvlc_MediaPlayerSeekableChanged, //
        libvlc_MediaPlayerPausableChanged, //
//        libvlc_MediaPlayerTitleChanged,
//        libvlc_MediaPlayerSnapshotTaken,
        libvlc_MediaPlayerLengthChanged, //
//        libvlc_MediaPlayerVout,
        libvlc_MediaPlayerBufferingTotal, //
        libvlc_MediaPlayerModuleChanged, //
        };

#endif//LIBVLCJNI_DANMAKU_INCLUDE_INC_C
