/***************************************************************/
/*                                                             */
/*      PROJECT NAME :  mcr_camera_2026base                    */
/*      FILE         :  mcr_camera_2026base.cpp                */
/*      DESCRIPTION  :  Main Program                           */
/*                                                             */
/*      カメラ入力 + メイン走行 (RunController)                */
/*                                                             */
/*      参考: mcr_shiozawa_cclass_2.38m-s/main.cpp             */
/*      （初心者向けにエンコーダ距離 → タイマー(ms)へ置換）     */
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
#include "drivers/Servo.h"
#include "drivers/LineDetector.h"
#include "drivers/SDLogger.h"
#include "drivers/RunController.h"

// OSTM0 タイマー割り込み (1ms周期)
// GR-PEACH (RZ/A1H) の周辺クロック P0Φ は 33.33MHz
// 1ms = 33333 カウント
#define OSTM0_CMP_1MS 33333

// デバッグ表示用: 閾値設定
#define DEBUG_THRESHOLD 170         // 画像二値化の閾値
#define DEBUG_PRINT_INTERVAL_MS 200 // シリアル出力間隔(ms)

// グローバルタイマーカウンタ
volatile unsigned long g_timer_1ms = 0;

// シリアル出力用カウンタ
volatile unsigned long g_cnt_printf = 0;

// フレーム更新カウンタ
static unsigned long s_frameCount = 0;

// 動作モード（main 起動時にスイッチ状態で決定）
//   true  = デバッグモード（カメラ画像のシリアル表示）
//   false = 走行モード（RunController が状態遷移を進める）
static bool s_debugMode = false;

// Onboardインスタンス（グローバル）
Onboard g_onboard;

extern "C" void ostm0_interrupt_callback(void);

// GIC (Generic Interrupt Controller) のグローバル有効化
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
static void initOSTM0(void)
{
  // OSTMのスタンバイ解除 (STBCR5 bit1 = OSTM0)
  CPG.STBCR5 &= ~(0x02);

  volatile uint8_t dummy = CPG.STBCR5;
  (void)dummy;

  // OSTM0を停止
  OSTM0.OSTMnTT = 0x01;

  // 比較レジスタに1ms相当のカウント値を設定
  OSTM0.OSTMnCMP = OSTM0_CMP_1MS;

  // OSTMnCTL: インターバルタイマーモード + 周期割り込み
  OSTM0.OSTMnCTL = 0x01;

  // GIC設定: OSTM0割り込みを有効化
  INTC.ICDICER4 = (1 << 6);
  INTC.ICDICPR4 = (1 << 6);
  INTC.ICDISER4 |= (1 << 6);

  // エッジトリガ設定 (ICDICFR8)
  uint32_t icf = INTC.ICDICFR8;
  icf &= ~(3 << 12);
  icf |= (2 << 12);
  INTC.ICDICFR8 = icf;

  // 割り込み優先度設定 (ICDIPR)
  uint32_t ipr = INTC.ICDIPR33;
  ipr &= ~(0xFF << 16);
  ipr |= (0x80 << 16);
  INTC.ICDIPR33 = ipr;

  // 割り込みプロセッサターゲット設定 (ICDIPTR)
  uint32_t iptr = INTC.ICDIPTR33;
  iptr &= ~(0xFF << 16);
  iptr |= (0x01 << 16);
  INTC.ICDIPTR33 = iptr;

  // OSTM0カウント開始
  OSTM0.OSTMnTS = 0x01;
}

// OSTM0割り込みコールバック
// inthandler.c の INT_Excep_OSTMI0() から呼ばれる
//
// 参考プロジェクト(2.38m-s)の intTimer() に相当する。
// XIP環境では imageCopy 等が1msを超えるが、フレーム処理完了後の
// Vfield 待ち期間にメインループへ CPU 時間が返る。
void ostm0_interrupt_callback(void)
{
  g_timer_1ms++;
  g_cnt_printf++;

  // カメラのフレーム周期処理（必須）
  g_camera.update();

  // モーター/サーボの周期処理（現状は空だが、将来の拡張用に呼ぶ）
  g_motor.update();
  g_servo.update();

  // 走行モードのときのみ RunController を進める
  if (!s_debugMode)
  {
    g_runController.update();
  }

  // GPIO ラッチ反映
  g_onboard.update();
}

