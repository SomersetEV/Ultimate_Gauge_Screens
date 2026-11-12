#include "Arduino.h"
#include "CANBus_Driver.h"
#include <stdio.h>

void CANBus_Init(void) {

  // Configure TWAI (CAN)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO, CAN_RX_GPIO, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = 32;   // default is 5: too small for a busy bus with an accept-all filter
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();  // Accept all IDs
   /*
    twai_filter_config_t f_config = {
    .acceptance_code = ((uint32_t)0x55B) << 21,     // Shift ID to the MSB bits as expected by TWAI filter
    .acceptance_mask = 0xFFE00000,         // Mask to match all 11 bits of the standard ID
    .single_filter = true                   // Use single filter mode
};
 */
    // Install and start TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Serial.println("TWAI driver installed.");
    } else {
        Serial.println("Failed to install TWAI driver.");
        ESP.restart();        // reboot ESP32
        while (1);
    }

    if (twai_start() == ESP_OK) {
        Serial.println("TWAI driver started. Listening for messages...");
    } else {
        Serial.println("Failed to start TWAI driver.");
        ESP.restart();        // reboot ESP32
        while (1);
    }

   // ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
   // ESP_ERROR_CHECK(twai_start());
}