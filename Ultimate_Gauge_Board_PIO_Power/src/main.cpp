/* Power gauge - ESP32-S3 2.1" Ultimate Gauge Board                */
/* Based on the driver setup by Andy Valentine - Garage Tinkering  */
/*                                                                 */
/* Shows battery current (ISA shunt 0x521) and power (0x526).      */

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

// Startup timing
static const uint32_t FADE_START_MS = 5800;
static const uint32_t FADE_END_MS   = 6300;
static const uint32_t SWEEP_UP_MS   = 1000;
static const uint32_t SWEEP_DOWN_MS = 800;

// ---- Values written by the CAN task, read by loop() ----
volatile int32_t g_amps = UI_AMPS_MIN;   // Battery current: -200A to 400A
volatile int32_t g_kw   = UI_KW_MIN;     // Battery power:   -70kW to 300kW
volatile bool g_sidelights_on = false;   // CAN 0x801
volatile bool g_charge_mode   = false;   // Screen stays off while charging (CAN ID TBD)
volatile bool data_ready      = false;   // New incoming data

// ---- loop()-only state ----
bool g_fade_done = false;

enum AnimState { ANIM_IDLE, ANIM_SWEEP_UP, ANIM_SWEEP_DOWN, ANIM_DONE };
AnimState g_anim_state    = ANIM_IDLE;
uint32_t  g_anim_start_ms = 0;


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

static int32_t read_le_i32(const uint8_t *b) {
  return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

void Process_Amps(const uint8_t *bytes) {
  // ISA shunt 0x521: amps (mA), bytes 2-5 little-endian
  int32_t raw = read_le_i32(&bytes[2]);
  g_amps = -(raw / 1000);
#if DEBUG_CAN
  Serial.printf("Amps raw mA: %ld  Amps: %ld\n", (long)raw, (long)g_amps);
#endif
}

void Process_KW(const uint8_t *bytes) {
  // ISA shunt 0x526: power (W), bytes 2-5 little-endian
  int32_t raw = read_le_i32(&bytes[2]);
  g_kw = -(raw / 1000);
#if DEBUG_CAN
  Serial.printf("kW raw W: %ld  kW: %ld\n", (long)raw, (long)g_kw);
#endif
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
      case 0x521:
        if (msg.data_length_code < 6) break;
        Process_Amps(msg.data);
        data_ready = true;
        break;
      case 0x526:
        if (msg.data_length_code < 6) break;
        Process_KW(msg.data);
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
  ui_set_amps(g_amps);
  ui_set_kw(g_kw);
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

void Tick_Animation(uint32_t now) {
  if (g_anim_state == ANIM_IDLE) {
    if (!g_fade_done) return;
    g_anim_state    = ANIM_SWEEP_UP;
    g_anim_start_ms = now;
  }

  uint32_t elapsed = now - g_anim_start_ms;

  if (g_anim_state == ANIM_SWEEP_UP) {
    float t = (float)elapsed / SWEEP_UP_MS;
    if (t > 1.0f) t = 1.0f;
    ui_set_amps(UI_AMPS_MIN + (int32_t)(t * (UI_AMPS_MAX - UI_AMPS_MIN)));
    ui_set_kw  (UI_KW_MIN   + (int32_t)(t * (UI_KW_MAX   - UI_KW_MIN)));
    if (elapsed >= SWEEP_UP_MS) {
      g_anim_state    = ANIM_SWEEP_DOWN;
      g_anim_start_ms = now;
    }
    return;
  }

  if (g_anim_state == ANIM_SWEEP_DOWN) {
    // Sweep from peak down to the current CAN value
    float t = (float)elapsed / SWEEP_DOWN_MS;
    if (t > 1.0f) t = 1.0f;
    ui_set_amps(UI_AMPS_MAX + (int32_t)(t * (g_amps - UI_AMPS_MAX)));
    ui_set_kw  (UI_KW_MAX   + (int32_t)(t * (g_kw   - UI_KW_MAX)));
    if (elapsed >= SWEEP_DOWN_MS) {
      g_anim_state = ANIM_DONE;
      data_ready = false;
      Update_Values();
    }
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
  uint32_t now = millis();

  Tick_Fade(now);

  if (g_anim_state != ANIM_DONE) {
    Tick_Animation(now);
  } else if (data_ready) {
    data_ready = false;
    Update_Values();
  }

  // Sleep until LVGL's next timer is due (1-5 ms)
  uint32_t wait_ms = lv_timer_handler();
  if (wait_ms > 5) wait_ms = 5;
  TickType_t ticks = pdMS_TO_TICKS(wait_ms);
  vTaskDelay(ticks ? ticks : 1);
}
