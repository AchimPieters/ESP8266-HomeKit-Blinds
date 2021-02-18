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

 #include <sysparam.h>
 #include <string.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <homekit/homekit.h>
 #include <homekit/characteristics.h>
 #include "characteristic_types.h"


void homekit_characteristic_bounds_check (homekit_characteristic_t *ch){

        printf ("%s: %s: ",__func__, ch->description);
        switch (ch->format) {
        case homekit_format_bool:
                if ((ch->value.bool_value != 0) && (ch->value.bool_value != 1)) {
                        ch->value.bool_value = false;
                }
                break;
        case homekit_format_uint8:
                printf ("Checking uint8 bounds");
                if (ch->value.int_value > *ch->max_value) {
                        ch->value.int_value = *ch->max_value;
                }
                if (ch->value.int_value < *ch->min_value) {
                        ch->value.int_value = *ch->min_value;
                }
                break;
        case homekit_format_int:
        case homekit_format_uint16:
        case homekit_format_uint32:
                printf ("Checking integer bounds");
                if (ch->value.int_value > *ch->max_value) {
                        ch->value.int_value = *ch->max_value;
                }
                if (ch->value.int_value < *ch->min_value) {
                        ch->value.int_value = *ch->min_value;
                }
                break;
        case homekit_format_string:
                break;
        case homekit_format_float:
                printf ("Checking float bounds");
                if (ch->value.float_value > *ch->max_value) {
                        ch->value.float_value = *ch->max_value;
                }
                if (ch->value.float_value < *ch->min_value) {
                        ch->value.float_value = *ch->min_value;
                }
                break;
        case homekit_format_uint64:
        case homekit_format_tlv:
        default:
                printf ("Unknown characteristic format\n");
        }
        printf("\n");
}



void print_binary_value(char *key, uint8_t *value, size_t len) {
        static size_t i;

        printf("  %s:", key);
        for (i = 0; i < len; i++) {
                if (!(i & 0x0f)) {
                        printf("\n   ");
                }
                printf(" %02x", value[i]);
        }
        printf("\n");
}


void get_sysparam_info() {

        printf("%s: , Freep Heap=%d\n", __func__, xPortGetFreeHeapSize());

        static uint32_t base_addr,num_sectors;
        static sysparam_iter_t sysparam_iter;
        static sysparam_status_t sysparam_status;

        sysparam_get_info(&base_addr, &num_sectors);

        printf ("%s: Sysparam base address %i, num_sectors %i\n", __func__, base_addr, num_sectors);
        sysparam_status = sysparam_iter_start (&sysparam_iter);
        if (sysparam_status != 0) {
                printf("%s: iter_start status %d\n",__func__, sysparam_status);
        }
        while (sysparam_status==0) {
                sysparam_status = sysparam_iter_next (&sysparam_iter);
                if (sysparam_status==0) {
                        if (!sysparam_iter.binary)
                                if (strncmp(sysparam_iter.key,"wifi_password", 13 ) == 0) {
                                        if (strlen ((char *)sysparam_iter.value) == 0) {
                                                printf("%s: sysparam name: %s, value: blank\n", __func__, sysparam_iter.key);
                                        } else {
                                                printf("%s: sysparam name: %s, value:%s\n", __func__, sysparam_iter.key,"**********");
                                        }
                                } else {
                                        printf("%s: sysparam name: %s, value:%s, key length:%d, value length:%d\n", __func__, sysparam_iter.key, (char *)sysparam_iter.value, sysparam_iter.key_len, sysparam_iter.value_len);
                                }
                        else
                                print_binary_value(sysparam_iter.key, sysparam_iter.value, sysparam_iter.value_len);
                } else {
                        printf("%s: while loop status %d\n",__func__, sysparam_status);
                }
        }
        sysparam_iter_end (&sysparam_iter);
        if (sysparam_status != SYSPARAM_NOTFOUND) {
                //   SYSPARAM_NOTFOUND is the normal status when we've reached the end of all entries.
                printf ("%s: sysparam iter_end error:%d\n", __func__, sysparam_status);
        }
        printf("%s: , Freep Heap=%d\n", __func__, xPortGetFreeHeapSize());
}


void save_int32_param ( const char *description, int32_t new_value){

        static sysparam_status_t status = SYSPARAM_OK;
        static int32_t current_value;

        status = sysparam_get_int32(description, &current_value);
        if (status == SYSPARAM_OK && current_value != new_value) {
                status = sysparam_set_int32(description, new_value);
        } else if (status == SYSPARAM_NOTFOUND) {
                status = sysparam_set_int32(description, new_value);
        }
}


void save_float_param ( const char *description, float new_float_value){

        static sysparam_status_t status = SYSPARAM_OK;
        static int32_t current_value, new_value;

        new_value = (int) (new_float_value*100);

        status = sysparam_get_int32(description, &current_value);

        switch (status) {
        case SYSPARAM_OK:
                if (current_value != new_value) {
                        status = sysparam_set_int32(description, new_value);
                        printf ("%s: description: %s, value: %f, stored: %d\n", __func__, description, new_float_value, new_value);
                } else {
                        printf ("%s: No change to value no update required, description: %s, value: %f\n", __func__, description, new_float_value);
                }
                break;
        case SYSPARAM_NOTFOUND:
                status = sysparam_set_int32(description, new_value);
                printf ("%s: description: %s, value: %f\n", __func__, description, new_float_value);
                break;
        default: {
                printf ("%s: error search for sysparam, %s, error no: %d\n", __func__, description, status);
        }
        }
}



