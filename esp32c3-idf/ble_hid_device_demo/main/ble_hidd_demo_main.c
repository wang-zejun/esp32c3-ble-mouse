/* This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this software is
   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_hidd_prf_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "driver/gpio.h"
#include "hid_dev.h"



#include "esp_log.h"
#include "unity.h"
#include "iot_button.h"

static const char *TAG = "BUTTON";

#define BUTTON_CENTER  4
#define BUTTON_UP      9
#define BUTTON_DOWN    5
#define BUTTON_LEFT    8
#define BUTTON_RIGHT   13

#define BUTTON_ACTIVE_LEVEL   0
#define BUTTON_NUM 16

static button_handle_t g_btns[BUTTON_NUM] = {0};

static int get_btn_index(button_handle_t btn)
{
    for (size_t i = 0; i < BUTTON_NUM; i++) {
        if (btn == g_btns[i]) {
            return i;
        }
    }
    return -1;
}

/**
 * Brief:
 * This example Implemented BLE HID device profile related functions, in which the HID device
 * has 4 Reports (1 is mouse, 2 is keyboard and LED, 3 is Consumer Devices, 4 is Vendor devices).
 * Users can choose different reports according to their own application scenarios.
 * BLE HID profile inheritance and USB HID class.
 */

/**
 * Note:
 * 1. Win10 does not support vendor report , So SUPPORT_REPORT_VENDOR is always set to FALSE, it defines in hidd_le_prf_int.h
 * 2. Update connection parameters are not allowed during iPhone HID encryption, slave turns
 * off the ability to automatically update connection parameters during encryption.
 * 3. After our HID device is connected, the iPhones write 1 to the Report Characteristic Configuration Descriptor,
 * even if the HID encryption is not completed. This should actually be written 1 after the HID encryption is completed.
 * we modify the permissions of the Report Characteristic Configuration Descriptor to `ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE_ENCRYPTED`.
 * if you got `GATT_INSUF_ENCRYPTION` error, please ignore.
 */

#define HID_DEMO_TAG "HID_DEMO"


static uint16_t hid_conn_id = 0;
static bool sec_conn = false;
static bool send_volum_up = false;
#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param);

#define HIDD_DEVICE_NAME            "ESP32C3-HID"
static uint8_t hidd_service_uuid128[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00,
};

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x03c0,       //HID Generic,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(hidd_service_uuid128),
    .p_service_uuid = hidd_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x30,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                //esp_bd_addr_t rand_addr = {0x04,0x11,0x11,0x11,0x11,0x05};
                esp_ble_gap_set_device_name(HIDD_DEVICE_NAME);
                esp_ble_gap_config_adv_data(&hidd_adv_data);

            }
            break;
        }
        case ESP_BAT_EVENT_REG: {
            break;
        }
        case ESP_HIDD_EVENT_DEINIT_FINISH:
	     break;
		case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_CONNECT");
            hid_conn_id = param->connect.conn_id;
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            sec_conn = false;
            ESP_LOGI(HID_DEMO_TAG, "ESP_HIDD_EVENT_BLE_DISCONNECT");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
            ESP_LOGI(HID_DEMO_TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
            ESP_LOG_BUFFER_HEX(HID_DEMO_TAG, param->vendor_write.data, param->vendor_write.length);
        }
        default:
            break;
    }
    return;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        break;
     case ESP_GAP_BLE_SEC_REQ_EVT:
        for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
             ESP_LOGD(HID_DEMO_TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
	 break;
     case ESP_GAP_BLE_AUTH_CMPL_EVT:
        sec_conn = true;
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(HID_DEMO_TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(HID_DEMO_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(HID_DEMO_TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(HID_DEMO_TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

void hid_demo_task(void *pvParameters)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    while(1) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        // if (sec_conn) {
        //     ESP_LOGI(HID_DEMO_TAG, "Send the volume");
        //     send_volum_up = true;
        //     //uint8_t key_vaule = {HID_KEY_A};
        //     //esp_hidd_send_keyboard_value(hid_conn_id, 0, &key_vaule, 1);
        //     //esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_UP, true);
        //     vTaskDelay(3000 / portTICK_PERIOD_MS);
        //     if (send_volum_up) {
        //         send_volum_up = false;
        //         //esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_UP, false);
        //         //esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_DOWN, true);
        //         vTaskDelay(3000 / portTICK_PERIOD_MS);
        //         //esp_hidd_send_consumer_value(hid_conn_id, HID_CONSUMER_VOLUME_DOWN, false);
        //     }
        // }
    }
}

static void button_event(button_handle_t btn)
{
    uint8_t key_vaule = 0, key_none = 0, x = 0;
    uint8_t mouse_report = 0;
    button_event_t evt = get_btn_index(btn);
    switch (evt) {
    case 0:
        ESP_LOGI(TAG, "BUTTON_CENTER");
        esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 0, 0);
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 0, 0);
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
        break;
    case 1:
        ESP_LOGI(TAG, "BUTTON_UP");  
        mouse_report |= (1<<0);
        esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 0, 0);
        for(unsigned int i = 0; i < 30 ; i++) 
        {
            esp_hidd_send_mouse_value(hid_conn_id, 1, 1, -10, 0);
        }
        mouse_report &= ~(1<<0);
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        for(unsigned int i = 0; i < 30 ; i++) 
        {
            esp_hidd_send_mouse_value(hid_conn_id, 0, -1, 10, 0);
        }
        break;
    case 2:
        ESP_LOGI(TAG, "BUTTON_DOWN");
        mouse_report |= (1<<0);
        esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 0, 0);
        for(unsigned int i = 0; i < 30 ; i++) 
        {
            esp_hidd_send_mouse_value(hid_conn_id, 1, 0, 10, 0);
        }
        mouse_report &= ~(1<<0);
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
        for(unsigned int i = 0; i < 30 ; i++) 
        {
            esp_hidd_send_mouse_value(hid_conn_id, 0, 0, -10, 0);
        }
        break;
    case 3:
        ESP_LOGI(TAG, "BUTTON_LEFT");
        // for(unsigned int i = 0; i < 100 ; i++) 
        // {
        //     esp_hidd_send_mouse_value(hid_conn_id, 0, -1, 0, 0);
        // }
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
        esp_hidd_send_mouse_value(hid_conn_id, HID_MOUSE_LEFT, 0, 0, 0);
        esp_hidd_send_mouse_value(hid_conn_id, 0, 0, 0, 0);
        break;
    case 4:
        ESP_LOGI(TAG, "BUTTON_RIGHT");
        for(unsigned int i = 0; i < 100 ; i++) 
        {
            esp_hidd_send_mouse_value(hid_conn_id, 0, 1, 0, 0);
        }

        break;

    default:
        break;
    }
}

