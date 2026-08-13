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

// =====================================================================
// グローバル C++ コンストラクタ手動実行
//
// 本プロジェクトの start.S は HardwareSetup() の後に main() を直接呼び、
// __libc_init_array() を呼ばない。GCC 13 はグローバル ctor を
// .init_array セクションに出力するため、main() 冒頭で手動で反復実行する
// 必要がある（さもなければ Servo/Motor/LineDetector 等の _config メンバが
// BSS ゼロのままになり、サーボ PWM 出力が 0 になる等の致命的不具合になる）。
//
// linker_script.ld 側で .init_array を .tors セクションに収集し、
// __init_array_start / __init_array_end ラベルを定義している。
// =====================================================================
extern "C" {
  typedef void (*ctor_fn)(void);
  extern ctor_fn __init_array_start[];
  extern ctor_fn __init_array_end[];
}

static void runGlobalConstructors(void)
{
  for (ctor_fn *p = __init_array_start; p < __init_array_end; ++p)
  {
    (*p)();
  }
}

// =====================================================================
// ベアメタル向けダミーシンボル
//
// グローバル ctor を有効化すると、GCC が以下を参照するようになるが、
// 本プロジェクトはベアメタル (newlib stubs 不使用 / dynamic loader 無し)
// のため、それぞれ意味を持たない。リンクを通すためダミーで定義する。
//
//   __dso_handle : __cxa_atexit() 用の DSO ハンドル。dynamic shared
//                  object 識別子だが、ベアメタル環境では単一実行像
//                  のため任意の固定値で良い。
//   _fini        : libc の __libc_fini_array() が呼ぶ「全 fini 実行」
//                  関数。本プロジェクトでは exit に到達しないため空。
//   __cxa_atexit : 静的 dtor 登録 (本プロジェクトは exit しないので
//                  常に成功扱いで何もしない)。
// =====================================================================
extern "C" {
  void *__dso_handle = (void *)&__dso_handle;
  void _fini(void) {}
  int  __cxa_atexit(void (*)(void *), void *, void *) { return 0; }
}

#include "drivers/Camera.h"
#include "drivers/Onboard.h"
#include "drivers/Serial.h"
#include "system/system_init.h"
#include "drivers/Motor.h"
#include "drivers/Servo.h"
#include "drivers/LineDetector.h"
#include "drivers/SDLogger.h"
#include "drivers/Encoder.h"
#include "core/SystemData.h"
#include "core/ProjectConfig.h"
#include "logic/RunLogic.h"

// OSTM0 タイマー割り込み (1ms周期)
// GR-PEACH (RZ/A1H) の周辺クロック P0Φ は 33.33MHz
// 1ms = 33333 カウント (33.33e6 / 1000)
// 注: 走行パラメータは ProjectConfig.h に集約する方針だが、この値はハード
//     クロックから一意に決まる定数なので main 側に残してある。
#define OSTM0_CMP_1MS 33333

// デバッグ表示用: 閾値設定
// 170 = 画素輝度 (0-255) の二値化しきい値。デバッグモードの 0/1 表示専用で、
// 走行制御には使われない。
// 注: 同じ 170 が CAMERA_CONFIG.thresholdDefault (ProjectConfig.h:42) と
//     LINE_DETECTOR_CONFIG.centerBrightnessAbs (:73) にも書かれており、
//     用途の違う 3 箇所に同値が独立して存在する（連動しない）。
#define DEBUG_THRESHOLD 170         // 画像二値化の閾値
#define DEBUG_PRINT_INTERVAL_MS 200 // シリアル出力間隔(ms)。g_cnt_printf は
                                    // 1ms 割り込みで ++ されるので値がそのまま ms

// グローバルタイマーカウンタ
volatile unsigned long g_timer_1ms = 0;

// シリアル出力用カウンタ
volatile unsigned long g_cnt_printf = 0;

// SystemData グローバルインスタンス（EMA 準拠の共有データハブ）
// 全ての updateInput / updateOutput 呼び出しで参照される。
SystemData g_sys;

// フレーム更新カウンタ
static unsigned long s_frameCount = 0;

// 動作モード（main 起動時にスイッチ状態で決定）
//   true  = デバッグモード（カメラ画像のシリアル表示）
//   false = 走行モード（RunController が状態遷移を進める）
static bool s_debugMode = false;

