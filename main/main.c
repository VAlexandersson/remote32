#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "wifi_manager.h"

#define LOG_LEVEL   ESP_LOG_VERBOSE
#define TAG "MAIN"

#define WIFI_SSID       CONFIG_WIFI_SSID
#define WIFI_PASSWORD   CONFIG_WIFI_PASSWORD

esp_err_t init_nvs(){
    esp_err_t status = nvs_flash_init();
    if (status == ESP_ERR_NVS_NO_FREE_PAGES || status == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        status = nvs_flash_erase();
        if (status == ESP_OK) {
            status = nvs_flash_init();
        }
    }
    return status;
}


void app_main() {    
    ESP_LOGI(TAG, "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(TAG, "Initializing NVS");
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_err_t status = ESP_OK;

    status = wifi_init(WIFI_SSID, WIFI_PASSWORD);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "wifi_init failed: %s", esp_err_to_name(status));
        return;
    }
}

/*
I (176) esp_image: segment 3: paddr=00040020 vaddr=400d0020 size=80a4ch (526924) map
I (357) esp_image: segment 4: paddr=000c0a74 vaddr=4008a944 size=0bd5ch ( 48476) load
I (387) boot: Loaded app from partition at offset 0x10000
I (388) boot: Disabling RNG early entropy source...
I (399) cpu_start: Multicore app
I (408) cpu_start: Pro cpu start user code
I (408) cpu_start: cpu freq: 160000000 Hz
I (408) cpu_start: Application information:
I (411) cpu_start: Project name:     IR_controller
I (416) cpu_start: App version:      8aaab8a-dirty
I (422) cpu_start: Compile time:     Oct  1 2024 12:32:37
I (428) cpu_start: ELF file SHA256:  c0d15bc05...
I (433) cpu_start: ESP-IDF:          v5.3-dev-1353-gb3f7e2c8a4
I (440) cpu_start: Min chip rev:     v0.0
I (445) cpu_start: Max chip rev:     v3.99 
I (449) cpu_start: Chip rev:         v3.1
I (454) heap_init: Initializing. RAM available for dynamic allocation:
I (462) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM
I (467) heap_init: At 3FFB7D60 len 000282A0 (160 KiB): DRAM
I (474) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM
I (480) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (486) heap_init: At 400966A0 len 00009960 (38 KiB): IRAM
I (494) spi_flash: detected chip: generic
I (497) spi_flash: flash io: dio
W (501) spi_flash: Detected size(4096k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
I (515) main_task: Started on CPU0
I (525) main_task: Calling app_main()
ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE) at 0x400de449
0x400de449: esp_netif_create_default_wifi_sta at /home/viktor/esp/esp-idf/components/esp_wifi/src/wifi_default.c:391 (discriminator 1)

file: "/IDF/components/esp_wifi/src/wifi_default.c" line 391
func: esp_netif_create_default_wifi_sta
expression: esp_wifi_set_default_wifi_sta_handlers()

abort() was called at PC 0x400891cf on core 0
0x400891cf: _esp_error_check_failed at /home/viktor/esp/esp-idf/components/esp_system/esp_err.c:50



Backtrace: 0x40081a22:0x3ffb9d80 0x400891d9:0x3ffb9da0 0x40090899:0x3ffb9dc0 0x400891cf:0x3ffb9e30 0x400de449:0x3ffb9e60 0x400d6890:0x3ffb9e90 0x400d6685:0x3ffba040 0x4014eadf:0x3ffba060 0x40089ce5:0x3ffba090
0x40081a22: panic_abort at /home/viktor/esp/esp-idf/components/esp_system/panic.c:472

0x400891d9: esp_system_abort at /home/viktor/esp/esp-idf/components/esp_system/port/esp_system_chip.c:93

0x40090899: abort at /home/viktor/esp/esp-idf/components/newlib/abort.c:38

0x400891cf: _esp_error_check_failed at /home/viktor/esp/esp-idf/components/esp_system/esp_err.c:50

0x400de449: esp_netif_create_default_wifi_sta at /home/viktor/esp/esp-idf/components/esp_wifi/src/wifi_default.c:391 (discriminator 1)

0x400d6890: wifi_init at /home/viktor/IR_controller/components/wifi_manager/wifi_manager.c:98

0x400d6685: app_main at /home/viktor/IR_controller/main/main.c:27

0x4014eadf: main_task at /home/viktor/esp/esp-idf/components/freertos/app_startup.c:208

0x40089ce5: vPortTaskWrapper at /home/viktor/esp/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:134





ELF file SHA256: c0d15bc05

Rebooting...
ets Jul 29 2019 12:21:46

rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0030,len:7172
load:0x40078000,len:15556
load:0x40080400,len:4
0x40080400: _init at ??:?
*/