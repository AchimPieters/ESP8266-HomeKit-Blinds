/** Copyright 2021 Achim Pieters | StudioPieters®

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Build upon: Custom Characteristics - Apache License - Copyright (c) Copyright 2018 David B Brown (@maccoylton)
 **/

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <FreeRTOS.h>

#ifndef __HOMEKIT_DBB_CUSTOM_CHARACTERISTICS__
#define __HOMEKIT_DBB_CUSTOM_CHARACTERISTICS__

#define HOMEKIT_CUSTOM_UUID_DBB(value) (value "-4772-4466-80fd-a6ea3d5bcd55")

#define HOMEKIT_CHARACTERISTIC_CUSTOM_FACTOR HOMEKIT_CUSTOM_UUID_DBB("F0000001")
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM_FACTOR(_value, ...), \.type = HOMEKIT_CHARACTERISTIC_CUSTOM_FACTOR, \
.description = "Factor", \
.format = homekit_format_float, \
.permissions = homekit_permissions_paired_read \
| homekit_permissions_notify, \
.min_value = (float[]) {0}, \
.max_value = (float[]) {30000}, \
.min_step = (float[]) {1}, \
.value = HOMEKIT_FLOAT_(_value), \
## __VA_ARGS__

#endif

void save_characteristic_to_flash (homekit_characteristic_t *ch, homekit_value_t value);

void load_characteristic_from_flash (homekit_characteristic_t *ch);

void get_sysparam_info();

void save_int32_param ( const char *description, int32_t new_value);

void save_float_param ( const char *description, float new_float_value);

void load_float_param ( const char *description, float *new_float_value);

void homekit_characteristic_bounds_check (homekit_characteristic_t *ch);
/* check that integers and floats are within min and max values */
