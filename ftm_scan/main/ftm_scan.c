#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "nvs_flash.h"
#include "cmd_system.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_console.h"
#include "esp_mac.h"
#include "esp_task_wdt.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"

static const char *TAG_STA = "ftm_scanner";

static EventGroupHandle_t s_ftm_event_group;
static const int FTM_REPORT_BIT = BIT0;
static const int FTM_FAILURE_BIT = BIT1;
wifi_ftm_report_entry_t *s_ftm_report;
static uint8_t s_ftm_report_num_entries;
static uint32_t s_rtt_est, s_dist_est;

static char *ftm_ssid;

uint16_t g_scan_ap_num;
wifi_ap_record_t *g_ap_list_buffer;

//static uint8_t INTERVAL = 1000; //10 sec wait
const TickType_t INTERVAL = 10000 / portTICK_PERIOD_MS;

const int g_report_lvl =
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_DIAG
    BIT0 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_RTT
    BIT1 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_T1T2T3T4
    BIT2 |
#endif
#ifdef CONFIG_ESP_FTM_REPORT_SHOW_RSSI
    BIT3 |
#endif
0;

//event handler for ESP32
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
  //if (event_id == WIFI_EVENT_FTM_REPORT) {
    int i;  
    int totalRss;
    float meanRss;
    wifi_event_ftm_report_t *event = (wifi_event_ftm_report_t *) event_data;

    ESP_LOGI(TAG_STA, "FTM HANDLER");
    if (event->status == FTM_STATUS_SUCCESS) {
        s_rtt_est = event->rtt_est;
        s_dist_est = event->dist_est;
        s_ftm_report = event->ftm_report_data;
        s_ftm_report_num_entries = event->ftm_report_num_entries;

        totalRss = 0;
        for (i = 0; i < s_ftm_report_num_entries; i++) {
            totalRss = totalRss +s_ftm_report[i].rssi;
        }

        meanRss = totalRss/s_ftm_report_num_entries;

        //X = emxCreate_real_T(1, 2);
        //X->data[0] = normalize(event->rtt_raw, MIN_RTT, MAX_RTT);
        //X->data[1] = normalize(meanRss, MIN_RSS, MAX_RSS);
        //emxInitArray_real_T(&result, 1);
        //predict(X, result);

        //resultCm = (int) (result->data[0]*100.0);
        //fprintf(stdout,"(%f , %f)", X->data[0], X->data[1]);

        ESP_LOGI(TAG_STA, "%s", ftm_ssid);

        /*
        fprintf(stdout,"{\"mac\":\""MACSTR"\", \"rtt_est\":%lu, \"rtt_raw\":%lu , \"dist_est\":%lu, \"num_frames\":%d, \"frames\":[", MAC2STR(event->peer_mac), event->rtt_est, event->rtt_raw, event->dist_est, event->ftm_report_num_entries);
        for (i = 0; i < s_ftm_report_num_entries; i++) {
            fprintf(stdout,"{\"rtt\":%lu, \"rssi\":%d, \"t1\":%lld, \"t2\":%lld, \"t3\":%lld, \"t4\":%lld}",s_ftm_report[i].rtt, s_ftm_report[i].rssi, s_ftm_report[i].t1, s_ftm_report[i].t2, s_ftm_report[i].t3, s_ftm_report[i].t4); 
            
            if (i<s_ftm_report_num_entries-1){
                fprintf(stdout,",");
            }
        }
        fprintf(stdout,"]}");
        fprintf(stdout,"\n");
        fflush(stdout);
        */
        fprintf(stdout,"{\"ssid\": \"%s\", \"mac\": \""MACSTR"\", \"rtt_est\": %lu, \"rtt_raw\": %lu , \"dist_est\": %lu, \"num_frames\": %d, \"mean_rssi\": %.2f}", ftm_ssid, MAC2STR(event->peer_mac), event->rtt_est, event->rtt_raw, event->dist_est, event->ftm_report_num_entries, meanRss);
        fprintf(stdout,"\n");
        fflush(stdout);

        xEventGroupSetBits(s_ftm_event_group, FTM_REPORT_BIT);
        //vTaskDelay(20);
    } else {
        if (event->status == FTM_STATUS_UNSUPPORTED){
                    ESP_LOGI(TAG_STA, "FTM procedure with Peer("MACSTR") failed! (Status - FTM_STATUS_UNSUPPORTED)",
                MAC2STR(event->peer_mac));
        } else if (event->status == FTM_STATUS_CONF_REJECTED){
                    ESP_LOGI(TAG_STA, "FTM procedure with Peer("MACSTR") failed! (Status - FTM_STATUS_CONF_REJECTED)",
                MAC2STR(event->peer_mac));
        } else if (event->status == FTM_STATUS_NO_RESPONSE){
                    ESP_LOGI(TAG_STA, "FTM procedure with Peer("MACSTR") failed! (Status - FTM_STATUS_NO_RESPONSE)",
                MAC2STR(event->peer_mac));
        } else if (event->status == FTM_STATUS_FAIL){
                    ESP_LOGI(TAG_STA, "FTM procedure with Peer("MACSTR") failed! (Status - FTM_STATUS_FAIL)",
                MAC2STR(event->peer_mac));
        }

        xEventGroupSetBits(s_ftm_event_group, FTM_FAILURE_BIT);
        //vTaskDelay(20);
    }
    //}
}

