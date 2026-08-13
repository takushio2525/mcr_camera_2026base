/*
 * Camera.cpp
 *
 *  NTSC Video Capture Driver Implementation
 *  VDC5 Channel 0 + DVDEC0 でNTSCアナログ映像を
 *  160x120 YCbCr422 でキャプチャし、輝度(Y)データを提供する
 */

#include "Camera.h"
#include "Serial.h"
#include "../core/SystemData.h"
#include "iodefine.h"
#include <stdlib.h>
#include <string.h>

// ====================================================================
// VDC5ビデオキャプチャ用フレームバッファ（NonCacheable領域に配置）
//
// 2 面を確保しダブルバッファに切り替えられる形にはなっているが、
// 切替を行う changeFrameBuffer() が現状どこからも呼ばれないため、
// **実際にはシングルバッファ運用**になっている:
//   - VDC5 は init() の Video_Write_Setting() で指定した s_frameBufA に書き続ける
//   - imageCopy() も s_writeBuf (= s_frameBufA) から直接読む
//   - s_saveBuf / s_frameBufB は書き込まれるだけで読み出されない
// フィールドの整合は Vfield 割り込み待ち (updateInput) で取っている。
// ====================================================================
static uint8_t s_frameBufA[CAM_VIDEO_BUFFER_STRIDE * CAM_PIXEL_VW]
    __attribute__((section("NC_BSS"), aligned(32)));
static uint8_t s_frameBufB[CAM_VIDEO_BUFFER_STRIDE * CAM_PIXEL_VW]
    __attribute__((section("NC_BSS"), aligned(32)));

// 現在VDC5が書き込み中のバッファと、ソフトウェアが読み出すバッファ
// （s_saveBuf は changeFrameBuffer() が未使用のため現状ずっと s_frameBufB のまま）
static volatile uint8_t *s_writeBuf = s_frameBufA;
static volatile uint8_t *s_saveBuf = s_frameBufB;

// Vsync/Vfieldカウンタ
//
// これらと上の s_writeBuf / s_saveBuf は、次の 2 つの実行文脈から触られる:
//   - VDC5 割り込み (vsyncCallback / vfieldCallback)
//   - OSTM0 1ms 割り込み内の Camera::updateInput()
// 排他手段は volatile によるコンパイラ最適化抑止だけで、クリティカル
// セクション（割り込み禁止）は張っていない。成立している前提は以下:
//   - いずれも 32bit アライン変数なので単一命令の load/store になり、
//     途中の値を読むこと（tearing）は無い
//   - s_vsyncCount は「割り込みがデクリメント、待ち側は読むだけ」
//   - s_vfieldToggle は「割り込みが書き、updateInput は読むだけ」
// 例外は updateInput() 側の s_vfieldCount = 0（消費）で、そこは
// read-modify-write になる。詳細は updateInput() 内のコメントを参照。
static volatile int32_t s_vsyncCount = 0;
static volatile int32_t s_vfieldCount = 0;
static volatile int32_t s_vfieldToggle = 1;

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

// ====================================================================
// コンストラクタ (Config 注入)
// ====================================================================
Camera::Camera(const CameraConfig& cfg)
    : _config(cfg),
      frameStep_(0), fieldToggle_(1), capturedField_(0),
      frameReady_(false), frameCount_(0)
{
  memset((void *)imageBuffer_, 0, sizeof(imageBuffer_));
  memset(ycbcrBuffer_, 0, sizeof(ycbcrBuffer_));
}

