/*****************************************************************************
 * libvlcjni.c
 *****************************************************************************
 * Copyright © 2010-2013 VLC authors and VideoLAN
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

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <vlc/vlc.h>
#include <vlc_common.h>
#include <vlc_url.h>

#include <jni.h>

#include <android/api-level.h>

#include "../../vlc/contrib/android/ffmpeg/libavutil/avstring.h"
#include "../../vlc/contrib/android/ffmpeg/libavutil/log.h"

#include "libvlcjni.h"
#include "aout.h"
#include "vout.h"
#include "utils.h"

#define VOUT_ANDROID_SURFACE 0
#define VOUT_OPENGLES2       1

#define HW_ACCELERATION_DISABLED 0
#define HW_ACCELERATION_DECODING 1
#define HW_ACCELERATION_FULL     2

#define LOG_TAG "VLC/JNI/main"
#include "log.h"

#include "jni_libvlc.h"
#include "jni_bundle.h"
#include "jni_libvlcevent.h"
#include "jni_vsl.h"
#include "libvlcjni_danmaku_events.inc.c"

#define AOUT_AUDIOTRACK_JAVA 0
#define AOUT_AUDIOTRACK      1
#define AOUT_OPENSLES        2

#if 0
libvlc_media_t *new_media(jlong instance, JNIEnv *env, jobject thiz, jstring fileLocation, bool noOmx, bool noVideo)
{
    libvlc_instance_t *libvlc = (libvlc_instance_t*)(intptr_t)instance;
    jboolean isCopy;
    const char *psz_location = (*env)->GetStringUTFChars(env, fileLocation, &isCopy);
    libvlc_media_t *p_md = libvlc_media_new_location(libvlc, psz_location);
    (*env)->ReleaseStringUTFChars(env, fileLocation, psz_location);
    if (!p_md)
        return NULL;

    if (!noOmx) {
        jclass cls = (*env)->GetObjectClass(env, thiz);
        jmethodID methodId = (*env)->GetMethodID(env, cls, "getHardwareAcceleration", "()I");
        int hardwareAcceleration = (*env)->CallIntMethod(env, thiz, methodId);
        if (hardwareAcceleration == HW_ACCELERATION_DECODING || hardwareAcceleration == HW_ACCELERATION_FULL) {
            /*
             * Set higher caching values if using iomx decoding, since some omx
             * decoders have a very high latency, and if the preroll data isn't
             * enough to make the decoder output a frame, the playback timing gets
             * started too soon, and every decoded frame appears to be too late.
             * On Nexus One, the decoder latency seems to be 25 input packets
             * for 320x170 H.264, a few packets less on higher resolutions.
             * On Nexus S, the decoder latency seems to be about 7 packets.
             */
            libvlc_media_add_option(p_md, ":file-caching=1500");
            libvlc_media_add_option(p_md, ":network-caching=1500");
            libvlc_media_add_option(p_md, ":codec=mediacodec,iomx,all");
        }
        if (noVideo)
            libvlc_media_add_option(p_md, ":no-video");
    }
    return p_md;
}
#endif

libvlc_media_player_t *getMediaPlayer(JNIEnv *env, jobject thiz)
{
    return (libvlc_media_player_t*)(intptr_t)getLong(env, thiz, "mInternalMediaPlayerInstance");
}

static void releaseMediaPlayer(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t* p_mp = getMediaPlayer(env, thiz);
    if (p_mp)
    {
        libvlc_media_player_stop(p_mp);
        libvlc_media_player_release(p_mp);
        setLong(env, thiz, "mInternalMediaPlayerInstance", 0);
    }
}

/* Pointer to the Java virtual machine
 * Note: It's okay to use a static variable for the VM pointer since there
 * can only be one instance of this shared library in a single VM
 */
JavaVM *myVm;

static jobject eventHandlerInstance = NULL;

/** vout lock declared in vout.c */
extern pthread_mutex_t vout_android_lock;

