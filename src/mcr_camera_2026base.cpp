/***************************************************************/
/*                                                             */
/*      PROJECT NAME :  mcr_camera_2026base                    */
/*      FILE         :  mcr_camera_2026base.cpp                */
/*      DESCRIPTION  :  Main Program                           */
/*                                                             */
/*      USER_LED点灯テスト                                      */
/*                                                             */
/***************************************************************/
#include "iodefine.h"
#include <typedefine.h>

#ifdef CPPAPP
// グローバルコンストラクタの初期化
extern void __main() {
  static int initialized;
  if (!initialized) {
    typedef void (*pfunc)();
    extern pfunc __ctors[];
    extern pfunc __ctors_end[];
    pfunc *p;

    initialized = 1;
    for (p = __ctors_end; p > __ctors;)
      (*--p)();
  }
}
#endif

#include "drivers/Onboard.h"

// OSTM0 タイマー割り込み (1ms周期)
// GR-PEACH (RZ/A1H) の周辺クロック P0Φ は 33.33MHz
// 1ms = 33333 カウント
#define OSTM0_CMP_1MS 33333

// グローバルタイマーカウンタ
volatile unsigned long g_timer_1ms = 0;

// Onboardインスタンスへのポインタ（割り込みハンドラからアクセスするため）
Onboard *g_onboard = nullptr;

extern "C" void ostm0_interrupt_callback(void);

// OSTM0タイマー初期化 (1ms周期インターバルタイマー)
static void initOSTM0(void) {
  // OSTMのスタンバイ解除 (STBCR5 bit1 = OSTM0)
  CPG.STBCR5 &= ~(0x02);

  // ダミーリード (リファレンスマニュアル推奨)
  volatile uint8_t dummy = CPG.STBCR5;
  (void)dummy;

  // OSTM0を停止
  OSTM0.OSTMnTT = 0x01;

  // 比較レジスタに1ms相当のカウント値を設定
  OSTM0.OSTMnCMP = OSTM0_CMP_1MS;

  // OSTMnCTL設定
  // bit1: OSTMnMD1 = 0 (インターバルタイマー)
  // bit0: OSTMnMD0 = 0 (コンペアマッチ時に割り込み要求を発生)
  OSTM0.OSTMnCTL = 0x00;

  // GIC設定: OSTM0割り込みを有効化
  // OSTM0 IRQ ID = 134
  // ICDISER (割り込みセットイネーブルレジスタ)
  // IRQ134 → レジスタ番号 = 134/32 = 4, ビット位置 = 134%32 = 6
  INTC.ICDISER4 |= (1 << 6);

  // エッジトリガ設定 (ICDICFR8)
  // IRQ134 -> 134/16 = 8, 134%16 = 6. 6 * 2 = 12ビット目
  // 10: Edge triggered (bit 13=1, bit 12=0)
  uint32_t icf = INTC.ICDICFR8;
  icf &= ~(3 << 12);
  icf |= (2 << 12);
  INTC.ICDICFR8 = icf;

  // 割り込み優先度設定 (ICDIPR)
  // IRQ134 → レジスタ番号 = 134/4 = 33, バイト位置 = (134%4)*8 = 16
  // 優先度: 低め (0xF0) に設定
  uint32_t ipr = INTC.ICDIPR33;
  ipr &= ~(0xFF << 16);
  ipr |= (0xF0 << 16);
  INTC.ICDIPR33 = ipr;

  // 割り込みプロセッサターゲット設定 (ICDIPTR)
  // CPU0にターゲット (0x01)
  // IRQ134 → レジスタ番号 = 134/4 = 33, バイト位置 = (134%4)*8 = 16
  uint32_t iptr = INTC.ICDIPTR33;
  iptr &= ~(0xFF << 16);
  iptr |= (0x01 << 16);
  INTC.ICDIPTR33 = iptr;

  // GICディストリビュータ有効化
  INTC.ICDDCR = 0x01;

  // GIC CPUインターフェース有効化
  INTC.ICCICR = 0x01;

  // 割り込み優先度マスク: 全割り込みを許可
  INTC.ICCPMR = 0xFF;

  // OSTM0カウント開始
  OSTM0.OSTMnTS = 0x01;

  // CPUレベルのIRQを有効化（CPSR Iビットクリア）
  __asm__ volatile("CPSIE i");
}

// OSTM0割り込みコールバック
// inthandler.c の INT_Excep_OSTMI0() から呼ばれる
void ostm0_interrupt_callback(void) {
  g_timer_1ms++;

  if (g_onboard != nullptr) {
    // LEDとスイッチのテスト: 100msごとにUSER LEDをトグル
    // スイッチが押されていたらREDを点灯
    if (g_timer_1ms % 1000 == 0) {
      static int toggle = 0;
      toggle = !toggle;
      g_onboard->setLed(3, toggle); // USER LED
    }

    if (g_onboard->sw()) {
      g_onboard->setLed(0, 1); // RED
      g_onboard->setLed(1, 1); // GREEN
      g_onboard->setLed(2, 1); // BLUE
    } else {
      g_onboard->setLed(0, 0); // RED
      g_onboard->setLed(1, 0); // GREEN
      g_onboard->setLed(2, 0); // BLUE
    }

    g_onboard->update();
  }
}

int main(void) {
  // オンボードLED/SWの初期化（インスタンス化）
  Onboard onboard;
  g_onboard = &onboard;

  // OSTM0タイマー割り込みを設定・開始（1ms周期）
  initOSTM0();

  // メインループ
  // 処理はすべて割り込み内（1ms周期）で実行されるため、ここでは何もしない
  while (1) {
    // 待機
  }

  return 0;
}