// ====================================================================
// VDC5 + DVDEC 初期化
// ====================================================================
// 各ステップの失敗時は false を返して呼び出し側に判断を委ねる。
// （以前はここで while(1); と無限ループしていたため、カメラ未接続時に
//  GIC 初期化直後で完全にハングし、シリアル出力も走行も一切行われなかった。
//  現在は main() の initModule() が enabled=false に落とすので、カメラ無しでも
//  シリアル表示とメインループは動き続ける。）
bool Camera::init()
{
  DisplayBase::graphics_error_t error;

  // グローバルコンストラクタが呼ばれない環境への対策として、ここで明示的に初期化する
  // 注: この前提はコミット 40eca9b で変わった。現在は main() 冒頭の
  //     runGlobalConstructors() が .init_array を手動反復するのでコンストラクタは
  //     実行される。よって以下はコンストラクタと重複した再初期化になっている
  //     （害はないが、消すなら runGlobalConstructors() が動くことが前提）。
  display_ = DisplayBase();
  frameStep_ = 0;
  fieldToggle_ = 1;
  capturedField_ = 0;
  frameReady_ = false;
  memset((void *)imageBuffer_, 0, sizeof(imageBuffer_));
  memset(ycbcrBuffer_, 0, sizeof(ycbcrBuffer_));

  g_serial.printf("[Camera::init] Start Graphics_init...\n");
  // 1. Graphics initialization process
  error = display_.Graphics_init(NULL);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf("[Camera::init] ERROR at Graphics_init: %d\n", error);
    return false;
  }
  g_serial.printf("[Camera::init] Graphics_init OK.\n");

  g_serial.printf("[Camera::init] Start Graphics_Video_init...\n");
  // 2. Video decoder initialization
  error = display_.Graphics_Video_init(DisplayBase::INPUT_SEL_VDEC, NULL);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf("[Camera::init] ERROR at Graphics_Video_init: %d\n", error);
    return false;
  }
  g_serial.printf("[Camera::init] Graphics_Video_init OK.\n");

  g_serial.printf("[Camera::init] Start Graphics_Irq_Handler_Set (VSYNC)...\n");
  // 3. Vsync callback setting
  error = display_.Graphics_Irq_Handler_Set(DisplayBase::INT_TYPE_S0_VI_VSYNC,
                                            0, vsyncCallback);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf(
        "[Camera::init] ERROR at Graphics_Irq_Handler_Set (VSYNC): %d\n",
        error);
    return false;
  }
  g_serial.printf("[Camera::init] Graphics_Irq_Handler_Set (VSYNC) OK.\n");

  g_serial.printf("[Camera::init] Start Video_Write_Setting...\n");
  // 4. Video write setting (NTSC, YCbCr422, 160x120)
  error = display_.Video_Write_Setting(
      DisplayBase::VIDEO_INPUT_CHANNEL_0, DisplayBase::COL_SYS_NTSC_358,
      (void *)s_frameBufA, CAM_VIDEO_BUFFER_STRIDE,
      DisplayBase::VIDEO_FORMAT_YCBCR422, DisplayBase::WR_RD_WRSWA_32_16BIT,
      CAM_PIXEL_VW, CAM_PIXEL_HW);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf("[Camera::init] ERROR at Video_Write_Setting: %d\n", error);
    return false;
  }
  g_serial.printf("[Camera::init] Video_Write_Setting OK.\n");

  g_serial.printf(
      "[Camera::init] Start Graphics_Irq_Handler_Set (VFIELD)...\n");
  // 5. Vfield callback setting (VIDEO_INT_TYPE = INT_TYPE_S0_VFIELD)
  error = display_.Graphics_Irq_Handler_Set(DisplayBase::INT_TYPE_S0_VFIELD, 0,
                                            vfieldCallback);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf(
        "[Camera::init] ERROR at Graphics_Irq_Handler_Set (VFIELD): %d\n",
        error);
    return false;
  }
  g_serial.printf("[Camera::init] Graphics_Irq_Handler_Set (VFIELD) OK.\n");

  // 6. Capture Start -> Stop -> Start (参考プロジェクトと同じ初期化シーケンス)
  // XIP環境ではbusy-waitループが極めて遅い（L1キャッシュ無効でSPI Flashから
  // 毎回フェッチするため実効2〜5MHz相当 → 6,000,000回で30秒以上かかる）。
  // 代わりにVideo_Startを先に呼んでVsync割り込みを発生させ、
  // 割り込みベースのカウントで約200msを測る（NTSC 60Hz × 12回 = 200ms）。
  //
  // 注: 上の「L1キャッシュ無効」という前提は現在は成り立たない。
  //     コミット 3216ec4 以降 main() が SystemInit() を呼び、
  //     system_init.c で MMU_Enable() / L1C_EnableCaches() を実行している。
  //     実効速度は改善しているはずだが再測していない（要実機計測）。
  //     いずれにせよ本待ちは Vsync 割り込み駆動なので実行速度に依存しない。
  g_serial.printf("[Camera::init] Video_Start (for Vsync stabilize wait)...\n");
  s_vsyncCount = 12; // Vsync 12回 ≈ 200ms
  display_.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);

  // Vsync 12回分待ち（割り込み駆動: XIP速度に依存しない）
  // timeout はハングアップ防止のフェイルセーフ。単位は「時間」ではなく
  // 空ループの回転数なので、実時間は実行速度（XIP / キャッシュ状態）で変わる。
  // 桁の決め方に根拠はなく、Vsync が来ない異常時に抜けられれば十分という値。
  for (volatile int timeout = 0; s_vsyncCount > 0 && timeout < 50000000;
       timeout++)
  {
  }
  g_serial.printf("[Camera::init] Stabilized (12 Vsyncs). Restart...\n");

  display_.Video_Stop(DisplayBase::VIDEO_INPUT_CHANNEL_0);
  display_.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);
  g_serial.printf("[Camera::init] Sequence done.\n");

  // 参考プロジェクトと同じ: Vsync/Vfield待ちでVDC5の安定を確認
  // WaitVsync(1) 相当: Vsyncが1回発生するまで待つ
  s_vsyncCount = 1;
  // timeout は上と同じくフェイルセーフの空ループ上限（待つ Vsync が
  // 12 回→1 回なので桁も 1 つ小さい。厳密な根拠は無い）。
  for (volatile int timeout = 0; s_vsyncCount > 0 && timeout < 5000000;
       timeout++)
  {
  }
  g_serial.printf("[Camera::init] WaitVsync done.\n");

  // WaitVfield(2) 相当: Vfieldが2回発生するまで待つ
  // s_vfieldCount は単調増加カウンタなので開始値からの差分で判定する
  {
    volatile int32_t vfield_start = s_vfieldCount;
    // timeout は上と同じくフェイルセーフの空ループ上限（根拠となる実測値は無い）。
    for (volatile int timeout = 0;
         (s_vfieldCount - vfield_start) < 2 && timeout < 10000000;
         timeout++)
    {
    }
  }
  g_serial.printf(
      "[Camera::init] All initialization completed successfully.\n");
  return true;
}