#include "libvlcjni_danmaku_events.inc.c"
static void vlc_event_callback(const libvlc_event_t *ev, void *data)
{
    JNIEnv *env;

    bool isAttached = false;

    if (eventHandlerInstance == NULL)
        return;

    if ((*myVm)->GetEnv(myVm, (void**) &env, JNI_VERSION_1_2) < 0) {
        if ((*myVm)->AttachCurrentThread(myVm, &env, NULL) < 0)
            return;
        isAttached = true;
    }

    JavaBundle javaBundle;
    memset(&javaBundle, 0, sizeof(javaBundle));
    if (NULL == jbundle_from_event(env, ev, &javaBundle)) {
        LOGE("EventHandler: failed to crate bundle");
        return;
    }

    /* Get the object class */
    jclass cls = (*env)->GetObjectClass(env, eventHandlerInstance);
    if (!cls) {
        LOGE("EventHandler: failed to get class reference");
        goto end;
    }

    /* Find the callback ID */
    jmethodID methodID = (*env)->GetMethodID(env, cls, "callback", "(ILandroid/os/Bundle;)V");
    if (methodID) {
        (*env)->CallVoidMethod(env, eventHandlerInstance, methodID, ev->type, javaBundle.bundle);
    } else {
        LOGE("EventHandler: failed to get the callback method");
    }

    (*env)->DeleteLocalRef(env, cls);
end:
    jbundle_destroy(env, &javaBundle);

    if (isAttached)
        (*myVm)->DetachCurrentThread(myVm);
}

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    // Keep a reference on the Java VM.
    myVm = vm;

    pthread_mutex_init(&vout_android_lock, NULL);
    pthread_cond_init(&vout_android_surf_attached, NULL);

    LOGD("JNI interface loaded.");
    return JNI_VERSION_1_2;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
    pthread_mutex_destroy(&vout_android_lock);
    pthread_cond_destroy(&vout_android_surf_attached);
}

// FIXME: use atomics
static bool verbosity;

#include "libvlcjni_fflog.inc.c"

void Java_org_videolan_libvlc_LibVLC_changeVerbosity(JNIEnv *env, jobject thiz, jboolean verbose)
{
    verbosity = verbose;
}

