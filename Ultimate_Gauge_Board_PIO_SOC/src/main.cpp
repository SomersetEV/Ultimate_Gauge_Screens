/* SOC gauge - ESP32-S3 2.1" Ultimate Gauge Board                  */
/* Based on the driver setup by Andy Valentine - Garage Tinkering  */
/*                                                                 */
/* Shows state of charge (0x355) and system voltage (0x802).       */

#include <Arduino.h>
#include "CANBus_Driver.h"
#include "LVGL_Driver.h"
#include "I2C_Driver.h"

#include "esp_system.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "ui/ui.h"

#define TAG "TWAI"

// Set to 1 to print decoded CAN values to Serial (slows the CAN task)
#define DEBUG_CAN 0

// Backlight levels (see Set_Backlight for scaling)
#define BL_NORMAL  50
#define BL_DIM     25

// SOC below this shows the arc in red
#define SOC_LOW_PCT 20

// Startup timing
static const uint32_t FADE_START_MS     = 5800;
static const uint32_t FADE_END_MS       = 6300;
static const uint32_t SWEEP_DURATION_MS = 1000;

// ---- Values written by the CAN task, read by loop() ----
volatile int32_t g_soc          = 50;     // %
volatile float   g_sys_voltage  = 0.0f;   // V
volatile bool g_sidelights_on   = false;  // CAN 0x801
volatile bool data_ready        = false;  // New incoming data

// ---- loop()-only state ----
bool g_fade_done = false;

enum SweepState { SWEEP_IDLE, SWEEP_UP, SWEEP_DOWN, SWEEP_DONE };
SweepState g_sweep_state    = SWEEP_IDLE;
uint32_t   g_sweep_start_ms = 0;


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

// 0x355: bytes 0+1, uint16 little-endian, scale 1 -> SOC %
void Process_SOC_355(const uint8_t *data) {
  g_soc = (uint16_t)(data[0] | (data[1] << 8));
#if DEBUG_CAN
  Serial.printf("SOC: %ld\n", (long)g_soc);
#endif
}

// 0x802: bytes 0+1, uint16 little-endian, mV -> system voltage
void Process_SysVoltage_802(const uint8_t *data) {
  uint16_t raw = (uint16_t)(data[0] | (data[1] << 8));
  g_sys_voltage = raw / 1000.0f;
#if DEBUG_CAN
  Serial.printf("Voltage: %.3f\n", raw / 1000.0f);
#endif
}

void Process_Sidelights(const uint8_t *data) {
  g_sidelights_on = (data[0] & 0x02) != 0;  // byte 0, bit 1
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
      case 0x355:
        if (msg.data_length_code < 2) break;
        Process_SOC_355(msg.data);
        data_ready = true;
        break;
      case 0x802:
        if (msg.data_length_code < 2) break;
        Process_SysVoltage_802(msg.data);
        data_ready = true;
        break;
      case 0x801:
        if (msg.data_length_code < 1) break;
        Process_Sidelights(msg.data);
        data_ready = true;
        break;
      default:
        break;
    }
  }
}

// ---------------- Display updates (loop only) ----------------

void Update_SOC(void) {
  int32_t soc = g_soc;
  ui_set_soc(soc);
  ui_set_soc_low(soc < SOC_LOW_PCT);
}

void Update_Backlight(void) {
  if (!g_fade_done) return;
  static int last_level = -1;
  int level = g_sidelights_on ? BL_DIM : BL_NORMAL;
  if (level == last_level) return;
  last_level = level;
  Set_Backlight(level);
}

void Update_Values(void) {
  if (g_sweep_state == SWEEP_DONE) Update_SOC();  // don't override the startup sweep
  ui_set_voltage(g_sys_voltage);
  Update_Backlight();
}

void Tick_Fade(uint32_t now) {
  // Blank until FADE_START_MS, then ramp to BL_NORMAL by FADE_END_MS
  if (g_fade_done) return;
  if (now >= FADE_END_MS) {
    g_fade_done = true;
    Update_Backlight();
  } else if (now >= FADE_START_MS) {
    Set_Backlight((now - FADE_START_MS) * BL_NORMAL / (FADE_END_MS - FADE_START_MS));
  }
}

void Run_Startup_Sweep(uint32_t now) {
  if (g_sweep_state == SWEEP_DONE) return;

  if (g_sweep_state == SWEEP_IDLE && g_fade_done) {
    g_sweep_state    = SWEEP_UP;
    g_sweep_start_ms = now;
  }

  int32_t sweep_val = 0;
  uint32_t elapsed = now - g_sweep_start_ms;
  if (g_sweep_state == SWEEP_UP) {
    if (elapsed >= SWEEP_DURATION_MS) {
      sweep_val = 100;
      g_sweep_state    = SWEEP_DOWN;
      g_sweep_start_ms = now;
    } else {
      sweep_val = (int32_t)(elapsed * 100 / SWEEP_DURATION_MS);
    }
  } else if (g_sweep_state == SWEEP_DOWN) {
    if (elapsed >= SWEEP_DURATION_MS) {
      // Sweep finished: show the live SOC straight away
      g_sweep_state = SWEEP_DONE;
      data_ready = false;
      Update_Values();
      return;
    }
    sweep_val = (int32_t)(100 - elapsed * 100 / SWEEP_DURATION_MS);
  }

  ui_set_soc(sweep_val);
  ui_set_soc_low(sweep_val < SOC_LOW_PCT);
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("begin");
  Drivers_Init();
  Set_Backlight(0);   // start dark, fade in via loop
  ui_init();          // shows 0% and 0.0v until CAN data arrives

  Set_EXIO(EXIO_PIN4, Low);
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Reset reason: %d\n", (int)reason);
  delay(500);
  CANBus_Init();

  xTaskCreatePinnedToCore(can_task, "CAN_Task", 4096, NULL, 2, NULL, 1);
}

void loop(void) {
  uint32_t now = millis();

  Tick_Fade(now);
  Run_Startup_Sweep(now);

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
