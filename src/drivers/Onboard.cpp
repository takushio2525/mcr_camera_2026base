/*
 * Onboard.cpp
 *
 *  Onboard LED/SW Driver (Latch System)
 *  EMA 準拠版: SystemData / Config ベース
 *  For GR-PEACH (RZ/A1H)
 *
 *  LED is Low Active (GPIO Output 0=ON, 1=OFF)
 */

#include "Onboard.h"
#include "../core/SystemData.h"
#include "iodefine.h"

#define PIN_SW (0)
#define PIN_LED_USER (12)
#define PIN_LED_RED (13)
#define PIN_LED_GRN (14)
#define PIN_LED_BLU (15)

// LED Pin number table (LED_PINS[i] と ledState_[i] の対応:
//   0=RED, 1=GREEN, 2=BLUE, 3=USER)
static const int LED_PINS[ONBOARD_LED_COUNT] = {PIN_LED_RED, PIN_LED_GRN,
                                                PIN_LED_BLU, PIN_LED_USER};

Onboard::Onboard(const OnboardConfig& cfg) : _config(cfg) {
  // ラッチバッファの初期化。
  // ledState_[i] は論理値で 1=点灯 / 0=消灯（GPIO は Low Active なので
  // updateOutput() 側で反転して出力する）。つまりここで入れている 1 は
  // 「全消灯」ではなく **全点灯** を意味する点に注意。
  //
  // 実害が出ていないのは、init() が GPIO.P6 |= ledMask で物理的に全消灯し、
  // runLogicInit() が sys.ob.*LedCmd をゼロクリアするため、最初の
  // updateOutput() で 0（消灯）に上書きされるから。
  for (int i = 0; i < ONBOARD_LED_COUNT; i++) {
    ledState_[i] = 1;
  }
}

bool Onboard::init() {
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
  return true;
}

void Onboard::updateInput(SystemData& sys) {
  // SW 状態を sys.ob.sw に反映
  sys.ob.sw = sw();
}

void Onboard::setUserLed(int val) { ledState_[3] = val ? 1 : 0; }

void Onboard::setColorLed(int r, int g, int b) {
  ledState_[0] = r ? 1 : 0;
  ledState_[1] = g ? 1 : 0;
  ledState_[2] = b ? 1 : 0;
}

void Onboard::updateOutput(SystemData& sys) {
  // SystemData → 内部ラッチに転記
  setColorLed(sys.ob.ledRedCmd, sys.ob.ledGreenCmd, sys.ob.ledBlueCmd);
  setUserLed(sys.ob.userLedCmd);

  // ラッチ → GPIO 反映
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
  // ユーザースイッチ(P6_0)は active-low（押すとLow）。
  // (PPR6 & 1) が 0 のとき押下と判定する。
  if ((GPIO.PPR6 & (1 << PIN_SW)) == 0) {
    return 1; // Pressed（active-low: 押すと0になる）
  } else {
    return 0; // Released
  }
}
