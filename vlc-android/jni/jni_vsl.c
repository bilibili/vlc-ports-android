/*****************************************************************************
 * jni_vsl.c: video segment list access
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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <jni.h>

#include <vlc/vlc.h>

#include "jni_vsl.h"
#include "jni_bundle.h"

#define LOG_TAG "VLC/JNI/vsl"
#include "log.h"

/** Unique Java VM instance, as defined in libvlcjni.c */
extern JavaVM *myVm;

#define BUNDLE_KEY_BUF_SIZE 64
#define BUNDLE_COUNT "count"

#define BUNDLE_MRL "segment_mrl"
#define BUNDLE_URL "segment_url"
#define BUNDLE_DURATION "duration"
#define BUNDLE_BYTES "bytes"

static JavaVMAttachArgs g_jvsl_thr_args = {JNI_VERSION_1_2, "vsl-thread", NULL};

static jobject do_get_bundle(JNIEnv* env, jobject gui)
{
    /* Get the object class */
    jclass cls = (*env)->GetObjectClass(env, gui);
    if (!cls) {
        LOGE("vsl: failed to get class reference");
        return NULL;
    }

    /* Find the callback ID */
    jmethodID methodID = (*env)->GetMethodID(env, cls, "vslGetBundle",
            "()Landroid/os/Bundle;");
    if (!methodID) {
        LOGE("vsl: failed to get method vslGetBundle");
        return NULL;
    }

    jobject jbundle = (*env)->CallObjectMethod(env, gui, methodID);
    return jbundle;
}

/* */
static int do_load(JNIEnv* env, jobject gui, bool b_force_reload)
{
    /* Get the object class */
    jclass cls = (*env)->GetObjectClass(env, gui);
    if (!cls) {
        LOGE("vsl: failed to get class reference");
        return -1;
    }

    /* Find the callback ID */
    jmethodID methodID = (*env)->GetMethodID(env, cls, "vslLoad", "(Z)Z");
    if (!methodID) {
        LOGE("vsl: failed to get method vslLoad");
        return -1;
    }

    jboolean jsuccess = (*env)->CallBooleanMethod(env, gui, methodID,
            b_force_reload);
    return jsuccess ? 0 : -1;
}

int jvsl_load(void *p_cb_data, bool b_force_reload)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return -1;
    }

    int ret = do_load(env, p_cb_data, b_force_reload);
    return ret;
}

/* */
static int do_load_segment(JNIEnv* env, jobject gui, bool b_force_reload, int segment)
{
    /* Get the object class */
    jclass cls = (*env)->GetObjectClass(env, gui);
    if (!cls) {
        LOGE("vsl: failed to get class reference");
        return -1;
    }

    /* Find the callback ID */
    jmethodID methodID = (*env)->GetMethodID(env, cls, "vslLoadSegment", "(ZI)Z");
    if (!methodID) {
        LOGE("vsl: failed to get method vslLoad");
        return -1;
    }

    jboolean jsuccess = (*env)->CallBooleanMethod(env, gui, methodID,
                                                  b_force_reload, segment);
    return jsuccess ? 0 : -1;
}

int jvsl_load_segment(void *p_cb_data, bool b_force_reload, int segment)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return -1;
    }

    int ret = do_load_segment(env, p_cb_data, b_force_reload, segment);
    return ret;
}

/* */
static int do_get_count(JNIEnv* env, jobject gui)
{
    jobject jbundle = do_get_bundle(env, gui);
    if (!jbundle) {
        LOGE("do_get_count NULL bundle");
        return -1;
    }

    JavaBundle javaBundle;
    jbundle_attach(env, &javaBundle, jbundle);
    int ret = jbundle_get_int(env, &javaBundle, BUNDLE_COUNT, -1);
    jbundle_destroy(env, &javaBundle);

    (*env)->DeleteLocalRef(env, jbundle);
    return ret;
}

int jvsl_get_count(void *p_cb_data)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return -1;
    }

    int ret = do_get_count(env, p_cb_data);
    return ret;
}

