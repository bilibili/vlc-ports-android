/*****************************************************************************
 * libvlcjni_danmaku_nativeInitEx.inc.c
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

#ifndef LIBVLCJNI_DANMAKU_NATIVEINITEX_INC_C
#define LIBVLCJNI_DANMAKU_NATIVEINITEX_INC_C

void Java_org_videolan_libvlc_LibVLC_nativeInitEx(JNIEnv *env, jobject thiz, jboolean verbose, jarray arguments)
{
    libvlc_instance_t *instance = NULL;

    int user_argc = (*env)->GetArrayLength(env, arguments);
    char **user_argv = (char **) calloc(user_argc, sizeof(char *));
    int i = 0;
    for (i = 0; i < user_argc; i++)
    {
        jstring argument = (*env)->GetObjectArrayElement(env, arguments, i);
        if (argument)
            user_argv[i] = (char *) (*env)->GetStringUTFChars(env, argument, NULL);
    }
    instance = libvlc_new(user_argc, (const char **) user_argv);
    for (i = 0; i < user_argc; i++)
    {
        jstring argument = (*env)->GetObjectArrayElement(env, arguments, i);
        if (argument)
            (*env)->ReleaseStringUTFChars(env, argument, user_argv[i]);
    }

    setLong(env, thiz, "mLibVlcInstance", (jlong)(intptr_t) instance);

    if (!instance)
    {
        jclass exc = (*env)->FindClass(env, "org/videolan/libvlc/LibVlcException");
        (*env)->ThrowNew(env, exc, "Unable to instantiate LibVLC");
    }

    LOGI("LibVLC initialized: %p", instance);

    libvlc_log_set(instance, debug_log, &verbosity);
    av_log_set_callback(ffmpeg_debug_log);
}

#endif//LIBVLCJNI_DANMAKU_NATIVEINITEX_INC_C
