/***************************************************************/
/*                                                             */
/*      PROJECT NAME :  mcr_camera_2026base                    */
/*      FILE         :  mcr_camera_2026base.cpp                */
/*      DESCRIPTION  :  Main Program                           */
/*                                                             */
/*      カメラ入力 + デバッグ表示                                */
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

#include "drivers/Camera.h"
#include "drivers/Onboard.h"
#include "drivers/Serial.h"

// OSTM0 タイマー割り込み (1ms周期)
// GR-PEACH (RZ/A1H) の周辺クロック P0Φ は 33.33MHz
// 1ms = 33333 カウント (P0 = 33.33MHz に戻す)
#define OSTM0_CMP_1MS 33333

// デバッグ表示用: 閾値設定
#define DEBUG_THRESHOLD 200         // 画像二値化の閾値
#define DEBUG_DISPLAY_ROW 60        // 表示行（画像の中央付近）
#define DEBUG_PRINT_INTERVAL_MS 200 // シリアル出力間隔(ms)

// グローバルタイマーカウンタ
volatile unsigned long g_timer_1ms = 0;

// シリアル出力用カウンタ
volatile unsigned long g_cnt_printf = 0;

// Onboardインスタンス（グローバル）
Onboard g_onboard;

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
  // bit1: OSTMnMD1 = 0 (インターバルタイマーモード)
  // bit0: OSTMnMD0 = 1 (コンペアマッチ時に割り込み要求を発生)
  OSTM0.OSTMnCTL = 0x01;

  // GIC設定: OSTM0割り込みを有効化
  // まず以前の割り込み状態を確実にクリア（安全のため）
  INTC.ICDICER4 = (1 << 6); // Disable
  INTC.ICDICPR4 = (1 << 6); // Clear Pending

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
  // 優先度: 最高 (0x00) もしくは高め (0x80) に設定
  uint32_t ipr = INTC.ICDIPR33;
  ipr &= ~(0xFF << 16);
  ipr |= (0x80 << 16); // 優先度を中間に引き上げ
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
  // (安全のため0xF8に設定することもあるが0xFFで全通し)
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
  g_cnt_printf++;

  // カメラのフレーム周期処理（ステップ実行）
  g_camera.update();

  // 1秒ごとにUSER LEDをトグル
  if (g_timer_1ms % 1000 == 0) {
    static int toggle = 0;
    toggle = !toggle;
    g_onboard.setUserLed(toggle);
  }

  // スイッチ状態でフルカラーLEDを制御
  if (g_onboard.sw()) {
    g_onboard.setColorLed(1, 1, 1);
  } else {
    g_onboard.setColorLed(0, 0, 0);
  }

  g_onboard.update();
}

int main(void) {
  // オンボードLED/SWの初期化
  g_onboard.init();

  // シリアル通信初期化 (115200bps)
  g_serial.init();
  g_serial.printf("\033[2J\033[H"); // 画面クリア & カーソルホーム
  g_serial.printf("\x1b[36m--- MCR Camera 2026 Base ---\x1b[39m\n");
  g_serial.printf("カメラ初期化中...\n");

  // カメラ初期化（VDC5 + DVDEC）
  g_camera.init();
  g_serial.printf("カメラ初期化完了\n");

  // OSTM0タイマー割り込みを設定・開始（1ms周期）
  initOSTM0();

  g_serial.printf("タイマー開始\n");
  g_serial.printf("デバッグ表示: 閾値=%d, 行=%d\n", DEBUG_THRESHOLD,
                  DEBUG_DISPLAY_ROW);
  g_serial.printf("0/1表示: 0=暗い, 1=明るい（閾値以上）\n\n");

  // メインループ: カメラ映像のデバッグ表示
  // 参考プロジェクト debug_mode case 3 と同じパターン
  int x, y, c;

  while (1) {
    // 一定間隔でシリアル出力
    if (g_cnt_printf >= DEBUG_PRINT_INTERVAL_MS) {
      g_cnt_printf = 0;

      // ヘッダ行（参考プロジェクト準拠）
      g_serial.printf(
          "shi 0         0         0         0         0         0  "
          "       0         0         0         0         1         "
          "1         1         1         1         1        1\r\n");
      g_serial.printf(
          "kii 0         1         2         3         4         5  "
          "       6         7         8         9         0         "
          "1         2         3         4         5        5\r\n");
      g_serial.printf(
          "200 01234567890123456789012345678901234567890123456789012345"
          "67890123456789012345678901234567890123456789012345678901234567"
          "8901234567890123456789012345678901234567890123456789\r\n");

      // 30行目〜100行目を2行飛ばしで表示（参考プロジェクトと同じ）
      for (y = 30; y < 100; y += 2) {
        g_serial.printf("%03d:", y);
        for (x = 0; x < 160; x++) {
          c = g_camera.getPixel(x, y) >= DEBUG_THRESHOLD ? 1 : 0;
          g_serial.printf("%d", c);
        }
        g_serial.printf("  \r\n");
      }

      // カーソルをホームに戻す（次の表示で上書き）
      g_serial.printf("\033[H");
    }
  }

  return 0;
}