static void button_press_down_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_PRESS_DOWN, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_DOWN", get_btn_index((button_handle_t)arg));
}

static void button_press_up_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_PRESS_UP, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_UP", get_btn_index((button_handle_t)arg));
}

static void button_press_repeat_cb(void *arg)
{
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_REPEAT[%d]", get_btn_index((button_handle_t)arg), iot_button_get_repeat((button_handle_t)arg));
}

static void button_single_click_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_SINGLE_CLICK, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
    button_event((button_handle_t)arg);
}

static void button_double_click_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_DOUBLE_CLICK, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_DOUBLE_CLICK", get_btn_index((button_handle_t)arg));
}

static void button_long_press_start_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_LONG_PRESS_START, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_START", get_btn_index((button_handle_t)arg));
}

static void button_long_press_hold_cb(void *arg)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_LONG_PRESS_HOLD, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_HOLD", get_btn_index((button_handle_t)arg));
}


void app_main(void)
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s enable controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
        return;
    }

    if((ret = esp_hidd_profile_init()) != ESP_OK) {
        ESP_LOGE(HID_DEMO_TAG, "%s init bluedroid failed\n", __func__);
    }

    ///register the callback function to the gap module
    esp_ble_gap_register_callback(gap_event_handler);
    esp_hidd_register_callbacks(hidd_event_callback);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    button_config_t cfg_center = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = BUTTON_CENTER,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    g_btns[0] = iot_button_create(&cfg_center);
    TEST_ASSERT_NOT_NULL(g_btns[0]);
    iot_button_register_cb(g_btns[0], BUTTON_SINGLE_CLICK, button_single_click_cb);
    iot_button_register_cb(g_btns[0], BUTTON_DOUBLE_CLICK, button_double_click_cb);

    button_config_t cfg_up = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = BUTTON_UP,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    g_btns[1] = iot_button_create(&cfg_up);
    TEST_ASSERT_NOT_NULL(g_btns[1]);
    iot_button_register_cb(g_btns[1], BUTTON_SINGLE_CLICK, button_single_click_cb);
    iot_button_register_cb(g_btns[1], BUTTON_DOUBLE_CLICK, button_double_click_cb);

    button_config_t cfg_down = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = BUTTON_DOWN,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    g_btns[2] = iot_button_create(&cfg_down);
    TEST_ASSERT_NOT_NULL(g_btns[2]);
    iot_button_register_cb(g_btns[2], BUTTON_SINGLE_CLICK, button_single_click_cb);
    iot_button_register_cb(g_btns[2], BUTTON_DOUBLE_CLICK, button_double_click_cb);

    button_config_t cfg_left = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = BUTTON_LEFT,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    g_btns[3] = iot_button_create(&cfg_left);
    TEST_ASSERT_NOT_NULL(g_btns[3]);
    iot_button_register_cb(g_btns[3], BUTTON_SINGLE_CLICK, button_single_click_cb);
    iot_button_register_cb(g_btns[3], BUTTON_DOUBLE_CLICK, button_double_click_cb);

    button_config_t cfg_right = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = BUTTON_RIGHT,
            .active_level = BUTTON_ACTIVE_LEVEL,
        },
    };
    g_btns[4] = iot_button_create(&cfg_right);
    TEST_ASSERT_NOT_NULL(g_btns[4]);
    iot_button_register_cb(g_btns[4], BUTTON_SINGLE_CLICK, button_single_click_cb);
    iot_button_register_cb(g_btns[4], BUTTON_DOUBLE_CLICK, button_double_click_cb);

    xTaskCreate(&hid_demo_task, "hid_task", 2048, NULL, 5, NULL);
}