// ====================================================================
// ビデオキャプチャ開始
// ====================================================================
void Camera::startCapture()
{
  display_.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);
}

// ====================================================================
// ビデオキャプチャ停止
// ====================================================================
void Camera::stopCapture()
{
  display_.Video_Stop(DisplayBase::VIDEO_INPUT_CHANNEL_0);
}

// ====================================================================
// フレームバッファ切替（ダブルバッファ方式）
//
// 現状どこからも呼ばれていない（呼ぶと imageCopy() の読み出し先も
// 切り替わるため、s_writeBuf / s_saveBuf の役割を見直す必要がある）。
// ====================================================================
void Camera::changeFrameBuffer()
{
  if (s_writeBuf == s_frameBufA)
  {
    s_writeBuf = s_frameBufB;
    s_saveBuf = s_frameBufA;
  }
  else
  {
    s_writeBuf = s_frameBufA;
    s_saveBuf = s_frameBufB;
  }

  // 新しいバッファアドレスをVDC5に設定
  display_.Video_Write_Change(DisplayBase::VIDEO_INPUT_CHANNEL_0,
                              (void *)s_writeBuf, CAM_VIDEO_BUFFER_STRIDE);
}

// ====================================================================
// Vfieldコールバック（VDC5割り込みから呼ばれる）
// ====================================================================
void Camera::vfieldCallback(DisplayBase::int_type_t int_type)
{
  (void)int_type;
  // 単調増加カウンタ: update() が消費する (0にリセット)
  // 何回トグルしても「1回以上来たか」を確実に検出できる
  s_vfieldCount++;
  // トップ/ボトムフィールドのトグル
  s_vfieldToggle = (s_vfieldToggle == 0) ? 1 : 0;
}

// ====================================================================
// Vsyncコールバック
// ====================================================================
void Camera::vsyncCallback(DisplayBase::int_type_t int_type)
{
  (void)int_type;
  if (s_vsyncCount > 0)
  {
    s_vsyncCount--;
  }
}