#include "libvlcjni_danmaku_nativeInitEx.inc.c"
#if 0
void Java_org_videolan_libvlc_LibVLC_nativeInit(JNIEnv *env, jobject thiz)
{
    //only use OpenSLES if java side says we can
    jclass cls = (*env)->GetObjectClass(env, thiz);
    jmethodID methodId = (*env)->GetMethodID(env, cls, "getAout", "()I");
    bool use_opensles = (*env)->CallIntMethod(env, thiz, methodId) == AOUT_OPENSLES;

    methodId = (*env)->GetMethodID(env, cls, "getVout", "()I");
    bool use_opengles2 = (*env)->CallIntMethod(env, thiz, methodId) == VOUT_OPENGLES2;

    methodId = (*env)->GetMethodID(env, cls, "timeStretchingEnabled", "()Z");
    bool enable_time_stretch = (*env)->CallBooleanMethod(env, thiz, methodId);

    methodId = (*env)->GetMethodID(env, cls, "frameSkipEnabled", "()Z");
    bool enable_frame_skip = (*env)->CallBooleanMethod(env, thiz, methodId);

    methodId = (*env)->GetMethodID(env, cls, "getDeblocking", "()I");
    int deblocking = (*env)->CallIntMethod(env, thiz, methodId);
    char deblockstr[2] = "3";
    snprintf(deblockstr, 2, "%d", deblocking);
    LOGD("Using deblocking level %d", deblocking);

    methodId = (*env)->GetMethodID(env, cls, "getNetworkCaching", "()I");
    int networkCaching = (*env)->CallIntMethod(env, thiz, methodId);
    char networkCachingstr[25] = "0";
    if(networkCaching > 0) {
        snprintf(networkCachingstr, 25, "--network-caching=%d", networkCaching);
        LOGD("Using network caching of %d ms", networkCaching);
    }

    methodId = (*env)->GetMethodID(env, cls, "getChroma", "()Ljava/lang/String;");
    jstring chroma = (*env)->CallObjectMethod(env, thiz, methodId);
    const char *chromastr = (*env)->GetStringUTFChars(env, chroma, 0);
    LOGD("Chroma set to \"%s\"", chromastr);

    methodId = (*env)->GetMethodID(env, cls, "getSubtitlesEncoding", "()Ljava/lang/String;");
    jstring subsencoding = (*env)->CallObjectMethod(env, thiz, methodId);
    const char *subsencodingstr = (*env)->GetStringUTFChars(env, subsencoding, 0);
    LOGD("Subtitle encoding set to \"%s\"", subsencodingstr);

    methodId = (*env)->GetMethodID(env, cls, "isVerboseMode", "()Z");
    verbosity = (*env)->CallBooleanMethod(env, thiz, methodId);

    methodId = (*env)->GetMethodID(env, cls, "getHardwareAcceleration", "()I");
    int hardwareAcceleration = (*env)->CallIntMethod(env, thiz, methodId);
    /* With the MediaCodec opaque mode we cannot use the OpenGL ES vout. */
    if (hardwareAcceleration == HW_ACCELERATION_FULL)
        use_opengles2 = false;

    /* Don't add any invalid options, otherwise it causes LibVLC to crash */
    const char *argv[] = {
        "-I", "dummy",
        "--no-osd",
        "--no-video-title-show",
        "--no-stats",
        "--no-plugins-cache",
        "--no-drop-late-frames",
        /* The VLC default is to pick the highest resolution possible
         * (i.e. 1080p). For mobile, pick a more sane default for slow
         * mobile data networks and slower hardware. */
        "--preferred-resolution", "360",
        "--avcodec-fast",
        "--avcodec-threads=0",
        "--subsdec-encoding", subsencodingstr,
        enable_time_stretch ? "--audio-time-stretch" : "--no-audio-time-stretch",
        "--avcodec-skiploopfilter", deblockstr,
        "--avcodec-skip-frame", enable_frame_skip ? "2" : "0",
        "--avcodec-skip-idct", enable_frame_skip ? "2" : "0",
        (networkCaching > 0) ? networkCachingstr : "",
        use_opensles ? "--aout=opensles" : "--aout=android_audiotrack",
        use_opengles2 ? "--vout=gles2" : "--vout=androidsurface",
        "--androidsurface-chroma", chromastr != NULL && chromastr[0] != 0 ? chromastr : "RV32",
        (hardwareAcceleration == HW_ACCELERATION_FULL) ? "" : "--no-mediacodec-dr",
    };
    libvlc_instance_t *instance = libvlc_new(sizeof(argv) / sizeof(*argv), argv);

    setLong(env, thiz, "mLibVlcInstance", (jlong)(intptr_t) instance);

    (*env)->ReleaseStringUTFChars(env, chroma, chromastr);
    (*env)->ReleaseStringUTFChars(env, subsencoding, subsencodingstr);

    if (!instance)
    {
        jclass exc = (*env)->FindClass(env, "org/videolan/libvlc/LibVlcException");
        (*env)->ThrowNew(env, exc, "Unable to instantiate LibVLC");
    }

    LOGI("LibVLC initialized: %p", instance);

    libvlc_log_set(instance, debug_log, &verbosity);
}
#endif

void Java_org_videolan_libvlc_LibVLC_nativeDestroy(JNIEnv *env, jobject thiz)
{
    releaseMediaPlayer(env, thiz);
    jlong libVlcInstance = getLong(env, thiz, "mLibVlcInstance");
    if (!libVlcInstance)
        return; // Already destroyed

    libvlc_instance_t *instance = (libvlc_instance_t*)(intptr_t) libVlcInstance;
    libvlc_log_unset(instance);
    libvlc_release(instance);

    setLong(env, thiz, "mLibVlcInstance", 0);
	jobject ref = (jobject) getInt(env, thiz, "mLibVlcVsl");
	if (ref)
		(*env)->DeleteGlobalRef(env, ref);
	setInt(env, thiz, "mLibVlcVsl", 0);
}

void Java_org_videolan_libvlc_LibVLC_detachEventHandler(JNIEnv *env, jobject thiz)
{
    if (eventHandlerInstance != NULL) {
        (*env)->DeleteGlobalRef(env, eventHandlerInstance);
        eventHandlerInstance = NULL;
    }
}

void Java_org_videolan_libvlc_LibVLC_setEventHandler(JNIEnv *env, jobject thiz, jobject eventHandler)
{
    if (eventHandlerInstance != NULL) {
        (*env)->DeleteGlobalRef(env, eventHandlerInstance);
        eventHandlerInstance = NULL;
    }

    eventHandlerInstance = getEventHandlerReference(env, thiz, eventHandler);
}

