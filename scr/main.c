/** Copyright 2019 Achim Pieters | StudioPieters®

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

   Build upon: ESP-HomeKit - MIT License - Copyright (c) 2017 Maxim Kulkin
 **/

#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include <button.h>
#include "ota-api.h"
#include "characteristic_types.h"
#include <sysparam.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define POSITION_OPEN 100
#define POSITION_CLOSED 0
#define POSITION_STATE_CLOSING 0
#define POSITION_STATE_OPENING 1
#define POSITION_STATE_STOPPED 2

#define BUTTON_UP 12
#define BUTTON_DOWN 13
#define DIR 14
#define STEP 4
#define ENABLE 5

#define LED_INBUILT_GPIO 2  // this is the onboard LED used to show on/off only
bool led_on = false;

TaskHandle_t updateStateTask;
homekit_characteristic_t current_position;
homekit_characteristic_t target_position;
homekit_characteristic_t position_state;
homekit_accessory_t *accessories[];

void led_write(bool on) {
        gpio_write(LED_INBUILT_GPIO, on ? 0 : 1);
}

void led_init() {
        gpio_enable(LED_INBUILT_GPIO, GPIO_OUTPUT);
        led_write(led_on);
}

void window_covering_identify_task(void *_args) {
        for (int i=0; i<3; i++) {
                for (int j=0; j<2; j++) {
                        led_write(true);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        led_write(false);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                vTaskDelay(250 / portTICK_PERIOD_MS);
        }
        led_write(led_on);
        vTaskDelete(NULL);
}

void window_covering_identify(homekit_value_t _value) {
        printf("Window Covering identify\n");
        xTaskCreate(window_covering_identify_task, "Window Covering identify", 128, NULL, 2, NULL);
}

void reset_configuration_task() {
//Flash the LED first before we start the reset
        for (int i=0; i<3; i++) {
                led_write(true);
                vTaskDelay(100 / portTICK_PERIOD_MS);
                led_write(false);
                vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        printf("Resetting Wifi Config\n");
        wifi_config_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Resetting HomeKit Config\n");
        homekit_server_reset();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Restarting\n");
        sdk_system_restart();
        vTaskDelete(NULL);
}

void reset_configuration() {
        printf("Resetting Window Covering configuration\n");
        xTaskCreate(reset_configuration_task, "Reset Window Covering", 256, NULL, 2, NULL);
}

void target_position_changed();
void stepper_write(int stepper, bool on) {
        gpio_write(stepper, on ? 1 : 0);
}

int stepcount = 0;
int multiplier = 0;
homekit_characteristic_t factor = HOMEKIT_CHARACTERISTIC_( CUSTOM_FACTOR, 0 );

void calibration_task() {
        if (stepcount == 0)
        {
                multiplier = 200;
                printf("Multiplier set to 200 \n");
        }
        else {
                stepcount = 0;
                printf("Set Stepcount to 0!\n");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                multiplier = 200;
                printf("Multiplier set to 200 \n");
        }
        vTaskDelete(NULL);
}

void calibration() {
        printf("Window Covering calibration\n");
        xTaskCreate(calibration_task, "Calibrate Window Covering", 256, NULL, 2, NULL);
}

void save_calibration_task() {
        factor.value.float_value = (stepcount/100);
        save_characteristic_to_flash (&factor, factor.value);
        printf("Save calibration values to flash\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Restarting\n");
        sdk_system_restart();
        vTaskDelete(NULL);
}

void save_calibration() {
        printf("Window Covering calibration\n");
        xTaskCreate(save_calibration_task, "Calibrate Window Covering", 256, NULL, 2, NULL);
}

void load_settings_from_flash (){
        printf("%s: Start, Freep Heap=%d\n", __func__, xPortGetFreeHeapSize());
        load_characteristic_from_flash(&factor);
        multiplier = factor.value.float_value;
        printf("Multiplier value %d\n", multiplier);
        printf("%s: End, Freep Heap=%d\n", __func__, xPortGetFreeHeapSize());
}


void motor_write(int state) {
        switch (state) {
        case POSITION_STATE_CLOSING:
                gpio_write(ENABLE, 0);
                gpio_write(DIR, 0);
                for (int i=0; i<multiplier; i++) {
                        gpio_write(STEP, 1);
                        sdk_os_delay_us(250);
                        gpio_write(STEP, 0);
                        sdk_os_delay_us(250);
                        stepcount--;
                        printf("Step count %d\n", stepcount);

                }
                break;
        case POSITION_STATE_OPENING:
                gpio_write(ENABLE, 0);
                gpio_write(DIR, 1);
                for (int i=0; i<(float)multiplier; i++) {
                        gpio_write(STEP, 1);
                        sdk_os_delay_us(250);
                        gpio_write(STEP, 0);
                        sdk_os_delay_us(250);
                        stepcount++;
                        printf("Step Count %d\n", stepcount);
                }
                break;
        default:
                gpio_write(ENABLE, 1);
                gpio_write(DIR, 0);
        }
}

void gpio_init() {
        gpio_enable(LED_INBUILT_GPIO, GPIO_OUTPUT);
        led_write(false);

        gpio_enable(BUTTON_UP, GPIO_INPUT);
        gpio_enable(BUTTON_DOWN, GPIO_INPUT);

        gpio_enable(DIR, GPIO_OUTPUT);
        gpio_enable(STEP, GPIO_OUTPUT);
        gpio_enable(ENABLE, GPIO_OUTPUT);
        stepper_write(DIR, false);
        stepper_write(STEP, false);
        stepper_write(ENABLE, true);
}

void button_up_callback(button_event_t event, void* context) {
        switch (event) {
        case button_event_single_press:
                printf("single press\n");
                if (position_state.value.int_value != POSITION_STATE_STOPPED) { // if moving, stop
                        target_position.value.int_value = current_position.value.int_value;
                        target_position_changed();
                }
                else {
                        {
                                target_position.value.int_value = POSITION_OPEN;
                                target_position_changed();
                        }
                }
                break;
        case button_event_double_press:
                printf("double press\n");
                break;
        case button_event_tripple_press:
                printf("tripple press\n");
                break;
        case button_event_long_press:
                printf("long press\n");
                calibration();
                break;
        }
}

void button_down_callback(button_event_t event, void* context) {
        switch (event) {
        case button_event_single_press:
                printf("single press\n");
                if (position_state.value.int_value != POSITION_STATE_STOPPED) { // if moving, stop
                        target_position.value.int_value = current_position.value.int_value;
                        target_position_changed();
                }
                else {
                        {
                                target_position.value.int_value = POSITION_CLOSED;
                                target_position_changed();
                        }
                }
                break;
        case button_event_double_press:
                printf("double press\n");
                break;
        case button_event_tripple_press:
                printf("tripple press\n");
                save_calibration();
                break;
        case button_event_long_press:
                printf("long press\n");
                reset_configuration();
                break;
        }
}

void update_state() {
        while (true) {
                printf("update_state\n");
                motor_write(position_state.value.int_value);
                uint8_t position = current_position.value.int_value;
                int8_t direction = position_state.value.int_value == POSITION_STATE_OPENING ? 1 : -1;
                int16_t newPosition = position + direction;

                current_position.value.int_value = newPosition;
                homekit_characteristic_notify(&current_position, current_position.value);

                if (newPosition == target_position.value.int_value) {
                        printf("reached destination %u\n", newPosition);
                        position_state.value.int_value = POSITION_STATE_STOPPED;
                        motor_write(position_state.value.int_value);
                        homekit_characteristic_notify(&position_state, position_state.value);
                        vTaskSuspend(updateStateTask);
                }
        }
}

void update_state_init() {
        xTaskCreate(update_state, "UpdateState", 256, NULL, tskIDLE_PRIORITY, &updateStateTask);
        vTaskSuspend(updateStateTask);
}

void on_update_target_position(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
        target_position_changed();
}

void target_position_changed(){
        printf("Update target position to: %u\n", target_position.value.int_value);

        if (target_position.value.int_value == current_position.value.int_value) {
                printf("Current position equal to target. Stopping.\n");
                position_state.value.int_value = POSITION_STATE_STOPPED;
                motor_write(position_state.value.int_value);
                homekit_characteristic_notify(&position_state, position_state.value);
                vTaskSuspend(updateStateTask);
        } else {
                position_state.value.int_value = target_position.value.int_value > current_position.value.int_value
                                                 ? POSITION_STATE_OPENING : POSITION_STATE_CLOSING;
                homekit_characteristic_notify(&position_state, position_state.value);
                vTaskResume(updateStateTask);
        }
}

homekit_characteristic_t current_position = { HOMEKIT_DECLARE_CHARACTERISTIC_CURRENT_POSITION(POSITION_CLOSED) };
homekit_characteristic_t target_position = { HOMEKIT_DECLARE_CHARACTERISTIC_TARGET_POSITION(POSITION_CLOSED, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(on_update_target_position)) };
homekit_characteristic_t position_state = { HOMEKIT_DECLARE_CHARACTERISTIC_POSITION_STATE(POSITION_STATE_STOPPED) };

#define DEVICE_NAME "Windows Covering System"
#define DEVICE_MANUFACTURER "StudioPieters®"
// See Naming convention.md
#define DEVICE_SERIAL "NLDA4SQN1466"
// See Naming convention.md
#define DEVICE_MODEL "SD466NL/A"
#define FW_VERSION "2.7.1"

homekit_characteristic_t ota_trigger  = API_OTA_TRIGGER;
homekit_characteristic_t name         = HOMEKIT_CHARACTERISTIC_(NAME, DEVICE_NAME);
homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER,  DEVICE_MANUFACTURER);
homekit_characteristic_t serial       = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, DEVICE_SERIAL);
homekit_characteristic_t model        = HOMEKIT_CHARACTERISTIC_(MODEL, DEVICE_MODEL);
homekit_characteristic_t revision     = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION,  FW_VERSION);

