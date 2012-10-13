/*****************************************************************************
 * libvlcjni_danmaku_readMediaEx.inc.c
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

#ifndef LIBVLCJNI_DANMAKU_READMEDIAEX_INC_C
#define LIBVLCJNI_DANMAKU_READMEDIAEX_INC_C

void create_player_and_play_ex(JNIEnv *env, jobject thiz, jlong instance,
        jobject gui, jstring mrl, jarray options) {
    /* Release previous media player, if any */
    releaseMediaPlayer(env, thiz);

    /* Create a new item */
    libvlc_media_t *m = new_media(instance, env, thiz, mrl, false, false);
    if (!m) {
        LOGE("readMediaEx: Could not create the media!");
        return;
    }

    jobject myJavaLibVLC = (*env)->NewGlobalRef(env, thiz);

    /* Create a media player playing environment */
    libvlc_media_player_t *mp = libvlc_media_player_new(
            (libvlc_instance_t*) instance);

    int user_argc = (*env)->GetArrayLength(env, options);
    int i = 0;
    LOGD("libvlc_media_add_option");
    for (i = 0; i < user_argc; i++) {
        jstring opt = (*env)->GetObjectArrayElement(env, options, i);
        if (opt) {
            const char* optString = (char *) (*env)->GetStringUTFChars(env, opt,
                    NULL);
            if (optString) {
                LOGD("  %s", optString);
                if (!strncmp(optString, ":aout=", 6)) {
                    // seems ":aout" does not effect libvlc_media_add_option()
                    const char* aout_name = optString + 6;
                    if (!strncmp(aout_name, "audiotrack_java", 15)) {
                        LOGD("    set libvlc_audio_output_set:%s", aout_name);
                        libvlc_audio_set_callbacks(mp, aout_play, NULL, NULL,
                                NULL, NULL, (void*) myJavaLibVLC);
                        libvlc_audio_set_format_callbacks(mp, aout_open,
                                aout_close);
                    } else {
                        LOGD("    set libvlc_audio_output_set:%s", aout_name);
                        libvlc_audio_output_set(mp, aout_name);
                    }
                } else {
                    libvlc_media_add_option(m, optString);
                }

                (*env)->ReleaseStringUTFChars(env, opt, optString);
            }
        }
    }

    libvlc_media_player_set_vsl_callback(mp, gui, //
            jvsl_load, //
            jvsl_get_count, //
            jvsl_get_mrl, //
            jvsl_get_url, //
            jvsl_get_duration, //
            jvsl_get_bytes);

    libvlc_media_player_set_media(mp, m);

    /* No need to keep the media now */
    libvlc_media_release(m);

    /* Connect the event manager */
    libvlc_event_manager_t *ev = libvlc_media_player_event_manager(mp);
    for (i = 0; i < (sizeof(mp_events) / sizeof(*mp_events)); ++i)
        libvlc_event_attach(ev, mp_events[i], vlc_event_callback, myVm);

    libvlc_media_t *md = libvlc_media_player_get_media(mp);
    if (md) {
        ev = libvlc_media_event_manager(md);
        for (i = 0; i < sizeof(md_events) / sizeof(*md_events); i++) {
            libvlc_event_attach(ev, md_events[i], vlc_event_callback, myVm);
        }
    }

    /* Keep a pointer to this media player */
    setLong(env, thiz, "mInternalMediaPlayerInstance", (jlong)(intptr_t) mp);

    libvlc_media_player_play(mp);
}

#endif//LIBVLCJNI_DANMAKU_READMEDIAEX_INC_C
