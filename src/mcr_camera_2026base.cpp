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
extern void __main()
{
  static int initialized;
  if (!initialized)
  {
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
#include "system/system_init.h"
#include "drivers/Motor.h"
#include "drivers/LineDetector.h"

// OSTM0 タイマー割り込み (1ms周期)
// GR-PEACH (RZ/A1H) の周辺クロック P0Φ は 33.33MHz
// 1ms = 33333 カウント (P0 = 33.33MHz に戻す)
#define OSTM0_CMP_1MS 33333

// デバッグ表示用: 閾値設定
#define DEBUG_THRESHOLD 170         // 画像二値化の閾値
#define DEBUG_DISPLAY_ROW 60        // 表示行（画像の中央付近）
#define DEBUG_PRINT_INTERVAL_MS 200 // シリアル出力間隔(ms)

// グローバルタイマーカウンタ
volatile unsigned long g_timer_1ms = 0;

// シリアル出力用カウンタ
volatile unsigned long g_cnt_printf = 0;

// フレーム更新カウンタ（デバッグ用：フレーム処理完了回数を追跡）
static unsigned long s_frameCount = 0;

// Onboardインスタンス（グローバル）
Onboard g_onboard;

extern "C" void ostm0_interrupt_callback(void);

// GIC (Generic Interrupt Controller) のグローバル有効化
// 参考プロジェクト(mbed-os)では、ブートコードがこれを先に行っている。
// カメラ初期化でVDC5割り込みが GIC に登録されるため、
// GIC と CPU IRQ はカメラ初期化の前に有効化する必要がある。
static void initGIC(void)
{
  // GICディストリビュータ有効化
  INTC.ICDDCR = 0x01;

  // GIC CPUインターフェース有効化
  INTC.ICCICR = 0x01;

  // 割り込み優先度マスク: 全割り込みを許可
  INTC.ICCPMR = 0xFF;

  // CPUレベルのIRQを有効化（CPSR Iビットクリア）
  __asm__ volatile("CPSIE i");
}

// OSTM0タイマー初期化 (1ms周期インターバルタイマー)
// ※GICのグローバル有効化(initGIC)は事前に呼び出し済みの前提
static void initOSTM0(void)
{
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
  // bit0: OSTMnMD0 = 1 (コンペアマッチ時に割り込み要求を発生=周期割り込み)
  // ※MD0=0だとカウント開始時の1回のみ割り込みが発生し、周期動作しない
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
  uint32_t ipr = INTC.ICDIPR33;
  ipr &= ~(0xFF << 16);
  ipr |= (0x80 << 16);
  INTC.ICDIPR33 = ipr;

  // 割り込みプロセッサターゲット設定 (ICDIPTR)
  // CPU0にターゲット (0x01)
  // IRQ134 → レジスタ番号 = 134/4 = 33, バイト位置 = (134%4)*8 = 16
  uint32_t iptr = INTC.ICDIPTR33;
  iptr &= ~(0xFF << 16);
  iptr |= (0x01 << 16);
  INTC.ICDIPTR33 = iptr;

  // OSTM0カウント開始
  OSTM0.OSTMnTS = 0x01;
}

// OSTM0割り込みコールバック
// inthandler.c の INT_Excep_OSTMI0() から呼ばれる
// 参考プロジェクト(2.38m-s)の intTimer() と同等の役割。
// XIP環境ではimageCopy等が1msを超過するが、フレーム処理完了後の
// Vfield待ち期間にメインループにCPU時間が返る。
void ostm0_interrupt_callback(void)
{
  g_timer_1ms++;
  g_cnt_printf++;

  // カメラのフレーム周期処理（参考プロジェクトと同じく割り込み内で実行）
  g_camera.update();
  g_motor.update();

  // 1秒ごとにUSER LEDをトグル
  if (g_timer_1ms % 1000 == 0)
  {
    static int toggle = 0;
    toggle = !toggle;
    g_onboard.setUserLed(toggle);
  }

  // モーターランプ制御ステートマシン
  // ボタン押下中: 正転加速→正転減速→停止→逆転加速→逆転減速→停止 を繰り返す
  {
    // ステート定義
    enum MotorPhase {
      RAMP_UP_FWD,    // 正転加速 (2秒)
      RAMP_DOWN_FWD,  // 正転減速 (2秒)
      STOP_PAUSE1,    // 停止 (0.5秒)
      RAMP_UP_REV,    // 逆転加速 (2秒)
      RAMP_DOWN_REV,  // 逆転減速 (2秒)
      STOP_PAUSE2     // 停止 (0.5秒)
    };

    static const int RAMP_TIME = 2000;   // 加減速時間 [ms]
    static const int PAUSE_TIME = 500;   // 停止時間 [ms]
    static const int MAX_SPEED = 100;    // 最大速度 [%]

    static MotorPhase phase = RAMP_UP_FWD;
    static int counter = 0;

    if (g_onboard.sw())
    {
      int speed = 0;

      switch (phase)
      {
      case RAMP_UP_FWD:
        // 正転: 0→MAX_SPEED (2秒)
        speed = MAX_SPEED * counter / RAMP_TIME;
        g_motor.set(speed, speed);
        g_onboard.setColorLed(0, 1, 0); // 緑: 正転加速
        counter++;
        if (counter >= RAMP_TIME) { counter = 0; phase = RAMP_DOWN_FWD; }
        break;

      case RAMP_DOWN_FWD:
        // 正転: MAX_SPEED→0 (2秒)
        speed = MAX_SPEED - MAX_SPEED * counter / RAMP_TIME;
        g_motor.set(speed, speed);
        g_onboard.setColorLed(0, 0, 1); // 青: 正転減速
        counter++;
        if (counter >= RAMP_TIME) { counter = 0; phase = STOP_PAUSE1; }
        break;

      case STOP_PAUSE1:
        // 停止 (0.5秒)
        g_motor.stop();
        g_onboard.setColorLed(1, 0, 0); // 赤: 停止中
        counter++;
        if (counter >= PAUSE_TIME) { counter = 0; phase = RAMP_UP_REV; }
        break;

      case RAMP_UP_REV:
        // 逆転: 0→-MAX_SPEED (2秒)
        speed = MAX_SPEED * counter / RAMP_TIME;
        g_motor.set(-speed, -speed);
        g_onboard.setColorLed(0, 1, 1); // シアン: 逆転加速
        counter++;
        if (counter >= RAMP_TIME) { counter = 0; phase = RAMP_DOWN_REV; }
        break;

      case RAMP_DOWN_REV:
        // 逆転: -MAX_SPEED→0 (2秒)
        speed = MAX_SPEED - MAX_SPEED * counter / RAMP_TIME;
        g_motor.set(-speed, -speed);
        g_onboard.setColorLed(1, 0, 1); // マゼンタ: 逆転減速
        counter++;
        if (counter >= RAMP_TIME) { counter = 0; phase = STOP_PAUSE2; }
        break;

      case STOP_PAUSE2:
        // 停止 (0.5秒)
        g_motor.stop();
        g_onboard.setColorLed(1, 0, 0); // 赤: 停止中
        counter++;
        if (counter >= PAUSE_TIME) { counter = 0; phase = RAMP_UP_FWD; }
        break;
      }
    }
    else
    {
      // ボタン離した: 即停止、ステートリセット
      g_motor.stop();
      g_onboard.setColorLed(0, 0, 0);
      phase = RAMP_UP_FWD;
      counter = 0;
    }
  }

  g_onboard.update();
}

int main(void)
{
  // MMU + L1 キャッシュ有効化
  // FPU有効化 → TLB/キャッシュ無効化 → MMU設定 → MMU有効化 → キャッシュ有効化
  // シリアルは未初期化なのでこの中でシリアル出力は行わない
  SystemInit();

  // オンボードLED/SWの初期化
  g_onboard.init();

  // シリアル通信初期化 (230400bps)
  g_serial.init();
  g_serial.printf("\033[2J\033[H"); // 画面クリア & カーソルホーム
  g_serial.printf("\x1b[36m--- MCR Camera 2026 Base ---\x1b[39m\n");
  g_serial.printf("GIC初期化中...\n");

  // GICのグローバル有効化 (カメラより先に行う)
  // mbed-osではブートコードがこれを行っているため、参考プロジェクトでは
  // カメラ初期化時にVDC5割り込みが正常に動作する。
  // ベアメタルでは明示的に先に呼ぶ必要がある。
  initGIC();
  g_serial.printf("GIC初期化完了\n");

  g_serial.printf("カメラ初期化中...\n");

  // カメラ初期化（VDC5 + DVDEC）
  g_camera.init();
  g_serial.printf("カメラ初期化完了\n");

  // OSTM0タイマー割り込みを設定・開始（1ms周期）
  initOSTM0();

  // モーター初期化
  g_motor.init();
  g_serial.printf("モーター初期化完了\n");

  // ライン検出初期化
  g_lineDetector.init();
  g_serial.printf("ライン検出初期化完了\n");

  g_serial.printf("タイマー開始\n");
  g_serial.printf("デバッグ表示: 閾値=%d, 行=%d\n", DEBUG_THRESHOLD,
                  DEBUG_DISPLAY_ROW);
  g_serial.printf("0/1表示: 0=暗い, 1=明るい（閾値以上）\n\n");

  // メインループ: カメラ映像のデバッグ表示
  // 参考プロジェクト debug_mode case 3 と完全に同じパターン
  int x, y, c;

  // 行バッファ: ハイライト時 最大11文字 × 160px + ヘッダ4 + 末尾4 = 最大 1768文字
  // + 偏差位置の黄色背景分の余裕
  static char lineBuf[2560];

  while (1)
  {
    // フレーム更新完了をカウント（カメラ更新は割り込み内で実行される）
    if (g_camera.isFrameReady())
    {
      s_frameCount++;
      g_camera.clearFrameReady();
      // フレーム完了時にライン検出を実行
      g_lineDetector.update();
    }

    // 一定間隔でシリアル出力（参考プロジェクトと同じ 200ms）
    if (g_cnt_printf >= DEBUG_PRINT_INTERVAL_MS)
    {
      g_cnt_printf = 0;

      // カーソルを左上に移動して上書き描画（画面全体クリアよりチラつきが少ない）
      g_serial.printf("\033[H");

      // タイマー値・フレーム更新カウンタを表示（デバッグ用）
      g_serial.printf("T=%lu Frame=%lu              \r\n",
                      g_timer_1ms, s_frameCount);

      // ライン検出フラグを表示
      g_serial.printf("Cross=%d Left=%d Right=%d Center=%d Dev[%d]=%d        \r\n",
                      g_lineDetector.isCrossLine() ? 1 : 0,
                      g_lineDetector.isLeftLine() ? 1 : 0,
                      g_lineDetector.isRightLine() ? 1 : 0,
                      g_lineDetector.isCenterLine() ? 1 : 0,
                      g_lineDetector.getDetectRow(),
                      g_lineDetector.getDeviation(g_lineDetector.getDetectRow()));

      // ヘッダ行（参考プロジェクト準拠）
      g_serial.print(
          "shi 0         0         0         0         0         0         0   "
          "      0         0         0         1         1         1         1 "
          "        1         1        1\r\n");
      g_serial.print(
          "kii 0         1         2         3         4         5         6   "
          "      7         8         9         0         1         2         3 "
          "        4         5        5\r\n");
      g_serial.print("200 "
                     "01234567890123456789012345678901234567890123456789012345"
                     "67890123456789012345678901234567890123456789012345678901"
                     "234567890123456789012345678901234567890123456789\r\n");

      // 30行目〜100行目を2行飛ばしで表示（参考プロジェクトと同じ）
      for (y = 30; y < 100; y += 2)
      {
        // 行ヘッダを書き込み
        int pos = 0;
        lineBuf[pos++] = '0' + (y / 100) % 10;
        lineBuf[pos++] = '0' + (y / 10) % 10;
        lineBuf[pos++] = '0' + y % 10;
        lineBuf[pos++] = ':';

        for (x = 0; x < 160; x++)
        {
          c = g_camera.getPixel(x, y) >= DEBUG_THRESHOLD ? 1 : 0;

          // 偏差位置（検出されたライン中心）を黄色背景でハイライト
          // 参考プロジェクト: if (x == -allDeviation[y] + 80) printf("\x1b[43m%d\x1b[49m", c);
          if (x == -g_lineDetector.getDeviation(y) + LineDetector::CENTER)
          {
            // 黄色背景 \x1b[43m + 数字 + \x1b[49m
            lineBuf[pos++] = '\x1b';
            lineBuf[pos++] = '[';
            lineBuf[pos++] = '4';
            lineBuf[pos++] = '3';
            lineBuf[pos++] = 'm';
            lineBuf[pos++] = '0' + c;
            lineBuf[pos++] = '\x1b';
            lineBuf[pos++] = '[';
            lineBuf[pos++] = '4';
            lineBuf[pos++] = '9';
            lineBuf[pos++] = 'm';
          }
          else if (c == 1)
          {
            // 1検出: 白背景 \x1b[47m + '1' + \x1b[49m
            lineBuf[pos++] = '\x1b';
            lineBuf[pos++] = '[';
            lineBuf[pos++] = '4';
            lineBuf[pos++] = '7';
            lineBuf[pos++] = 'm';
            lineBuf[pos++] = '1';
            lineBuf[pos++] = '\x1b';
            lineBuf[pos++] = '[';
            lineBuf[pos++] = '4';
            lineBuf[pos++] = '9';
            lineBuf[pos++] = 'm';
          }
          else
          {
            // 0検出: 色なし
            lineBuf[pos++] = '0';
          }
        }
        // 行末
        lineBuf[pos++] = ' ';
        lineBuf[pos++] = ' ';
        lineBuf[pos++] = '\r';
        lineBuf[pos++] = '\n';
        lineBuf[pos] = '\0';
        g_serial.print(lineBuf);
      }
    }
  }

  return 0;
}