void Java_org_videolan_libvlc_LibVLC_setVslHandler(JNIEnv *env, jobject thiz, jobject handler)
{
	jobject ref;

	ref = (jobject) getInt(env, thiz, "mLibVlcVsl");
	if (ref)
		(*env)->DeleteGlobalRef(env, ref);
	ref = (*env)->NewGlobalRef(env, handler);
	setInt(env, thiz, "mLibVlcVsl", (int) ref);
}

void Java_org_videolan_libvlc_LibVLC_playMRL(JNIEnv *env, jobject thiz, jlong instance,
                                             jstring mrl, jobjectArray mediaOptions)
{
    /* Release previous media player, if any */
    releaseMediaPlayer(env, thiz);

    /* Create a media player playing environment */
    libvlc_media_player_t *mp = libvlc_media_player_new((libvlc_instance_t*)(intptr_t)instance);
    jobject myJavaLibVLC = (*env)->NewGlobalRef(env, thiz);

#if 0
    //if AOUT_AUDIOTRACK_JAVA, we use amem
    jclass cls = (*env)->GetObjectClass(env, thiz);
    jmethodID methodId = (*env)->GetMethodID(env, cls, "getAout", "()I");
    if ( (*env)->CallIntMethod(env, thiz, methodId) == AOUT_AUDIOTRACK_JAVA )
    {
        libvlc_audio_set_callbacks(mp, aout_play, aout_pause, NULL, NULL, NULL,
                                   (void*) myJavaLibVLC);
        libvlc_audio_set_format_callbacks(mp, aout_open, aout_close);
    }

    /* Connect the event manager */
    libvlc_event_manager_t *ev = libvlc_media_player_event_manager(mp);
    static const libvlc_event_type_t mp_events[] = {
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerVout,
        libvlc_MediaPlayerPositionChanged,
        libvlc_MediaPlayerEncounteredError
    };
    for(int i = 0; i < (sizeof(mp_events) / sizeof(*mp_events)); i++)
        libvlc_event_attach(ev, mp_events[i], vlc_event_callback, myVm);
#endif


    /* Keep a pointer to this media player */
    setLong(env, thiz, "mInternalMediaPlayerInstance", (jlong)(intptr_t)mp);

#if 0
    cls = (*env)->GetObjectClass(env, thiz);
    jmethodID methodID = (*env)->GetMethodID(env, cls, "applyEqualizer", "()V");
    (*env)->CallVoidMethod(env, thiz, methodID);
#endif

    const char* p_mrl = (*env)->GetStringUTFChars(env, mrl, 0);

    libvlc_media_t* p_md = libvlc_media_new_location((libvlc_instance_t*)(intptr_t)instance, p_mrl);
    /* media options */
    if (mediaOptions != NULL)
    {
        int stringCount = (*env)->GetArrayLength(env, mediaOptions);
        for(int i = 0; i < stringCount; i++)
        {
            jstring option = (jstring)(*env)->GetObjectArrayElement(env, mediaOptions, i);
            const char* p_st = (*env)->GetStringUTFChars(env, option, 0);
            LOGD("  %s", p_st);
            if (!strncmp(p_st, ":aout=", 6)) {
                // seems ":aout" does not effect libvlc_media_add_option()
                const char* aout_name = p_st + 6;
                if (!strncmp(aout_name, "audiotrack_java", 15)) {
                    LOGD("    set libvlc_audio_output_set:%s", aout_name);
                    libvlc_audio_set_callbacks(mp, aout_play, aout_pause, NULL,
                                               NULL, NULL, (void*) myJavaLibVLC);
                    libvlc_audio_set_format_callbacks(mp, aout_open,
                                                      aout_close);
                } else {
                    LOGD("    set libvlc_audio_output_set:%s", aout_name);
                    libvlc_audio_output_set(mp, aout_name);
                }
            } else {
                libvlc_media_add_option(p_md, p_st); // option
            }
            (*env)->ReleaseStringUTFChars(env, option, p_st);
        }
    }

    (*env)->ReleaseStringUTFChars(env, mrl, p_mrl);

    libvlc_media_player_set_vsl_callback(mp,
                                         getInt(env, thiz, "mLibVlcVsl"), //
                                         jvsl_load, //
                                         jvsl_load_segment, //
                                         jvsl_get_count, //
                                         jvsl_get_mrl, //
                                         jvsl_get_url, //
                                         jvsl_get_duration, //
                                         jvsl_get_bytes);

    libvlc_media_player_set_media(mp, p_md);
    /* No need to keep the media now */
    libvlc_media_release(p_md);

    /* Connect the event manager */
    libvlc_event_manager_t *ev = libvlc_media_player_event_manager(mp);
    for (int i = 0; i < (sizeof(mp_events) / sizeof(*mp_events)); ++i)
        libvlc_event_attach(ev, mp_events[i], vlc_event_callback, myVm);

    p_md = libvlc_media_player_get_media(mp);
    if (p_md) {
        ev = libvlc_media_event_manager(p_md);
        for (int i = 0; i < sizeof(md_events) / sizeof(*md_events); i++) {
            libvlc_event_attach(ev, md_events[i], vlc_event_callback, myVm);
        }
    }

    libvlc_media_player_play(mp);
}

