// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <light_driver.h>
#include <led_driver.h>

#include "app_priv.h"

#define LIGHT_CW_PWM_GPIO_COLD      (gpio_num_t)19  // Main light cold white channel
#define LIGHT_CW_PWM_GPIO_WARM      (gpio_num_t)20  // Main light warm white channel
#define LIGHT_C_PWM_GPIO_DEDICATED  (gpio_num_t)0   // Nightlight PWM channel

static const char *TAG = "app_driver";
static bool s_night_light_on = false;
static uint8_t s_night_light_brightness = 100;

static void app_driver_update_nightlight(void)
{
    uint8_t duty = s_night_light_on ? s_night_light_brightness : 0;
    led_driver_set_channel(LED_CHANNEL_BLUE, duty);
}

int app_driver_init()
{
    printf("%s: Initializing light driver\n", TAG);

    light_driver_config_t cfg = {
        .device_type = LIGHT_DEVICE_TYPE_LED,
        .channel_comb = LIGHT_CHANNEL_COMB_2CH_CW,
        .io_conf.led_io = {
            .cold = LIGHT_CW_PWM_GPIO_COLD,
            .warm = LIGHT_CW_PWM_GPIO_WARM,
        },
        .min_brightness = 0,
        .max_brightness = 100,
    };

    if (light_driver_init(&cfg) != 0) {
        printf("%s: Failed to initialize main light driver\n", TAG);
        return -1;
    }

    if (led_driver_regist_channel(LED_CHANNEL_BLUE, LIGHT_C_PWM_GPIO_DEDICATED) != 0) {
        printf("%s: Failed to register nightlight channel\n", TAG);
        return -1;
    }

    s_night_light_on = false;
    s_night_light_brightness = 100;
    app_driver_update_nightlight();

    light_driver_set_temperature(4000);
    light_driver_set_brightness(100);
    light_driver_set_power(1);

    return 0;
}

int app_driver_set_light_state(bool state)
{
    printf("%s: Setting main light state: %s\n", TAG, state ? "ON" : "OFF");
    return light_driver_set_power(state);
}

int app_driver_set_light_brightness(uint8_t brightness)
{
    uint8_t mapped_brightness = brightness * 100 / 255;
    printf("%s: Setting main light brightness: %d\n", TAG, mapped_brightness);
    return light_driver_set_brightness(mapped_brightness);
}

int app_driver_set_light_temperature(uint16_t temperature)
{
    uint32_t temperature_kelvin = 1000000 / temperature;
    printf("%s: Setting main light temperature: %d\n", TAG, temperature_kelvin);
    return light_driver_set_temperature(temperature_kelvin);
}

int app_driver_set_c_light_state(bool state)
{
    printf("%s: Setting nightlight state: %s\n", TAG, state ? "ON" : "OFF");
    s_night_light_on = state;
    app_driver_update_nightlight();
    return 0;
}

int app_driver_set_c_light_brightness(uint8_t brightness)
{
    s_night_light_brightness = brightness * 100 / 255;
    printf("%s: Setting nightlight brightness: %d\n", TAG, s_night_light_brightness);
    app_driver_update_nightlight();
    return 0;
}

int app_driver_event_handler(low_code_event_t *event)
{
    /* Get the events. Appropriate indicators should be shown to the user based on the event. */
    printf("%s: Received event: %d\n", TAG, event->event_type);
    light_effect_config_t effect_config = {
        .type = LIGHT_EFFECT_INVALID,
        .mode = LIGHT_WORK_MODE_WHITE,
        .max_brightness = 100,
        .min_brightness = 10
    };

    switch (event->event_type) {
        case LOW_CODE_EVENT_SETUP_MODE_START:
            printf("%s: Setup mode started\n", TAG);
            effect_config.type = LIGHT_EFFECT_BLINK;
            light_driver_effect_start(&effect_config, 2000, 120000);
            break;
        case LOW_CODE_EVENT_SETUP_MODE_END:
            printf("%s: Setup mode ended\n", TAG);
            light_driver_effect_stop();
            break;
        case LOW_CODE_EVENT_SETUP_DEVICE_CONNECTED:
            printf("%s: Device connected during setup\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_STARTED:
            printf("%s: Setup process started\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_SUCCESSFUL:
            printf("%s: Setup process successful\n", TAG);
            break;
        case LOW_CODE_EVENT_SETUP_FAILED:
            printf("%s: Setup process failed\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_CONNECTED:
            printf("%s: Network connected\n", TAG);
            break;
        case LOW_CODE_EVENT_NETWORK_DISCONNECTED:
            printf("%s: Network disconnected\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STARTED:
            printf("%s: OTA update started\n", TAG);
            break;
        case LOW_CODE_EVENT_OTA_STOPPED:
            printf("%s: OTA update stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_READY:
            printf("%s: Device is ready\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_START:
            printf("%s: Identification started\n", TAG);
            break;
        case LOW_CODE_EVENT_IDENTIFICATION_STOP:
            printf("%s: Identification stopped\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_LOW_CODE:
            printf("%s: Low code test mode is triggered for subtype: %d\n", TAG, (int)*((int*)(event->event_data)));
            break;
        case LOW_CODE_EVENT_TEST_MODE_COMMON:
            printf("%s: common test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_BLE:
            printf("%s: ble test mode triggered\n", TAG);
            break;
        case LOW_CODE_EVENT_TEST_MODE_SNIFFER:
            printf("%s: sniffer test mode triggered\n", TAG);
            break;
        default:
            printf("%s: Unhandled event type: %d\n", TAG, event->event_type);
            break;
    }

    return 0;
}
