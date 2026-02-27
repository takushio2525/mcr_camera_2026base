/*
 * Camera.cpp
 *
 *  NTSC Video Capture Driver Implementation
 *  VDC5 Channel 0 + DVDEC0 でNTSCアナログ映像を
 *  160x120 YCbCr422 でキャプチャし、輝度(Y)データを提供する
 */

#include "Camera.h"
#include "Serial.h"
#include "iodefine.h"
#include <stdlib.h>
#include <string.h>

// ====================================================================
// VDC5ビデオキャプチャ用フレームバッファ（NonCacheable領域に配置）
// ダブルバッファ方式: VDC5が書き込む先とソフトウェアが読む先を分離
// ====================================================================
static uint8_t s_frameBufA[CAM_VIDEO_BUFFER_STRIDE * CAM_PIXEL_VW]
    __attribute__((section("NC_BSS"), aligned(32)));
static uint8_t s_frameBufB[CAM_VIDEO_BUFFER_STRIDE * CAM_PIXEL_VW]
    __attribute__((section("NC_BSS"), aligned(32)));

// 現在VDC5が書き込み中のバッファと、ソフトウェアが読み出すバッファ
static volatile uint8_t *s_writeBuf = s_frameBufA;
static volatile uint8_t *s_saveBuf = s_frameBufB;

// Vsync/Vfieldカウンタ
static volatile int32_t s_vsyncCount = 0;
static volatile int32_t s_vfieldCount = 0;
static volatile int32_t s_vfieldToggle = 1;

// グローバルインスタンス
Camera g_camera;

// ====================================================================
// コンストラクタ
// ====================================================================
Camera::Camera()
    : frameStep_(0), fieldToggle_(1), fieldToggleBuf_(0), frameReady_(false)
{
  memset((void *)imageBuffer_, 0, sizeof(imageBuffer_));
  memset(ycbcrBuffer_, 0, sizeof(ycbcrBuffer_));
}