homekit_accessory_t *accessories[] = {
        HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_window_covering, .services=(homekit_service_t*[]) {
                HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
                        &name,
                        &manufacturer,
                        &serial,
                        &model,
                        &revision,
                        HOMEKIT_CHARACTERISTIC(IDENTIFY, window_covering_identify),
                        NULL
                }),
                HOMEKIT_SERVICE(WINDOW_COVERING, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
                        HOMEKIT_CHARACTERISTIC(NAME, "Windows Covering System"),
                        &current_position,
                        &target_position,
                        &position_state,
                        &ota_trigger,
                        NULL
                }),
                NULL
        }),
        NULL
};

// tools/gen_qrcode 14 734-45-419 7VQH qrcode.png
homekit_server_config_t config = {
        .accessories = accessories,
        .password = "123-45-678",
        .setupId="1QJ8",
};

void create_accessory_name() {
        int serialLength = snprintf(NULL, 0, "%d", sdk_system_get_chip_id());
        char *serialNumberValue = malloc(serialLength + 1);
        snprintf(serialNumberValue, serialLength + 1, "%d", sdk_system_get_chip_id());
        int name_len = snprintf(NULL, 0, "%s-%s", DEVICE_NAME, serialNumberValue);
        if (name_len > 63) { name_len = 63; }
        char *name_value = malloc(name_len + 1);
        snprintf(name_value, name_len + 1, "%s-%s", DEVICE_NAME, serialNumberValue);
        name.value = HOMEKIT_STRING(name_value);
}

void on_wifi_ready() {
}

void user_init(void) {
        uart_set_baud(0, 115200);
        load_settings_from_flash ();
        gpio_init();
        update_state_init();
        create_accessory_name();
        homekit_server_init(&config);

        button_config_t config = BUTTON_CONFIG(
                button_active_low,
                .long_press_time = 4000,
                .max_repeat_presses = 3,
                );

        int u = button_create(BUTTON_UP, config, button_up_callback, NULL);
        if (u) {
                printf("Failed to initialize a button\n");
        }
        int d = button_create(BUTTON_DOWN, config, button_down_callback, NULL);
        if (d) {
                printf("Failed to initialize a button\n");
        }
}