// 各モジュールインスタンス（グローバル, Config 注入）
// EMA 準拠: 全モジュールのインスタンス定義を main 側に集約
Serial       g_serial      (SERIAL_CONFIG);
Onboard      g_onboard     (ONBOARD_CONFIG);
Motor        g_motor       (MOTOR_CONFIG);
Servo        g_servo       (SERVO_CONFIG);
Encoder      g_encoder     (ENCODER_CONFIG);  // 現状 init/呼出は未接続
Camera       g_camera      (CAMERA_CONFIG);
LineDetector g_lineDetector(LINE_DETECTOR_CONFIG);
SDCard       g_sdcard      (SDCARD_CONFIG);
SDLogger     g_sdlogger    (SDLOGGER_CONFIG);

// ====================================================================
// IModule* 配列 — EMA 準拠の 3 フェーズ呼出
//
// LineDetector / SDLogger は処理時間 / バス制約のため ISR から外し、
// メインループで sys.cam.frameReady 駆動で呼ぶ。
// Encoder も現状は ISR 配列に未登録（Step 12 SDLogger 連携時に有効化）。
// ====================================================================
static IModule* s_inputModules[] = {
    &g_onboard,    // Input  : SW 状態を sys.ob.sw に
    &g_camera,     // Input  : 1ms ごとのフレーム段階処理 + sys.cam.* 更新
};
static const int N_IN = sizeof(s_inputModules) / sizeof(IModule*);

static IModule* s_outputModules[] = {
    &g_motor,      // Output : sys.mot.*Cmd → PWM
    &g_servo,      // Output : sys.srv.angleCmd → PWM
    &g_onboard,    // Output : sys.ob.*LedCmd → GPIO
};
static const int N_OUT = sizeof(s_outputModules) / sizeof(IModule*);

extern "C" void ostm0_interrupt_callback(void);

