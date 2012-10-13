/*****************************************************************************
 * jni_bundle.c
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

#include <stdbool.h>

#define LOG_TAG "VLC/JNI/jni_bundle"
#include "log.h"

#include "jni_bundle.h"

JavaBundle *jbundle_init(JNIEnv *env, JavaBundle *p_bundle) {
    memset(p_bundle, 0, sizeof(JavaBundle));

    p_bundle->clsBundle = (*env)->FindClass(env, "android/os/Bundle");
    jmethodID clsCtor = (*env)->GetMethodID(env, p_bundle->clsBundle, "<init>",
            "()V");

    p_bundle->bundle = (*env)->NewObject(env, p_bundle->clsBundle, clsCtor);
    p_bundle->needReleaseBundle = true;
    return p_bundle;
}

JavaBundle *jbundle_attach(JNIEnv *env, JavaBundle *p_bundle, jobject jbundle) {
    memset(p_bundle, 0, sizeof(JavaBundle));

    p_bundle->clsBundle = (*env)->FindClass(env, "android/os/Bundle");
    p_bundle->bundle = jbundle;
    p_bundle->needReleaseBundle = false;
    return p_bundle;
}

void jbundle_destroy(JNIEnv *env, JavaBundle *p_bundle) {
    if (p_bundle->needReleaseBundle && p_bundle->bundle != NULL) {
        (*env)->DeleteLocalRef(env, p_bundle->bundle);
    }

    memset(p_bundle, 0, sizeof(JavaBundle));
}

void jbundle_put_int(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int value) {
    if (p_bundle->putInt == NULL) {
        p_bundle->putInt = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "putInt", "(Ljava/lang/String;I)V");
    }

    jstring j_key = (*env)->NewStringUTF(env, key);
    (*env)->CallVoidMethod(env, p_bundle->bundle, p_bundle->putInt, j_key,
            value);
    (*env)->DeleteLocalRef(env, j_key);
}

int jbundle_get_int(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int default_value) {
    if (p_bundle->getInt == NULL) {
        p_bundle->getInt = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "getInt", "(Ljava/lang/String;I)I");
    }

    jstring j_key = (*env)->NewStringUTF(env, key);
    jint ret = (*env)->CallIntMethod(env, p_bundle->bundle, p_bundle->getInt,
            j_key, default_value);
    (*env)->DeleteLocalRef(env, j_key);
    return ret;
}

int64_t jbundle_get_long(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int64_t default_value) {
    if (p_bundle->getLong == NULL) {
        p_bundle->getLong = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "getLong", "(Ljava/lang/String;J)J");
    }

    jstring j_key = (*env)->NewStringUTF(env, key);
    jlong ret = (*env)->CallLongMethod(env, p_bundle->bundle, p_bundle->getLong,
            j_key, (jlong) default_value);
    (*env)->DeleteLocalRef(env, j_key);
    return ret;
}

void jbundle_put_float(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        float value) {
    if (p_bundle->putFloat == NULL) {
        p_bundle->putFloat = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "putFloat", "(Ljava/lang/String;F)V");
    }

    jstring j_key = (*env)->NewStringUTF(env, key);
    (*env)->CallVoidMethod(env, p_bundle->bundle, p_bundle->putFloat, j_key,
            value);
    (*env)->DeleteLocalRef(env, j_key);
}

void jbundle_put_string(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        const char *value) {
    if (p_bundle->putString == NULL) {
        p_bundle->putString = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "putString", "(Ljava/lang/String;Ljava/lang/String;)V");
    }

    jstring j_key = (*env)->NewStringUTF(env, key);
    jstring j_value = (*env)->NewStringUTF(env, value ? value : "");
    (*env)->CallVoidMethod(env, p_bundle->bundle, p_bundle->putString, j_key,
            j_value);
    (*env)->DeleteLocalRef(env, j_key);
    (*env)->DeleteLocalRef(env, j_value);
}

char *jbundle_get_string(JNIEnv *env, JavaBundle *p_bundle, const char *key) {
    if (p_bundle->getString == NULL) {
        p_bundle->getString = (*env)->GetMethodID(env, p_bundle->clsBundle,
                "getString", "(Ljava/lang/String;)Ljava/lang/String;");
    }

    char *ret = NULL;
    jstring j_key = (*env)->NewStringUTF(env, key);
    jobject j_value = (*env)->CallObjectMethod(env, p_bundle->bundle,
            p_bundle->getString, j_key);
    (*env)->DeleteLocalRef(env, j_key);
    if (j_value) {
        char *utf_chars = (char *) (*env)->GetStringUTFChars(env, j_value,
                NULL);
        if (utf_chars) {
            ret = strdup(utf_chars);
            (*env)->ReleaseStringUTFChars(env, j_value, utf_chars);
        }
        (*env)->DeleteLocalRef(env, j_value);
    }

    return ret;
}