// ====================================================================
// update(): フレーム周期ステップ処理
// 1ms割り込み(OSTM0)から呼ばれ、ステップごとに画像処理を分割実行
// 参考プロジェクト(2.38m-s)の intTimer() 内 switch(counter++) と同等
//
// 参考プロジェクトと同じ割り込み内実行方式:
// - 1回の割り込みで1ステップ（imageCopyまたはextractBrightness）を実行
// - XIPではstep毎に~8msかかるが、4ステップ完了後のVfield待ち期間に
//   default:break で即リターンし、メインループにCPU時間を返す
// - フレーム開始時にs_vfieldToggleをcapturedField_にキャプチャし、
//   4ステップ全体で同じフィールド値を使用する
// ====================================================================
void Camera::updateInput(SystemData& sys)
{
  // メインループが sys.cam.frameReady を false に戻したら、
  // 内部状態もリセットして次フレームの段階処理を開始可能にする。
  if (!sys.cam.frameReady && frameReady_)
  {
    frameReady_ = false;
  }

  // フレーム処理完了後: 次の Vfield 発生を待って新フレームを開始
  // s_vfieldCount > 0 なら 1回以上 Vfield が来ているので即開始
  // 待ち中はすぐにリターンするため、メインループにCPU時間が返る
  if (frameStep_ > 3)
  {
    if (s_vfieldCount > 0)
    {
      // 「読んで 0 を書く」read-modify-write なので、この 2 命令の間に
      // VDC5 の Vfield 割り込みが入ると、そこで ++ された分が 0 で
      // 上書きされて 1 フィールド分取りこぼす。
      // ここは「1 回以上来たか」だけを見る使い方であり、取りこぼしても
      // 次のフィールドで開始が 1 回遅れるだけなので許容している
      // （s_vfieldCount -= consumed のような形にはしていない）。
      s_vfieldCount = 0; // 消費
      frameStep_ = 0;
      frameReady_ = false;
    }
    else
    {
      return; // Vfield待ち: 即リターン
    }
  }

  switch (frameStep_++)
  {
  case 0:
    // ステップ0: フィールド値をキャプチャし、YCbCr422コピー（前半0-59行）
    capturedField_ = (int)s_vfieldToggle;
    imageCopy(0);
    break;

  case 1:
    // ステップ1: YCbCr422 生データのコピー（後半60-119行）
    imageCopy(1);
    break;

  case 2:
    // ステップ2: 輝度抽出（前半0-59行）
    extractBrightness(0);
    break;

  case 3:
    // ステップ3: 輝度抽出（後半60-119行）→ フレームデータ確定
    extractBrightness(1);
    frameReady_ = true;
    frameCount_++;
    break;

  default:
    break;
  }

  // SystemData へ最新状態を反映（毎周期）
  //
  // sys.cam.frameReady はメインループが false を書き、この ISR が
  // frameReady_ で毎周期上書きする。割り込み禁止では守っていない。
  //   - 関数冒頭の「!sys.cam.frameReady && frameReady_ → frameReady_ = false」で
  //     メインループ側の消費を内部状態に取り込む
  //   - ただし 4 ステップ完了後に次の Vfield が来ると、消費されたかどうかに
  //     関わらず frameStep_ = 0 / frameReady_ = false にして次フレームを始める
  // したがってメインループが遅れると frameReady == true の窓を取りこぼしうる。
  // その場合はそのフレームの LineDetector 実行が 1 回飛ぶだけで破綻はしない。
  sys.cam.frameReady     = frameReady_;
  sys.cam.frameCount     = frameCount_;
  sys.cam.field          = capturedField_;
  sys.cam.imageBufferPtr = imageBuffer_;
}

