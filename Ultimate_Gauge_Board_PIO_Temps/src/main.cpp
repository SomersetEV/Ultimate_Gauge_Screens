/* Temps screen - ESP32-S3 2.1" Ultimate Gauge Board               */
/* Based on the driver setup by Andy Valentine - Garage Tinkering  */
/*                                                                 */
/* Shows motor/inverter temps (0x126), battery temp and cell       */
/* voltages (0x373) and heater power (0x802).                      */

#include <Arduino.h>
#include "CANBus_Driver.h"
#include "LVGL_Driver.h"
#include "I2C_Driver.h"

#include "esp_system.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ui/ui.h"

#define TAG "TWAI"

// Backlight levels (see Set_Backlight for scaling)
#define BL_NORMAL  50
#define BL_DIM     25

// Startup timing
static const uint32_t FADE_START_MS = 5800;
static const uint32_t FADE_END_MS   = 6300;

// ---- Values written by the CAN task, read by loop() ----
// Battery data (CAN 0x373)
volatile int32_t g_min_cell_mv   = 0;   // mV
volatile int32_t g_max_cell_mv   = 0;   // mV
volatile int32_t g_battery_temp  = 0;   // °C (average of min/max)
// Motor/Inverter temps (CAN 0x126)
volatile int32_t g_inverter_temp = 0;   // °C
volatile int32_t g_motor_temp    = 0;   // °C
// Heater power (CAN 0x802)
volatile int32_t g_heater_power  = 0;   // W

volatile bool g_sidelights_on = false;  // CAN 0x801
volatile bool g_charge_mode   = false;  // Screen stays off while charging (CAN ID TBD)
volatile bool data_ready      = false;  // New incoming data

// ---- loop()-only state ----
bool g_fade_done = false;


void Drivers_Init(void) {
  I2C_Init();

  Serial.println("Scanning for TCA9554...");
  bool found = false;
  for (int attempt = 0; attempt < 10; attempt++) {
    if (I2C_ScanAddress(0x20)) { // 0x20 is default for TCA9554
      found = true;
      break;
    }
    delay(50);
  }

  if (!found) {
    Serial.println("TCA9554 not detected! Skipping expander init.");
  } else {
    TCA9554PWR_Init(0x00);
  }
  LCD_Init();
  Lvgl_Init();
}

// ---------------- CAN decoding (runs in the CAN task) ----------------

void Process_Motor_Inverter(const uint8_t *data) {
  g_inverter_temp = (int16_t)(data[4] | (data[5] << 8));
  g_motor_temp    = (int16_t)(data[6] | (data[7] << 8));
}

void Process_Battery(const uint8_t *data) {
  int minCell = data[0] | (data[1] << 8);   // mV
  int maxCell = data[2] | (data[3] << 8);
  int minTemp = data[4] | (data[5] << 8);   // Kelvin
  int maxTemp = data[6] | (data[7] << 8);

  g_min_cell_mv  = minCell;
  g_max_cell_mv  = maxCell;
  g_battery_temp = ((minTemp - 273) + (maxTemp - 273)) / 2;
}

void Process_HeaterPower(const uint8_t *data) {
  g_heater_power = (int32_t)(uint16_t)(data[2] | (data[3] << 8));
}

void Process_Sidelights(const uint8_t *data) {
  g_sidelights_on = (data[0] & 0x02) != 0;  // byte 0, bit 1
}

void Process_ChargeMode(const uint8_t *data) {
  // TODO: update byte/bit mask when CAN ID is confirmed
  g_charge_mode = (data[0] & 0x01) != 0;  // placeholder: byte 0, bit 0
}

// Blocks on the TWAI driver queue: wakes only when a frame arrives.
void can_task(void *arg) {
  twai_message_t msg;
  while (1) {
    esp_err_t err = twai_receive(&msg, portMAX_DELAY);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Message reception failed: %s", esp_err_to_name(err));
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    if (msg.extd || msg.rtr) continue;

    switch (msg.identifier) {
      case 0x373:
        if (msg.data_length_code < 8) break;
        Process_Battery(msg.data);
        data_ready = true;
        break;
      case 0x126:
        if (msg.data_length_code < 8) break;
        Process_Motor_Inverter(msg.data);
        data_ready = true;
        break;
      case 0x802:
        if (msg.data_length_code < 4) break;
        Process_HeaterPower(msg.data);
        data_ready = true;
        break;
      case 0x801:
        if (msg.data_length_code < 1) break;
        Process_Sidelights(msg.data);
        data_ready = true;
        break;
      // TODO: replace 0xTBD with actual charge mode CAN ID
      // case 0xTBD:
      //   if (msg.data_length_code < 1) break;
      //   Process_ChargeMode(msg.data);
      //   data_ready = true;
      //   break;
      default:
        break;
    }
  }
}

// ---------------- Display updates (loop only) ----------------

void Update_Backlight(void) {
  if (!g_fade_done) return;
  static int last_level = -1;
  int level = g_charge_mode ? 0 : (g_sidelights_on ? BL_DIM : BL_NORMAL);
  if (level == last_level) return;
  last_level = level;
  Set_Backlight(level);
}

void Update_Values(void) {
  ui_set_motor_temp(g_motor_temp);
  ui_set_inverter_temp(g_inverter_temp);
  ui_set_battery_temp(g_battery_temp);
  ui_set_heater_w(g_heater_power);
  ui_set_cell_high_mv(g_max_cell_mv);
  ui_set_cell_low_mv(g_min_cell_mv);
  Update_Backlight();
}

void Tick_Fade(uint32_t now) {
  // Blank until FADE_START_MS, then ramp to BL_NORMAL by FADE_END_MS.
  // Screen stays off if charge mode is active.
  if (g_fade_done) return;
  if (now >= FADE_END_MS) {
    g_fade_done = true;
    Update_Backlight();
  } else if (now >= FADE_START_MS && !g_charge_mode) {
    Set_Backlight((now - FADE_START_MS) * BL_NORMAL / (FADE_END_MS - FADE_START_MS));
  }
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("begin");
  Drivers_Init();
  Set_Backlight(0);   // start dark, fade in via loop
  ui_init();

  Set_EXIO(EXIO_PIN4, Low);
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason: %d\n", (int)reason);
  delay(500);
  CANBus_Init();

  xTaskCreatePinnedToCore(can_task, "CAN_Task", 4096, NULL, 2, NULL, 1);
}

void loop(void) {
  Tick_Fade(millis());

  if (data_ready) {
    data_ready = false;
    Update_Values();
  }

  // Sleep until LVGL's next timer is due (1-5 ms)
  uint32_t wait_ms = lv_timer_handler();
  if (wait_ms > 5) wait_ms = 5;
  TickType_t ticks = pdMS_TO_TICKS(wait_ms);
  vTaskDelay(ticks ? ticks : 1);
}
