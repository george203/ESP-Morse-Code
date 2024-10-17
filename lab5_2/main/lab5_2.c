/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "ADC";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
//ADC1 Channels
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_0

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

#define THRESHOLD 700

#define MAX_CHAR 6 // Max length of a Morse code character (e.g., "....." for '5')

// Structure to map Morse code to ASCII characters
typedef struct {
    const char *morse;
    char ascii;
} MorseCodeMap;

// Morse code mapping table
MorseCodeMap morseMap[] = {
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},  {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'}, {"..", 'I'},   {".---", 'J'},
    {"-.-", 'K'},  {".-..", 'L'}, {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'},
    {"----.", '9'}, {"/", ' '} // Add space for word separator
};

#define MORSE_MAP_SIZE (sizeof(morseMap) / sizeof(morseMap[0]))

// Function to find the corresponding ASCII character for a given Morse code
char morseToAscii(const char *morse) {
    for (int i = 0; i < MORSE_MAP_SIZE; i++) {
        if (strcmp(morseMap[i].morse, morse) == 0) {
            return morseMap[i].ascii;
        }
    }
    return '?'; // Return '?' for unknown Morse code
}

// Function to convert a Morse code string to an ASCII string
void morseToAsciiString(const char *morseCode, char *asciiString) {
    char morseChar[MAX_CHAR + 1]; // Temporary buffer for a Morse code character
    int morseCharIndex = 0; // Index for the current Morse code character
    int asciiIndex = 0; // Index for the resulting ASCII string

    for (int i = 0; morseCode[i] != '\0'; i++) {
        if (morseCode[i] == ' ') {
            if (morseCharIndex > 0) { // End of a Morse code character
                morseChar[morseCharIndex] = '\0';
                asciiString[asciiIndex++] = morseToAscii(morseChar);
                morseCharIndex = 0; // Reset the Morse code character buffer
            }
        } else if (morseCode[i] == '/') {
            if (morseCharIndex > 0) { // End of a Morse code character before the word separator
                morseChar[morseCharIndex] = '\0';
                asciiString[asciiIndex++] = morseToAscii(morseChar);
                morseCharIndex = 0; // Reset the Morse code character buffer
            }
            asciiString[asciiIndex++] = ' '; // Add space for word separator
        } else {
            morseChar[morseCharIndex++] = morseCode[i]; // Add dot or dash to the Morse code character buffer
        }
    }

    // Convert the last Morse code character if any
    if (morseCharIndex > 0) {
        morseChar[morseCharIndex] = '\0';
        asciiString[asciiIndex++] = morseToAscii(morseChar);
    }

    asciiString[asciiIndex] = '\0'; // Null-terminate the resulting ASCII string
}

static int adc_raw[2][10];
static int voltage[2][10];
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

void app_main(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);

    int high_count = 0;
    int low_count = 0;

    char message[200] = "";
    int msg_len = 0;

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0][0]));
        //ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0][0]);
        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0][0], &voltage[0][0]));
            //ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage[0][0]);
            if(voltage[0][0] >= THRESHOLD) {
                high_count++;
                if( (low_count > 18) && (low_count < 22) ) {
                    message[msg_len] = ' ';
                    msg_len++;
                }
                if( (low_count > 53) && (low_count < 57) ) {
                    message[msg_len] = ' ';
                    message[msg_len + 1] = '/';
                    message[msg_len + 2] = ' ';
                    msg_len += 3;
                }
                low_count = 0;
            }
            else {
                low_count++;
                if( (high_count > 3) && (high_count < 7)) {
                    message[msg_len] = '.';
                    msg_len++;
                }
                if( (high_count > 13) && (high_count < 17) ) {
                    message[msg_len] = '-';
                    msg_len++;
                }
                high_count = 0;
            }
            if((low_count > 80) && (msg_len > 0)) {
                message[msg_len] = '\0';
                ESP_LOGI(TAG, "Encrypted message: %s", message);
                char asciiString[100];
                morseToAsciiString(message, asciiString);
                ESP_LOGI(TAG, "Decrypted message: %s", asciiString);
                low_count = 0;
                msg_len = 0;
            }
            //ESP_LOGI(TAG, "low count: %d\thigh count: %d", low_count, high_count);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        example_adc_calibration_deinit(adc1_cali_chan0_handle);
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