// ====================================================================
// ImageCopy: VDC5フレームバッファから中間バッファへコピー
// インターレース対応：トップ/ボトムフィールドを交互に取得
// half: 0=前半(0-59行), 1=後半(60-119行)
// ====================================================================
void Camera::imageCopy(int half)
{
  const int hwTwice = CAM_PIXEL_HW * 2; // YCbCr422は2バイト/ピクセル
  // 参考プロジェクトと同様に、VDC5が書き込み中のバッファから直接読む
  // （s_saveBuf ではなく s_writeBuf を使用する）
  const volatile uint8_t *src = s_writeBuf;
  // フレーム開始時にキャプチャしたフィールド値を使用
  // （参考プロジェクトでは4ステップが4ms以内に完了するため自然に同一値だが、
  //  XIPでは各ステップが重く途中でVfieldが変わるため明示的に固定する）
  const int frame = capturedField_;

  if (half == 0)
  {
    // 前半: トップ/ボトムフィールドの前半(0〜59行)をコピー
    for (int y = frame; y < (int)(CAM_PIXEL_VW / 2); y += 2)
    {
      for (int x = 0; x < hwTwice; x++)
      {
        ycbcrBuffer_[y * hwTwice + x] = src[y * hwTwice + x];
      }
    }
  }
  else
  {
    // 後半: トップ/ボトムフィールドの後半(60〜119行)をコピー
    for (int y = (int)(CAM_PIXEL_VW / 2) + frame; y < (int)CAM_PIXEL_VW; y += 2)
    {
      for (int x = 0; x < hwTwice; x++)
      {
        ycbcrBuffer_[y * hwTwice + x] = src[y * hwTwice + x];
      }
    }
    // もう一方のフィールドラインを黒で埋める（参考プロジェクトと同じ）
    int otherField = (frame == 0) ? 1 : 0;
    for (int y = otherField; y < (int)CAM_PIXEL_VW; y += 2)
    {
      for (int x = 0; x < hwTwice; x += 2)
      {
        ycbcrBuffer_[y * hwTwice + x + 0] = 0;   // Y = 0 (黒)
        ycbcrBuffer_[y * hwTwice + x + 1] = 128; // Cb/Cr = 128 (ニュートラル)
      }
    }
  }
}

// ====================================================================
// 輝度抽出: YCbCr422 中間バッファから Y成分を抽出し imageBuffer_ へ
// YCbCr422フォーマット: [Y0][Cb][Y1][Cr] の繰り返し
// → 偶数バイト位置がY成分
// half: 0=前半(0-59行), 1=後半(60-119行)
// ====================================================================
void Camera::extractBrightness(int half)
{
  const int hwTwice = CAM_PIXEL_HW * 2;
  // フレーム開始時にキャプチャしたフィールド値を使用（参考プロジェクト準拠）
  const int frame = capturedField_;
  const int otherField = (frame == 0) ? 1 : 0;

  if (half == 0)
  {
    // 前半: 自フィールドのY成分を抽出 (0-59行)
    for (int y = frame; y < (int)(CAM_PIXEL_VW / 2); y += 2)
    {
      int px = 0;
      for (int x = 0; x < hwTwice; x += 2, px++)
      {
        imageBuffer_[y * CAM_PIXEL_HW + px] = ycbcrBuffer_[y * hwTwice + x];
      }
    }
    // 他フィールド行をバイリニア補間 (0-59行)
    for (int y = otherField; y < (int)(CAM_PIXEL_VW / 2); y += 2)
    {
      for (int x = 0; x < (int)CAM_PIXEL_HW; x++)
      {
        if (y <= 0)
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              imageBuffer_[(y + 1) * CAM_PIXEL_HW + x];
        }
        else if (y < (int)(CAM_PIXEL_VW / 2) - 1 && y > 0)
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              (unsigned char)(((int)imageBuffer_[(y - 1) * CAM_PIXEL_HW + x] +
                               (int)imageBuffer_[(y + 1) * CAM_PIXEL_HW + x]) /
                              2);
        }
        else
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              imageBuffer_[(y - 1) * CAM_PIXEL_HW + x];
        }
      }
    }
  }
  else
  {
    // 後半: 自フィールドのY成分を抽出 (60-119行)
    for (int y = (int)(CAM_PIXEL_VW / 2) + frame; y < (int)CAM_PIXEL_VW;
         y += 2)
    {
      int px = 0;
      for (int x = 0; x < hwTwice; x += 2, px++)
      {
        imageBuffer_[y * CAM_PIXEL_HW + px] = ycbcrBuffer_[y * hwTwice + x];
      }
    }
    // 他フィールド行をバイリニア補間 (60-119行)
    for (int y = (int)(CAM_PIXEL_VW / 2) + otherField; y < (int)CAM_PIXEL_VW;
         y += 2)
    {
      for (int x = 0; x < (int)CAM_PIXEL_HW; x++)
      {
        if (y <= 0)
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              imageBuffer_[(y + 1) * CAM_PIXEL_HW + x];
        }
        else if (y < (int)CAM_PIXEL_VW - 1 && y > 0)
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              (unsigned char)(((int)imageBuffer_[(y - 1) * CAM_PIXEL_HW + x] +
                               (int)imageBuffer_[(y + 1) * CAM_PIXEL_HW + x]) /
                              2);
        }
        else
        {
          imageBuffer_[y * CAM_PIXEL_HW + x] =
              imageBuffer_[(y - 1) * CAM_PIXEL_HW + x];
        }
      }
    }
  }
}

