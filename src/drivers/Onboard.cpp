/*
 * Onboard.cpp
 *
 *  Onboard LED/SW Driver (Latch System)
 *  For GR-PEACH (RZ/A1H)
 *
 *  LED is Low Active (GPIO Output 0=ON, 1=OFF)
 */

#include "Onboard.h"
#include "iodefine.h"

// GR-PEACH Onboard LEDs (Port 6)
// LED_RED:   P6_13
// LED_GREEN: P6_14
// LED_BLUE:  P6_15
// LED_USER:  P6_12

// USER_BUTTON: P6_0

#define PIN_SW (0)
#define PIN_LED_USER (12)
#define PIN_LED_RED (13)
#define PIN_LED_GRN (14)
#define PIN_LED_BLU (15)

// LED Pin number table
static const int LED_PINS[ONBOARD_LED_COUNT] = {PIN_LED_RED, PIN_LED_GRN,
                                                PIN_LED_BLU, PIN_LED_USER};

// Latch buffer (Initial: All OFF)
int Onboard::ledState_[ONBOARD_LED_COUNT] = {0, 0, 0, 0};

void Onboard::init() {
  // Port 6 Setting

  // LED -> Output
  // PM6 (Port mode): 0 = Output, 1 = Input
  // PIPC6: 0 = Software I/O control (Direct control mode)
  // PMC6 (Port mode control): 0 = Port mode (GPIO), 1 = Alternate mode
  uint16_t ledMask = (1 << PIN_LED_RED) | (1 << PIN_LED_GRN) |
                     (1 << PIN_LED_BLU) | (1 << PIN_LED_USER);

  // Set to Port Mode (use as GPIO)
  GPIO.PMC6 &= ~ledMask;

  // Set direction to Output
  GPIO.PM6 &= ~ledMask;

  // PIPC6: Select software I/O control
  GPIO.PIPC6 &= ~ledMask;

  // Initial state: Low Active, so High Output = OFF
  GPIO.P6 |= ledMask;

  // スイッチ -> 入力
  // PIPC6_0 -> ソフトウェアIO制御 (0)
  GPIO.PIPC6 &= ~(1 << PIN_SW);

  // PMC6_0 -> ポートモード (0)
  GPIO.PMC6 &= ~(1 << PIN_SW);

  // PM6_0 -> 入力 (1)
  GPIO.PM6 |= (1 << PIN_SW);

  // PIBC6 (ポート入力バッファ制御) -> 有効 (1)
  GPIO.PIBC6 |= (1 << PIN_SW);

  // PBDC6 -> 双方向モード無効 (0)
  GPIO.PBDC6 &= ~(1 << PIN_SW);

  // Initialize latch buffer (All OFF)
  for (int i = 0; i < ONBOARD_LED_COUNT; i++) {
    ledState_[i] = 0;
  }
}

void Onboard::setLed(int id, int val) {
  // Range check
  if (id < 0 || id >= ONBOARD_LED_COUNT)
    return;

  // Save to latch buffer (Not reflected to GPIO yet)
  ledState_[id] = val ? 1 : 0;
}

void Onboard::update() {
  // Reflect latch buffer contents to GPIO in one go
  // Low Active: val=1(ON) -> GPIO output=Low(0), val=0(OFF) -> GPIO
  // output=High(1)
  for (int i = 0; i < ONBOARD_LED_COUNT; i++) {
    int pin = LED_PINS[i];
    if (ledState_[i]) {
      // ON: Low output
      GPIO.P6 &= ~(1 << pin);
    } else {
      // OFF: High output
      GPIO.P6 |= (1 << pin);
    }
  }
}

int Onboard::sw() {
  // Read P6_0
  // If pulled-up and switch connected to GND, pressed = Low (0)
  if ((GPIO.PPR6 & (1 << PIN_SW)) == 0) {
    return 1; // Pressed
  } else {
    return 0; // Released
  }
}
