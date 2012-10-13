/*****************************************************************************
 * jni_bundle.h
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

#ifndef JNI_BUNDLE_H
#define JNI_BUNDLE_H

#include "jni_libvlc.h"

typedef struct {
    jclass clsBundle;
    jobject bundle;
    bool needReleaseBundle;

    jmethodID putInt;
    jmethodID getInt;
    jmethodID getLong;
    jmethodID putFloat;
    jmethodID putString;
    jmethodID getString;
} JavaBundle;

JavaBundle *jbundle_init(JNIEnv *env, JavaBundle *p_bundle);
JavaBundle *jbundle_attach(JNIEnv *env, JavaBundle *p_bundle, jobject jbundle);

void jbundle_destroy(JNIEnv *env, JavaBundle *p_bundle);

void jbundle_put_int(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int value);
int jbundle_get_int(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int default_value);

int64_t jbundle_get_long(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        int64_t default_value);

void jbundle_put_float(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        float value);

void jbundle_put_string(JNIEnv *env, JavaBundle *p_bundle, const char *key,
        const char *value);
char *jbundle_get_string(JNIEnv *env, JavaBundle *p_bundle, const char *key);

#endif//JNI_BUNDLE_H