// ====================================================================
// getPixel: 指定座標のピクセル輝度値を取得
// ====================================================================
unsigned char Camera::getPixel(int x, int y) const
{
  if (x < 0 || x >= (int)CAM_PIXEL_HW || y < 0 || y >= (int)CAM_PIXEL_VW)
  {
    return 0;
  }
  return imageBuffer_[x + CAM_PIXEL_HW * y];
}

// ====================================================================
// getImageBuffer: 輝度バッファへの直接アクセス
// ====================================================================
const volatile unsigned char *Camera::getImageBuffer() const
{
  return imageBuffer_;
}

// ====================================================================
// isFrameReady / clearFrameReady は廃止 (sys.cam.frameReady を直接読み書き)。
// ====================================================================

// ====================================================================
// thresholdConvert: 閾値変換
// 参考プロジェクトの shikiichi_henkan と同等の処理
// 指定行の8点を取得し、閾値を自動調整、2進数8ビットに変換
// gyou: 行番号(0-119), threshold: 閾値, diff: 隣接差分閾値
// ====================================================================
unsigned char Camera::thresholdConvert(int gyou, int threshold,
                                       int diff) const
{
  int d[8];

  // センサ配置に対応する8点のX座標（参考プロジェクト準拠、単位は画素）
  // 画像中心 x=80 からのオフセットは
  //   左: -49, -37, -26, -9 / 右: +8, +25, +36, +48
  // でほぼ左右対称、外側ほど間隔が広い。
  // ビット割り当ては d[7] = 最左 (x=31) 〜 d[0] = 最右 (x=128)。
  // 実際の物理センサ位置との対応関係は不明（値は参考プロジェクトからの移植）。
  d[7] = getPixel(31, gyou);
  d[6] = getPixel(43, gyou);
  d[5] = getPixel(54, gyou);
  d[4] = getPixel(71, gyou);
  d[3] = getPixel(88, gyou);
  d[2] = getPixel(105, gyou);
  d[1] = getPixel(116, gyou);
  d[0] = getPixel(128, gyou);

  // 最大値・最小値を求める
  int minVal = d[0], maxVal = d[0];
  for (int i = 1; i < 8; i++)
  {
    if (maxVal <= d[i])
      maxVal = d[i];
    if (minVal >= d[i])
      minVal = d[i];
  }

  // 隣同士の差の絶対値
  int sa[7];
  for (int i = 0; i < 7; i++)
  {
    sa[i] = abs(d[i + 1] - d[i]);
  }

  // 閾値の自動調整
  int shikiVal;
  if (maxVal >= threshold)
  {
    // 最大値が閾値以上→閾値をそのまま使用
    shikiVal = threshold;
  }
  else
  {
    // 隣同士の差が1つでもdiff以上なら動的閾値
    bool hasDiff = false;
    for (int i = 0; i < 7; i++)
    {
      if (sa[i] >= diff)
      {
        hasDiff = true;
        break;
      }
    }
    if (hasDiff)
    {
      // (最大値 - 最小値) * 0.7 + 最小値
      // 係数 0.7 は参考プロジェクトの shikiichi_henkan 由来。整数演算のため
      // *7/10 と書いている。Config 化されていないので、変えるならここを直接
      // 編集する（LINE_DETECTOR_CONFIG 側の各種 brightnessRatio とは別物）。
      shikiVal = (maxVal - minVal) * 7 / 10 + minVal;
    }
    else
    {
      // すべて0にする（閾値を最大に）
      shikiVal = 256;
    }
  }

  // 8ビット変換: d[7]→bit7 ... d[0]→bit0
  unsigned char ret = 0;
  for (int i = 7; i >= 0; i--)
  {
    ret <<= 1;
    ret |= (d[i] >= shikiVal ? 1 : 0);
  }

  return ret;
}