void save_characteristic_to_flash(homekit_characteristic_t *ch, homekit_value_t value){

        static sysparam_status_t status = SYSPARAM_OK;
        static bool bool_value;
        static int8_t int8_value;
        static char *string_value=NULL;

        printf ("%s: %s: ",__func__, ch->description);
        switch (ch->format) {
        case homekit_format_bool:
                printf ("writing bool value to flash %s\n", ch->value.bool_value ? "true" : "false");
                status = sysparam_get_bool(ch->description, &bool_value);
                if (status == SYSPARAM_OK && bool_value != ch->value.bool_value) {
                        status = sysparam_set_bool(ch->description, ch->value.bool_value);
                } else if (status == SYSPARAM_NOTFOUND) {
                        status = sysparam_set_bool(ch->description, ch->value.bool_value);
                }
                break;
        case homekit_format_uint8:
                printf ("writing int8 value to flash %d\n", ch->value.int_value);
                status = sysparam_get_int8(ch->description, &int8_value);
                if (status == SYSPARAM_OK && int8_value != ch->value.int_value) {
                        status = sysparam_set_int8(ch->description, ch->value.int_value);
                } else if (status == SYSPARAM_NOTFOUND) {
                        status = sysparam_set_int8(ch->description, ch->value.int_value);
                }
                break;
        case homekit_format_int:
        case homekit_format_uint16:
        case homekit_format_uint32:
                printf ("writing int32 value to flash %d\n",  ch->value.int_value);
                save_int32_param (ch->description, ch->value.int_value);
                break;
        case homekit_format_string:
                printf ("writing string value to flash %s\n", ch->value.string_value);
                status = sysparam_get_string(ch->description, &string_value);
                if (status == SYSPARAM_OK && !strcmp (string_value, ch->value.string_value)) {
                        status = sysparam_set_string(ch->description, ch->value.string_value);
                }  else if (status == SYSPARAM_NOTFOUND) {
                        status = sysparam_set_string(ch->description, ch->value.string_value);
                }
                free(string_value);
                break;
        case homekit_format_float:
                printf ("writing float value to flash %f\n", ch->value.float_value);
                save_float_param (ch->description,ch->value.float_value);
                break;
        case homekit_format_uint64:
        case homekit_format_tlv:
        default:
                printf ("Unknown characteristic format\n");
        }
        if (status != SYSPARAM_OK) {
                printf ("%s: Error in sysparams error:%i\n", __func__, status);
        }

}



void load_float_param ( const char *description, float *new_float_value){

        static sysparam_status_t status = SYSPARAM_OK;
        static int32_t int32_value;

        status = sysparam_get_int32(description, &int32_value);

        if (status == SYSPARAM_OK ) {
                *new_float_value = int32_value * 1.0f / 100;
                printf("%s: %s value %f, stored: %d\n", __func__, description, *new_float_value, int32_value);
        }
}


void load_characteristic_from_flash (homekit_characteristic_t *ch){

        static sysparam_status_t status = SYSPARAM_OK;
        static bool bool_value;
        static int8_t int8_value;
        static int32_t int32_value;
        static char *string_value = NULL;

        printf ("%s: %s: ",__func__, ch->description);
        switch (ch->format) {
        case homekit_format_bool:
                printf("bool: ");
                status = sysparam_get_bool(ch->description, &bool_value);
                if (status == SYSPARAM_OK ) {
                        ch->value.bool_value = bool_value;
                        printf("%s\n", ch->value.bool_value ? "true" : "false");
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_uint8:
                printf("uint8: ");
                status = sysparam_get_int8(ch->description, &int8_value);
                if (status == SYSPARAM_OK) {
                        ch->value.int_value = int8_value;
                        printf("%d\n", ch->value.int_value);
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_int:
                printf("int: ");
                status = sysparam_get_int32(ch->description, &int32_value);
                if (status == SYSPARAM_OK ) {
                        ch->value.int_value = (int)int32_value;
                        printf("%d\n", ch->value.int_value);
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_uint16:
                printf("uint16: ");
                status = sysparam_get_int32(ch->description, &int32_value);
                if (status == SYSPARAM_OK ) {
                        ch->value.int_value = (uint16_t)int32_value;
                        printf("%d\n", ch->value.int_value);
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_uint32:
                printf("uint32: ");
                status = sysparam_get_int32(ch->description, &int32_value);
                if (status == SYSPARAM_OK ) {
                        ch->value.int_value = int32_value;
                        printf("%d\n", ch->value.int_value);
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_string:
                printf("string: ");
                status = sysparam_get_string(ch->description, &string_value);
                if (status == SYSPARAM_OK) {
                        ch->value = HOMEKIT_STRING(string_value);
                        printf("%s\n", string_value);
                        homekit_characteristic_bounds_check(ch);
                }
                break;
        case homekit_format_float:
                printf("float: ");
                load_float_param ( ch->description, &ch->value.float_value);
                homekit_characteristic_bounds_check(ch);
                break;
        case homekit_format_uint64:
        case homekit_format_tlv:
        default:
                printf ("%s: Unknown characteristic format\n", __func__);
        }
        if (status != SYSPARAM_OK) {
                printf ("%s: Error in sysparams error:%i loading characteristic\n", __func__, status);
        }
}