// ====================================================================
// 走行モード: RunController によるメイン走行
// ====================================================================
static void runMainLoop(void)
{
  g_serial.printf("\n--- 走行モード ---\n");
  g_serial.printf("動作: スタート前=赤LED、バー検知=緑LED、ボタンで開始\n\n");

  bool savedToSD = false;

  while (1)
  {
    // フレーム完了時の処理
    if (g_camera.isFrameReady())
    {
      s_frameCount++;
      g_camera.clearFrameReady();

      // ライン検出を実行（RunController はこの結果を 1ms 割り込み内で参照する）
      g_lineDetector.update();

      // 走行中はログ記録
      // pattern==11 以降の走行ロジック中のみ（FINISH 状態は除く）
      int pat = g_runController.getPattern();
      if (pat >= RunController::TRACE_NORMAL
          && pat != RunController::FINISH
          && !g_sdlogger.isFull())
      {
        SDLOG_T &e = g_sdlogger.current();
        e.cnt_msdwritetime = g_timer_1ms;
        e.pattern          = (unsigned int)pat;
        e.convertBCD       = (unsigned int)g_lineDetector.getSensorBin();
        e.handle           = g_runController.getHandleVal();
        e.hennsa           = g_lineDetector.getDeviation(RunController::TRACE_ROW);
        e.encoder          = 0;
        e.motorL           = g_motor.getLeft();
        e.motorR           = g_motor.getRight();
        e.flagL            = g_lineDetector.isLeftLine()   ? 1 : 0;
        e.flagK            = g_lineDetector.isCrossLine()  ? 1 : 0;
        e.flagR            = g_lineDetector.isRightLine()  ? 1 : 0;
        e.flagS            = g_lineDetector.isCenterLine() ? 1 : 0;
        e.total            = (signed int)g_timer_1ms;
        for (int i = 0; i < 160; i++)
        {
          e.imageData0[i] = g_camera.getPixel(i, RunController::TRACE_ROW);
        }
        g_sdlogger.commit();
      }
    }

    // 走行終了時に1回だけSD保存
    if (g_runController.isFinished() && !savedToSD)
    {
      savedToSD = true;
      g_serial.printf("\n*** 走行終了 → ログ保存中 ***\n");
      g_motor.set(0, 0);
      g_servo.setAngle(0);
      g_sdlogger.saveToSD();
      g_serial.printf("*** ログ保存完了 ***\n");
    }

    // 一定間隔でステータス表示
    if (g_cnt_printf >= DEBUG_PRINT_INTERVAL_MS)
    {
      g_cnt_printf = 0;
      g_serial.printf(
          "\033[H"
          "T=%lu Frame=%lu Pat=%d Handle=%d Cross=%d Lf=%d Rt=%d Ce=%d Dev=%d  Log=%u   \r\n",
          g_timer_1ms, s_frameCount,
          g_runController.getPattern(),
          g_runController.getHandleVal(),
          g_lineDetector.isCrossLine() ? 1 : 0,
          g_lineDetector.isLeftLine() ? 1 : 0,
          g_lineDetector.isRightLine() ? 1 : 0,
          g_lineDetector.isCenterLine() ? 1 : 0,
          g_lineDetector.getDeviation(RunController::TRACE_ROW),
          g_sdlogger.getLogCount());
    }
  }
}

// ====================================================================
// デバッグモード: 既存のカメラ画像シリアル表示
// ====================================================================
static void runDebugLoop(void)
{
  g_serial.printf("\n--- デバッグモード ---\n");
  g_serial.printf("デバッグ表示: 閾値=%d\n", DEBUG_THRESHOLD);
  g_serial.printf("0/1表示: 0=暗い, 1=明るい（閾値以上）\n\n");

  int x, y, c;
  static char lineBuf[2560];

  while (1)
  {
    // フレーム更新完了をカウント
    if (g_camera.isFrameReady())
    {
      s_frameCount++;
      g_camera.clearFrameReady();
      // フレーム完了時にライン検出を実行
      g_lineDetector.update();
    }

    // 一定間隔でシリアル出力
    if (g_cnt_printf >= DEBUG_PRINT_INTERVAL_MS)
    {
      g_cnt_printf = 0;

      // カーソルを左上に移動して上書き描画
      g_serial.printf("\033[H");

      // タイマー値・フレーム更新カウンタを表示
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

      // ヘッダ行
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

      // 30行目〜100行目を2行飛ばしで表示
      for (y = 30; y < 100; y += 2)
      {
        int pos = 0;
        lineBuf[pos++] = '0' + (y / 100) % 10;
        lineBuf[pos++] = '0' + (y / 10) % 10;
        lineBuf[pos++] = '0' + y % 10;
        lineBuf[pos++] = ':';

        for (x = 0; x < 160; x++)
        {
          c = g_camera.getPixel(x, y) >= DEBUG_THRESHOLD ? 1 : 0;

          // 偏差位置（検出されたライン中心）を黄色背景でハイライト
          if (x == -g_lineDetector.getDeviation(y) + LineDetector::CENTER)
          {
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
            lineBuf[pos++] = '0';
          }
        }
        lineBuf[pos++] = ' ';
        lineBuf[pos++] = ' ';
        lineBuf[pos++] = '\r';
        lineBuf[pos++] = '\n';
        lineBuf[pos] = '\0';
        g_serial.print(lineBuf);
      }
    }
  }
}

int main(void)
{
  // MMU + L1 キャッシュ有効化
  SystemInit();

  // オンボードLED/SWの初期化
  g_onboard.init();

  // シリアル通信初期化 (230400bps)
  g_serial.init();
  g_serial.printf("\033[2J\033[H"); // 画面クリア & カーソルホーム
  g_serial.printf("\x1b[36m--- MCR Camera 2026 Base ---\x1b[39m\n");

  // 起動時にスイッチが押されていたらデバッグモード
  // （カメラ初期化前なのでシリアル出力で確認できる）
  s_debugMode = (g_onboard.sw() != 0);
  g_serial.printf("起動モード: %s\n", s_debugMode ? "デバッグモード" : "走行モード");

  g_serial.printf("GIC初期化中...\n");
  initGIC();
  g_serial.printf("GIC初期化完了\n");

  // SDLogger初期化（SDカード + FatFs）
  g_sdlogger.init();

  g_serial.printf("カメラ初期化中...\n");
  g_camera.init();
  g_serial.printf("カメラ初期化完了\n");

  // OSTM0タイマー割り込みを設定・開始（1ms周期）
  initOSTM0();

  // モーター初期化
  g_motor.init();
  g_serial.printf("モーター初期化完了\n");

  // サーボ初期化
  g_servo.init();
  g_serial.printf("サーボ初期化完了\n");

  // ライン検出初期化
  g_lineDetector.init();
  g_serial.printf("ライン検出初期化完了\n");

  // 走行制御初期化
  g_runController.init();
  g_serial.printf("走行制御初期化完了\n");

  g_serial.printf("タイマー開始\n");

  if (s_debugMode)
  {
    runDebugLoop();
  }
  else
  {
    runMainLoop();
  }

  return 0;
}