#if 0
jfloat Java_org_videolan_libvlc_LibVLC_getRate(JNIEnv *env, jobject thiz) {
    libvlc_media_player_t* mp = getMediaPlayer(env, thiz);
    if(mp)
        return libvlc_media_player_get_rate(mp);
    else
        return 1.00;
}

void Java_org_videolan_libvlc_LibVLC_setRate(JNIEnv *env, jobject thiz, jfloat rate) {
    libvlc_media_player_t* mp = getMediaPlayer(env, thiz);
    if(mp)
        libvlc_media_player_set_rate(mp, rate);
}
#endif

jboolean Java_org_videolan_libvlc_LibVLC_hasMediaPlayer(JNIEnv *env, jobject thiz)
{
    return !!getMediaPlayer(env, thiz);
}

jboolean Java_org_videolan_libvlc_LibVLC_isPlaying(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return !!libvlc_media_player_is_playing(mp);
    else
        return 0;
}

jboolean Java_org_videolan_libvlc_LibVLC_isSeekable(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return !!libvlc_media_player_is_seekable(mp);
    return 0;
}

void Java_org_videolan_libvlc_LibVLC_play(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        libvlc_media_player_play(mp);
}

void Java_org_videolan_libvlc_LibVLC_pause(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        libvlc_media_player_pause(mp);
}

void Java_org_videolan_libvlc_LibVLC_stop(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        libvlc_media_player_stop(mp);
}

#if 0
jint Java_org_videolan_libvlc_LibVLC_getVolume(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return (jint) libvlc_audio_get_volume(mp);
    return -1;
}

jint Java_org_videolan_libvlc_LibVLC_setVolume(JNIEnv *env, jobject thiz, jint volume)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        //Returns 0 if the volume was set, -1 if it was out of range or error
        return (jint) libvlc_audio_set_volume(mp, (int) volume);
    return -1;
}
#endif

jlong Java_org_videolan_libvlc_LibVLC_getTime(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return libvlc_media_player_get_time(mp);
    return -1;
}

void Java_org_videolan_libvlc_LibVLC_setTime(JNIEnv *env, jobject thiz, jlong time)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        libvlc_media_player_set_time(mp, time);
}

jfloat Java_org_videolan_libvlc_LibVLC_getPosition(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return (jfloat) libvlc_media_player_get_position(mp);
    return -1;
}

void Java_org_videolan_libvlc_LibVLC_setPosition(JNIEnv *env, jobject thiz, jfloat pos)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        libvlc_media_player_set_position(mp, pos);
}

jlong Java_org_videolan_libvlc_LibVLC_getLength(JNIEnv *env, jobject thiz)
{
    libvlc_media_player_t *mp = getMediaPlayer(env, thiz);
    if (mp)
        return (jlong) libvlc_media_player_get_length(mp);
    return -1;
}

jstring Java_org_videolan_libvlc_LibVLC_version(JNIEnv* env, jobject thiz)
{
    return (*env)->NewStringUTF(env, libvlc_get_version());
}

jstring Java_org_videolan_libvlc_LibVLC_compiler(JNIEnv* env, jobject thiz)
{
    return (*env)->NewStringUTF(env, libvlc_get_compiler());
}

jstring Java_org_videolan_libvlc_LibVLC_changeset(JNIEnv* env, jobject thiz)
{
    return (*env)->NewStringUTF(env, libvlc_get_changeset());
}