// GIC (Generic Interrupt Controller) のグローバル有効化
//
// RZ/A1H の GIC はディストリビュータ (INTC.ICD*) と CPU インターフェース
// (INTC.ICC*) の2段構成。ディストリビュータが割り込み源ごとの許可/優先度/
// 配信先 CPU を管理し、CPU インターフェースが CPU への IRQ 通知を行う。
// 両方を有効にしないと個別 IRQ を許可しても CPU には届かない。
static void initGIC(void)
{
  // GICディストリビュータ有効化 (ICDDCR bit0 = Enable)
  INTC.ICDDCR = 0x01;

  // GIC CPUインターフェース有効化 (ICCICR bit0 = Enable)
  INTC.ICCICR = 0x01;

  // 割り込み優先度マスク: 全割り込みを許可
  // GIC は優先度の数値が小さいほど高優先。ICCPMR に最大値 0xFF を入れると
  // 全優先度が通る（= マスクしない）。個別 IRQ の優先度は ICDIPRn で設定する。
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
  //   bit1 = 動作モード選択（インターバルタイマー / フリーランコンペア）
  //   bit0 = カウント開始時の割り込み出力設定
  // 0x01 がインターバルタイマー動作。iodefine.h 側にビットフィールド定義が
  // 無いため、各ビットの正確な定義は RZ/A1H ハードウェアマニュアルの
  // OSTMnCTL を参照すること。
  OSTM0.OSTMnCTL = 0x01;

  // ------------------------------------------------------------------
  // GIC設定: OSTM0割り込みを有効化
  //
  // OSTM0 の割り込み ID は **134**。
  //   根拠: generate/vects.c の RelocatableVectors テーブルで OSTMI0 の
  //         エントリがオフセット 0x218。1 エントリ 4 バイトなので
  //         0x218 / 4 = 134。
  //
  // 以下のレジスタ添字とビット位置はすべてこの ID から機械的に決まる。
  // ID を変えたら同じ式で計算し直すこと。
  //
  //   ICDISERn / ICDICERn / ICDICPRn : 1 IRQ = 1 bit → 32 IRQ / レジスタ
  //       添字  = ID / 32 = 134 / 32 = 4
  //       ビット = ID % 32 = 134 % 32 = 6        → (1 << 6)
  //
  //   ICDICFRn : 1 IRQ = 2 bit → 16 IRQ / レジスタ
  //       添字  = ID / 16 = 134 / 16 = 8
  //       シフト = (ID % 16) * 2 = 6 * 2 = 12    → (3 << 12) が該当 2 bit
  //
  //   ICDIPRn / ICDIPTRn : 1 IRQ = 8 bit → 4 IRQ / レジスタ
  //       添字  = ID / 4 = 134 / 4 = 33
  //       シフト = (ID % 4) * 8 = 2 * 8 = 16     → (0xFF << 16) が該当 1 byte
  // ------------------------------------------------------------------

  // ICDICER4: 設定変更中の誤発火を避けるため、いったん割り込み禁止
  INTC.ICDICER4 = (1 << 6);
  // ICDICPR4: 残っている保留フラグをクリア
  INTC.ICDICPR4 = (1 << 6);
  // ICDISER4: 割り込み許可
  INTC.ICDISER4 |= (1 << 6);

  // エッジトリガ設定 (ICDICFR8 の bit13-12)
  // 2 bit フィールドの上位ビットが 1 でエッジトリガ、0 でレベルセンシティブ。
  // → 0b10 = 2 を書いてエッジトリガにする。
  uint32_t icf = INTC.ICDICFR8;
  icf &= ~(3 << 12);
  icf |= (2 << 12);
  INTC.ICDICFR8 = icf;

  // 割り込み優先度設定 (ICDIPR33 のバイト2 = bit23-16)
  // 0x80 は中位の優先度。GIC は数値が小さいほど高優先度で、
  // 上の ICCPMR = 0xFF より小さいのでマスクされずに通る。
  // なお下位数ビットは実装されない場合があり、実効の粒度はハード依存。
  uint32_t ipr = INTC.ICDIPR33;
  ipr &= ~(0xFF << 16);
  ipr |= (0x80 << 16);
  INTC.ICDIPR33 = ipr;

  // 割り込みプロセッサターゲット設定 (ICDIPTR33 のバイト2 = bit23-16)
  // 1 bit が CPU 1 個に対応。0x01 = CPU0 のみに配信する
  // （RZ/A1H は Cortex-A9 シングルコアなので CPU0 固定でよい）。
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
// 到達経路:
//   IRQ → INT_Excep_IRQ() が ICCIAR から割り込み ID を読む
//     → g_irq_handlers[ID] が登録済みならそれを呼ぶ（VDC5 用の動的登録）
//     → 未登録なら RelocatableVectors[ID] にフォールバック
//   OSTM0 は g_irq_handlers に登録していないので **後者**の経路を通り、
//   RelocatableVectors[134] = INT_Excep_OSTMI0() 経由でここに来る。
//
// 参考プロジェクト(2.38m-s)の intTimer() に相当する。
//
// 周期と実時間のズレについて:
//   本関数は 1ms 周期で起動する想定だが、Camera::updateInput() の 1 ステップ
//   （imageCopy / extractBrightness）は 1ms を超える。処理が長引いている間に
//   来た OSTM0 割り込みは、GIC 側で保留されても **保留は 1 回分にまとめられる**
//   ため、超過分の tick はそのまま失われる。つまり g_timer_1ms は実時間より
//   遅れる方向にズレる（進みすぎることはない）。
//   走行ロジックのタイマーはすべて g_timer_1ms 基準なので、実時間との差は
//   ステップ処理の重さに比例する。厳密な実時間が要るなら別途 OSTM1 等で
//   フリーランカウンタを持つ必要がある。
//
//   なお本関数自体の再入は起きない。IRQ ハンドラ実行中は CPSR の I ビットで
//   IRQ がマスクされ、INT_Excep_IRQ() を抜けるまで次の IRQ は入らない。
//   4 ステップ完了後は Vfield 待ちで即リターンするので、その間に
//   メインループへ CPU 時間が返る。
void ostm0_interrupt_callback(void)
{
  g_timer_1ms++;
  g_cnt_printf++;

  // ============ Input フェーズ ============
  // ハードウェア → SystemData
  for (int i = 0; i < N_IN; ++i)
  {
    if (s_inputModules[i]->enabled)
    {
      s_inputModules[i]->updateInput(g_sys);
    }
  }

  // ============ Logic フェーズ ============
  // SystemData → SystemData
  // EMA 準拠: 走行ロジックは src/logic/RunLogic.cpp の純関数群が担当。
  if (!s_debugMode)
  {
    applyDrivingPattern(g_sys);
  }
  else
  {
    // デバッグモードでも USER_LED は中央2点センサ反応で点灯させる
    // 0x18 = 0b0001_1000 = bit4|bit3。Camera::thresholdConvert() の割り当てで
    // bit7 が最左 (x=31)、bit0 が最右 (x=128) なので、bit4/bit3 は x=71/x=88
    // ＝画像中央 (x=80) を挟む 2 点にあたる。
    // LineDetector.h の MASK 定数群に該当する値が無いためリテラルのまま。
    // RunLogic.cpp の applyDrivingPattern() と同じ式が意図的に重複している。
    g_sys.ob.userLedCmd = (g_sys.line.sensorBin & 0x18) ? 1 : 0;
  }

  // ============ Output フェーズ ============
  // SystemData → ハードウェア
  for (int i = 0; i < N_OUT; ++i)
  {
    if (s_outputModules[i]->enabled)
    {
      s_outputModules[i]->updateOutput(g_sys);
    }
  }
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
    // フレーム完了時の処理: ライン検出を main loop で実行
    if (g_sys.cam.frameReady)
    {
      s_frameCount++;
      g_sys.cam.frameReady = false;
      g_lineDetector.updateInput(g_sys);
    }

    // SDLogger を毎周期呼ぶ (内部で走行中ログ自動記録 + saveRequested 処理)
    // ISR ではなくここから呼ぶ。SD/FatFs のタイミング制約に加え、保存時の
    // saveToSD() が秒オーダーでブロックするため（詳細は SDLogger.h 冒頭）。
    g_sdlogger.updateOutput(g_sys);

    // 走行終了時に1回だけSD保存リクエスト
    if (g_sys.run.finished && !savedToSD)
    {
      savedToSD = true;
      g_serial.printf("\n*** 走行終了 → ログ保存中 ***\n");
      g_sys.mot.leftCmd  = 0;
      g_sys.mot.rightCmd = 0;
      g_sys.srv.angleCmd = 0;
      g_sys.sd.saveRequested = true;
      // 即座に保存実行。ここは最大 4000 エントリ × 160 画素の CSV 書き出しで
      // 秒オーダーかかり、その間メインループは止まる。
      // 一方 1ms 割り込みは走り続けるので、Camera のフレーム処理と
      // Motor/Servo への出力は継続する。
      // 直前の出力ゼロクリアは保存中に走り続けないための保険だが、ISR の
      // Logic フェーズ (runFinish) が毎周期 sys.mot / sys.srv を上書きするため、
      // 実際に停止を決めているのは runFinish 側である。
      g_sdlogger.updateOutput(g_sys);  // 即座に保存実行
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
          g_sys.run.pattern,
          g_sys.run.handleVal,
          g_sys.line.crossLine ? 1 : 0,
          g_sys.line.leftLine  ? 1 : 0,
          g_sys.line.rightLine ? 1 : 0,
          g_sys.line.centerLine? 1 : 0,
          g_sys.line.deviation[RUN_CONFIG.traceRow],
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
    if (g_sys.cam.frameReady)
    {
      s_frameCount++;
      g_sys.cam.frameReady = false;
      // フレーム完了時にライン検出を実行
      g_lineDetector.updateInput(g_sys);
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
  // ★ 最初に C++ グローバルコンストラクタを実行する。
  //   start.S が __libc_init_array を呼ばないため、
  //   ここで実行しないと g_servo / g_motor 等の _config メンバが
  //   未初期化のまま (BSS ゼロ) となり、PWM 出力が壊れる。
  runGlobalConstructors();

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

  // モーター初期化
  g_motor.init();
  g_serial.printf("モーター初期化完了\n");

  // サーボ初期化
  g_servo.init();
  g_serial.printf("サーボ初期化完了\n");

  // ライン検出初期化
  g_lineDetector.init();
  g_serial.printf("ライン検出初期化完了\n");

  // 走行制御初期化 (Logic レイヤー)
  runLogicInit(g_sys);
  g_serial.printf("走行制御初期化完了\n");

  // ★ OSTM0 (1ms 割り込み) は全ドライバ init 完了後に最後に開始する。
  //   ISR の Output フェーズが Motor/Servo を毎ms 叩くため、ここより前で
  //   起動すると MTU2 がスタンバイ状態のまま PWM レジスタに書き込みが入り、
  //   バス例外/ハングの原因になる（EMA 化で 3 フェーズ全実行になったため）。
  initOSTM0();

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