//initializa wifi
void wifiInit() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(nvs_flash_init());
  //tcpip_adapter_init();
  s_ftm_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    WIFI_EVENT_FTM_REPORT,
                    &event_handler,
                    NULL,
                    NULL));

  wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

}

static void ftm_process_report(void)
{
    int i;
    char *log = NULL;

    if (!g_report_lvl)
        return;

    log = malloc(200);

    if (!log) {
        ESP_LOGI(TAG_STA, "Failed to alloc buffer for FTM report");
        return;
    }

    bzero(log, 200);
    sprintf(log, "%s%s%s%s", g_report_lvl & BIT0 ? " Diag |":"", g_report_lvl & BIT1 ? "   RTT   |":"",
                 g_report_lvl & BIT2 ? "       T1       |       T2       |       T3       |       T4       |":"",
                 g_report_lvl & BIT3 ? "  RSSI  |":"");
    ESP_LOGI(TAG_STA, "FTM Report:");
    ESP_LOGI(TAG_STA, "|%s", log);
    for (i = 0; i < s_ftm_report_num_entries; i++) {
        char *log_ptr = log;

        bzero(log, 200);
        if (g_report_lvl & BIT0) {
            log_ptr += sprintf(log_ptr, "%6d|", s_ftm_report[i].dlog_token);
        }
        if (g_report_lvl & BIT1) {
            log_ptr += sprintf(log_ptr, "%7lu  |", s_ftm_report[i].rtt);
        }
        if (g_report_lvl & BIT2) {
            log_ptr += sprintf(log_ptr, "%14llu  |%14llu  |%14llu  |%14llu  |", s_ftm_report[i].t1,
                                        s_ftm_report[i].t2, s_ftm_report[i].t3, s_ftm_report[i].t4);
        }
        if (g_report_lvl & BIT3) {
            log_ptr += sprintf(log_ptr, "%6d  |", s_ftm_report[i].rssi);
        }
        ESP_LOGI(TAG_STA, "|%s", log);
    }
    free(log);
}