/* */
static char *do_get_mrl(JNIEnv* env, jobject gui, int order)
{
    jobject jbundle = do_get_bundle(env, gui);
    if (!jbundle) {
        LOGE("do_get_count NULL bundle");
        return NULL;
    }

    char szBuf[BUNDLE_KEY_BUF_SIZE];
    sprintf(szBuf, "%d_%s", order, BUNDLE_MRL);
    JavaBundle javaBundle;
    jbundle_attach(env, &javaBundle, jbundle);
    LOGD("do_get_mrl: %s", szBuf);
    char *ret = jbundle_get_string(env, &javaBundle, szBuf);
    jbundle_destroy(env, &javaBundle);

    (*env)->DeleteLocalRef(env, jbundle);
    return ret;
}

char *jvsl_get_mrl(void *p_cb_data, int order)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return NULL;
    }

    char *ret = do_get_mrl(env, p_cb_data, order);
    return ret;
}

/* */
static char *do_get_url(JNIEnv* env, jobject gui, int order)
{
    jobject jbundle = do_get_bundle(env, gui);
    if (!jbundle) {
        LOGE("do_get_count NULL bundle");
        return NULL;
    }

    char szBuf[BUNDLE_KEY_BUF_SIZE];
    sprintf(szBuf, "%d_%s", order, BUNDLE_URL);
    JavaBundle javaBundle;
    jbundle_attach(env, &javaBundle, jbundle);
    LOGD("do_get_url: %s", szBuf);
    char *ret = jbundle_get_string(env, &javaBundle, szBuf);
    jbundle_destroy(env, &javaBundle);

    (*env)->DeleteLocalRef(env, jbundle);
    return ret;
}

char *jvsl_get_url(void *p_cb_data, int order)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return NULL;
    }

    char *ret = do_get_url(env, p_cb_data, order);
    return ret;
}

/* */
static int do_get_duratuon(JNIEnv* env, jobject gui, int order)
{
    jobject jbundle = do_get_bundle(env, gui);
    if (!jbundle) {
        LOGE("do_get_count NULL bundle");
        return -1;
    }

    char szBuf[BUNDLE_KEY_BUF_SIZE];
    sprintf(szBuf, "%d_%s", order, BUNDLE_DURATION);
    JavaBundle javaBundle;
    jbundle_attach(env, &javaBundle, jbundle);
    LOGD("do_get_duratuon: %s", szBuf);
    int ret = jbundle_get_int(env, &javaBundle, szBuf, -1);
    jbundle_destroy(env, &javaBundle);

    (*env)->DeleteLocalRef(env, jbundle);
    return ret;
}

int jvsl_get_duration(void *p_cb_data, int order)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return -1;
    }

    int ret = do_get_duratuon(env, p_cb_data, order);
    return ret;
}

/* */
static int64_t do_get_bytes(JNIEnv* env, jobject gui, int order)
{
    jobject jbundle = do_get_bundle(env, gui);
    if (!jbundle) {
        LOGE("do_get_count NULL bundle");
        return -1;
    }

    char szBuf[BUNDLE_KEY_BUF_SIZE];
    sprintf(szBuf, "%d_%s", order, BUNDLE_BYTES);
    JavaBundle javaBundle;
    jbundle_attach(env, &javaBundle, jbundle);
    LOGD("do_get_bytes: %s", szBuf);
    int64_t ret = jbundle_get_long(env, &javaBundle, szBuf, -1);
    jbundle_destroy(env, &javaBundle);

    (*env)->DeleteLocalRef(env, jbundle);
    return ret;
}

int64_t jvsl_get_bytes(void *p_cb_data, int order)
{
    JNIEnv *env = NULL;
    if (SDL_AndroidJni_SetupThreadEnv(&env, &g_jvsl_thr_args) != 0) {
        LOGE("Could not attach the jvsl thread to the JVM !");
        return -1;
    }

    int64_t ret = do_get_bytes(env, p_cb_data, order);
    return ret;
}
