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

// GR-PEACH USER_LED: P6_12 (Low Active: Low出力で点灯)
#define PIN_LED_USER (12)

int main(void) {

  // --- USER_LED (P6_12) をGPIO出力に設定 ---

  // PMC6: ポートモード制御 -> 0 = ポートモード（GPIO）
  GPIO.PMC6 &= ~(1 << PIN_LED_USER);

  // PM6: ポート方向 -> 0 = 出力
  GPIO.PM6 &= ~(1 << PIN_LED_USER);

  // PIPC6: ソフトウェアIO制御を選択 -> 0
  GPIO.PIPC6 &= ~(1 << PIN_LED_USER);

  // --- USER_LEDを点灯（Low Active: Low出力 = 点灯） ---
  GPIO.P6 &= ~(1 << PIN_LED_USER);

  // 無限ループ（プログラムが終了しないようにする）
  while (1) {
    // 何もしない（LEDは点灯したまま）
  }

  return 0;
}