// ====================================================================
// VDC5 + DVDEC 初期化
// ====================================================================
void Camera::init()
{
  DisplayBase::graphics_error_t error;

  // グローバルコンストラクタが呼ばれない環境への対策として、ここで明示的に初期化する
  display_ = DisplayBase();
  frameStep_ = 0;
  fieldToggle_ = 1;
  fieldToggleBuf_ = 0;
  frameReady_ = false;
  memset((void *)imageBuffer_, 0, sizeof(imageBuffer_));
  memset(ycbcrBuffer_, 0, sizeof(ycbcrBuffer_));

  g_serial.printf("[Camera::init] Start Graphics_init...\n");
  // 1. Graphics initialization process
  error = display_.Graphics_init(NULL);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf("[Camera::init] ERROR at Graphics_init: %d\n", error);
    while (1)
      ;
  }
  g_serial.printf("[Camera::init] Graphics_init OK.\n");

  g_serial.printf("[Camera::init] Start Graphics_Video_init...\n");
  // 2. Video decoder initialization
  error = display_.Graphics_Video_init(DisplayBase::INPUT_SEL_VDEC, NULL);
  if (error != DisplayBase::GRAPHICS_OK)
  {
    g_serial.printf("[Camera::init] ERROR at Graphics_Video_init: %d\n", error);
    while (1)
      ;
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
    while (1)
      ;
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
    while (1)
      ;
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
    while (1)
      ;
  }
  g_serial.printf("[Camera::init] Graphics_Irq_Handler_Set (VFIELD) OK.\n");

  g_serial.printf("[Camera::init] Waiting for video signal to stabilize...\n");
  // 映像信号安定待ち（約200ms）
  for (volatile int i = 0; i < 6000000; i++)
  {
  }
  g_serial.printf("[Camera::init] Wait done.\n");

  g_serial.printf(
      "[Camera::init] Capture Start -> Stop -> Start sequence...\n");
  // 6. Capture Start -> Stop -> Start (参考プロジェクトと同じ初期化シーケンス)
  display_.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);
  display_.Video_Stop(DisplayBase::VIDEO_INPUT_CHANNEL_0);
  display_.Video_Start(DisplayBase::VIDEO_INPUT_CHANNEL_0);
  g_serial.printf("[Camera::init] Sequence done.\n");

  // 参考プロジェクトと同じ: Vsync/Vfield待ちでVDC5の安定を確認
  // WaitVsync(1) 相当: Vsyncが1回発生するまで待つ
  s_vsyncCount = 1;
  for (volatile int timeout = 0; s_vsyncCount > 0 && timeout < 5000000;
       timeout++)
  {
  }
  g_serial.printf("[Camera::init] WaitVsync done.\n");

  // WaitVfield(2) 相当: Vfieldが2回発生するまで待つ
  s_vfieldCount = 2;
  for (volatile int timeout = 0; s_vfieldCount > 0 && timeout < 10000000;
       timeout++)
  {
  }
  g_serial.printf(
      "[Camera::init] All initialization completed successfully.\n");
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
  if (s_vfieldCount > 0)
  {
    s_vfieldCount--;
  }
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
// メインループから毎回呼ばれ、ステップごとに画像処理を分割実行
// 参考プロジェクト(2.38m-s)の intTimer() 内 switch(counter++) と同等
//
// 【XIP環境対応】
// mbed-os(RAM実行+L1キャッシュ)では各ステップが1ms以内に完了するため、
// Vfield(16.7ms間隔)ごとに余裕をもって4ステップを終えられる。
// しかしXIP(SPIフラッシュ直接実行)ではimageCopyが重く、4ステップの
// 合計が16.7msを超えることがある。
// 従来のロジックではVfieldトグル変化で即座にframeStep_をリセットしていたため、
// ステップ途中でリセットされフレーム処理が永遠に完了しない問題があった。
//
// 修正: フレーム処理中(frameStep_ 0〜3)はVfieldリセットを行わず、
// 処理完了後(frameStep_ >= 4)に次のVfield変化を待って新フレームを開始する。
// ====================================================================
void Camera::update()
{
  if (frameStep_ <= 3)
  {
    // フレーム処理中: Vfield変化を無視して現在のフレームを最後まで処理する
    // XIP環境では各ステップが重いため、途中リセットを防止する
  }
  else
  {
    // フレーム処理完了後: 次のVfieldトグル変化を待って新フレームを開始
    if ((int)s_vfieldToggle != fieldToggleBuf_)
    {
      fieldToggleBuf_ = (int)s_vfieldToggle;
      frameStep_ = 0;
      frameReady_ = false; // 新フレーム処理開始時にクリア
    }
    else
    {
      // まだVfieldが来ていない → 何もしない
      return;
    }
  }

  switch (frameStep_++)
  {
  case 0:
    // ステップ0: YCbCr422 生データのコピー（前半0-59行）
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
    break;

    // case 4〜: 追加の処理があればここに記述可能
    // 例: エンコーダ更新、偏差計算、モーター制御値計算 etc.

  default:
    // 上記以外のカウンタ値では何もしない（次フィールドを待つ）
    break;
  }
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
  // VDC5 Vfield割り込みで取得した実際のフィールド値を使用
  const int frame = (int)s_vfieldToggle;

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
  // VDC5 Vfield割り込みの実フィールド値を使用（参考プロジェクト準拠）
  const int frame = (int)s_vfieldToggle;
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
// isFrameReady: 新しいフレームが準備できたかチェック
// ====================================================================
bool Camera::isFrameReady() const { return frameReady_; }

// ====================================================================
// clearFrameReady: フレーム準備完了フラグをクリア
// メインループでフレーム読み出し後に呼んでフラグをリセットする
// ====================================================================
void Camera::clearFrameReady() { frameReady_ = false; }

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

  // センサ配置に対応する8点のX座標（参考プロジェクト準拠）
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
