/*
 * Onboard.cpp
 *
 *  Onboard LED/SW Driver (Latch System)
 *  For GR-PEACH (RZ/A1H)
 *  Object-Oriented Version
 *
 *  LED is Low Active (GPIO Output 0=ON, 1=OFF)
 */

#include "Onboard.h"
#include "iodefine.h"

#define PIN_SW (0)
#define PIN_LED_USER (12)
#define PIN_LED_RED (13)
#define PIN_LED_GRN (14)
#define PIN_LED_BLU (15)

// LED Pin number table
static const int LED_PINS[ONBOARD_LED_COUNT] = {PIN_LED_RED, PIN_LED_GRN,
                                                PIN_LED_BLU, PIN_LED_USER};

Onboard::Onboard() {
  // Initialize latch buffer (All OFF)
  for (int i = 0; i < ONBOARD_LED_COUNT; i++) {
    ledState_[i] = 0;
  }

  // Port 6 Setting

  // LED -> Output
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
}

void Onboard::setLed(int id, int val) {
  if (id < 0 || id >= ONBOARD_LED_COUNT)
    return;
  ledState_[id] = val ? 1 : 0;
}

void Onboard::update() {
  for (int i = 0; i < ONBOARD_LED_COUNT; i++) {
    int pin = LED_PINS[i];
    if (ledState_[i]) {
      GPIO.P6 &= ~(1 << pin); // ON: Low output
    } else {
      GPIO.P6 |= (1 << pin); // OFF: High output
    }
  }
}

int Onboard::sw() {
  // ユーザースイッチ(P6_0)はMbed側ソースと実機挙動から active-high と推測。
  // (PPR6 & 1) が 1 のとき押下と判定する。
  if ((GPIO.PPR6 & (1 << PIN_SW)) != 0) {
    return 1; // Pressed // Active High: 押すと1になる
  } else {
    return 0; // Released
  }
}