void wifi_ftm_req (wifi_ap_record_t ap_record) {
  
    wifi_ftm_initiator_cfg_t ftm_cfg = {
        .frm_count = 16,
        .burst_period = 2
    };  

    ESP_LOGI(TAG_STA, "Inside FTM req");

    EventBits_t bits;

    ESP_LOGI(TAG_STA, "Requesting FTM session with Frm Count - %d, Burst Period - %d mSec", ftm_cfg.frm_count, ftm_cfg.burst_period*100);

    memcpy(ftm_cfg.resp_mac, ap_record.bssid, 6);
    ftm_cfg.channel = ap_record.primary;
    
    //printf("%X:%X:%X:%X:%X:%X\n", ftm_cfg.resp_mac[0], ftm_cfg.resp_mac[1], ftm_cfg.resp_mac[2], ftm_cfg.resp_mac[3], ftm_cfg.resp_mac[4], ftm_cfg.resp_mac[5]);

    if (ESP_OK != esp_wifi_ftm_initiate_session(&ftm_cfg)) {
        ESP_LOGI(TAG_STA, "Failed to start FTM session");
    }
    
    ESP_LOGI(TAG_STA, "ftm req sent");
    //println(FTM_REPORT_BIT);
    bits = xEventGroupWaitBits(s_ftm_event_group, FTM_REPORT_BIT | FTM_FAILURE_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    
    /* Processing data from FTM session */
    if (bits & FTM_REPORT_BIT) {
        //ftm_process_report();
        free(s_ftm_report);
        s_ftm_report = NULL;
        s_ftm_report_num_entries = 0;
        ESP_LOGI(TAG_STA, "Estimated RTT - %lu nSec, Estimated Distance - %lu.%2lu meters", s_rtt_est, s_dist_est / 100, s_dist_est % 100);
    } else {
        /* Failure case */
        ESP_LOGI(TAG_STA, "FAILED FTM");
    }
}

//wifi scan function
void wifi_perform_scan(){
  wifi_scan_config_t scan_config = { 0 };
  uint8_t i;
  //uint16_t g_scan_ap_num = 0;
  uint8_t ftm_ap_num = 0;

  esp_wifi_scan_start(&scan_config, true);

  esp_wifi_scan_get_ap_num(&g_scan_ap_num);
  if (g_scan_ap_num == 0) {
      ESP_LOGI(TAG_STA, "No matching AP found");
  }
  
  g_ap_list_buffer = malloc(g_scan_ap_num * sizeof(wifi_ap_record_t));
  //wifi_ap_record_t g_ap_list_buffer[g_scan_ap_num];
  //memset(g_ap_list_buffer, 0, sizeof(g_ap_list_buffer));
  
  if (g_ap_list_buffer == NULL) {
      ESP_LOGI(TAG_STA, "Failed to alloc buffer to print scan results");
  }
    
  if (esp_wifi_scan_get_ap_records(&g_scan_ap_num, g_ap_list_buffer) == ESP_OK) {
    ESP_LOGI(TAG_STA, "              SSID             | Channel | RSSI | FTM |         MAC        ");
    ESP_LOGI(TAG_STA, "***************************************************************************");
    for (i = 0; i < g_scan_ap_num; i++) {
        ESP_LOGI(TAG_STA, "%30s | %7d | %4d | %3d | %02X:%02X:%02X:%02X:%02X:%02X", (char *)g_ap_list_buffer[i].ssid, g_ap_list_buffer[i].primary, g_ap_list_buffer[i].rssi, g_ap_list_buffer[i].ftm_responder, g_ap_list_buffer[i].bssid[0], g_ap_list_buffer[i].bssid[1], g_ap_list_buffer[i].bssid[2], g_ap_list_buffer[i].bssid[3], g_ap_list_buffer[i].bssid[4], g_ap_list_buffer[i].bssid[5]);
        vTaskDelay(5);
        if (g_ap_list_buffer[i].ftm_responder == 1){
          ftm_ap_num++;  
          ftm_ssid = (char *)g_ap_list_buffer[i].ssid;
          wifi_ftm_req(g_ap_list_buffer[i]);
        }
    }
    ESP_LOGI(TAG_STA, "***************************************************************************");
    ESP_LOGI(TAG_STA, "%d FTM SSID found", ftm_ap_num);  
  }  

  ESP_LOGI(TAG_STA, "WiFi scan done");
}

void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "ftm_scan>";

    #if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
        esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));

    #elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
        esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

    #elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
        esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));

    #else
    #error Unsupported console type
    #endif

    // start console REPL
    ESP_ERROR_CHECK(esp_console_start_repl(repl));


    //Serial.begin(115200);
    wifiInit();
    ESP_LOGI(TAG_STA, "ESP32S3 initialization done");

    while(1) {
        ESP_LOGI(TAG_STA, "Scanning");
        wifi_perform_scan();
        ESP_LOGI(TAG_STA, "Scan finished");
        xEventGroupClearBits(s_ftm_event_group, FTM_REPORT_BIT);
        ESP_LOGI(TAG_STA, "Bit cleared");
        vTaskDelay(INTERVAL);

        //Serial.println(ESP.getFreeHeap());
    }
}