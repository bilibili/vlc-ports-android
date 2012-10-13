/*****************************************************************************
 * jni_vsl.h: video segment list access
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

#ifndef JNI_VSL_H
#define JNI_VSL_H

#include <stdint.h>
#include <stdbool.h>

int     jvsl_load(void *p_cb_data, bool b_force_reload);
int     jvsl_load_segment(void *p_cb_data, bool b_force_reload, int segment);
int     jvsl_get_count(void *p_cb_data);
char   *jvsl_get_mrl(void *p_cb_data, int i_order);
char   *jvsl_get_url(void *p_cb_data, int i_order);
int     jvsl_get_duration(void *p_cb_data, int i_order);
int64_t jvsl_get_bytes(void *p_cb_data, int i_order);

#endif // JNI_VSL_H
