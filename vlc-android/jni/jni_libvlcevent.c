/*****************************************************************************
 * jni_libvlcevent.c
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

#include "jni_libvlcevent.h"

#define BUNDLE_MediaPlayerBuffering_NEW_CACHE "new_cache"
#define BUNDLE_MediaPlayerSeekableChanged_NEW_SEEKABLE "new_seekable"
#define BUNDLE_MediaPlayerPausableChanged_NEW_PAUSABLE "new_pausable"
#define BUNDLE_MediaPlayerBufferingTotal_NEW_CACHE_TOTAL "new_cache_total"

#define BUNDLE_MediaPlayerModuleChanged_VideoDecoder "video_decoder"
#define BUNDLE_MediaPlayerModuleChanged_VideoDecoderImpl "video_decoder_impl"
#define BUNDLE_MediaPlayerModuleChanged_AudioDecoder "audio_decoder"
#define BUNDLE_MediaPlayerModuleChanged_AudioDecoderImpl "audio_decoder_impl"

JavaBundle *jbundle_from_event(JNIEnv *env, const libvlc_event_t *ev,
        JavaBundle *p_bundle) {
    if (NULL == jbundle_init(env, p_bundle))
        return NULL;

    switch (ev->type) {
    case libvlc_MediaPlayerBuffering: {
        float new_cache = ev->u.media_player_buffering.new_cache;
        jbundle_put_float(env, p_bundle, BUNDLE_MediaPlayerBuffering_NEW_CACHE,
                new_cache);
        break;
    }
    case libvlc_MediaPlayerSeekableChanged: {
        int new_seekable = ev->u.media_player_seekable_changed.new_seekable;
        jbundle_put_int(env, p_bundle,
                BUNDLE_MediaPlayerSeekableChanged_NEW_SEEKABLE, new_seekable);
        break;
    }
    case libvlc_MediaPlayerPausableChanged: {
        int new_pausable = ev->u.media_player_pausable_changed.new_pausable;
        jbundle_put_int(env, p_bundle,
                BUNDLE_MediaPlayerPausableChanged_NEW_PAUSABLE, new_pausable);
        break;
    }
    case libvlc_MediaPlayerBufferingTotal: {
        float new_cache_total =
                ev->u.media_player_buffering_total.new_cache_total;
        jbundle_put_float(env, p_bundle,
                BUNDLE_MediaPlayerBufferingTotal_NEW_CACHE_TOTAL,
                new_cache_total);
        break;
    }
    case libvlc_MediaPlayerModuleChanged: {
        const char* video_decoder =
                ev->u.media_player_module_changed.psz_video_decoder;
        const char* video_decoder_impl =
                ev->u.media_player_module_changed.psz_video_decoder_impl;
        const char* audio_decoder =
                ev->u.media_player_module_changed.psz_audio_decoder;
        const char* audio_decoder_impl =
                ev->u.media_player_module_changed.psz_audio_decoder_impl;

        jbundle_put_string(env, p_bundle,
                BUNDLE_MediaPlayerModuleChanged_VideoDecoder, video_decoder);
        jbundle_put_string(env, p_bundle,
                BUNDLE_MediaPlayerModuleChanged_VideoDecoderImpl,
                video_decoder_impl);
        jbundle_put_string(env, p_bundle,
                BUNDLE_MediaPlayerModuleChanged_AudioDecoder, audio_decoder);
        jbundle_put_string(env, p_bundle,
                BUNDLE_MediaPlayerModuleChanged_AudioDecoderImpl,
                audio_decoder_impl);
        break;
    }
    }

    return p_bundle;
}
