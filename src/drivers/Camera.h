/*
 * Camera.h
 *
 *  NTSC Video Capture Driver for GR-PEACH (RZ/A1H)
 *  VDC5 Channel 0 + DVDEC0 によるNTSCキャプチャ
 *  160x120 YCbCr422 → 輝度(Y)抽出
 */

#ifndef DRIVERS_CAMERA_H_
#define DRIVERS_CAMERA_H_

#include "../core/IModule.h"
#include "video/DisplayBace.h"
#include <stdint.h>

// 画像サイズ定義
#define CAM_PIXEL_HW 160u // 水平ピクセル数 (QVGA)
#define CAM_PIXEL_VW 120u // 垂直ピクセル数 (QVGA)

// YCbCr422 = 2バイト/ピクセル
#define CAM_DATA_SIZE_PER_PIC 2u

// フレームバッファストライド（32バイト境界にアライン）
#define CAM_VIDEO_BUFFER_STRIDE \
  (((CAM_PIXEL_HW * CAM_DATA_SIZE_PER_PIC) + 31u) & ~31u)

class Camera : public IModule
{
public:
  // コンストラクタ
  Camera();

  // VDC5/DVDEC初期化、NTSCビデオキャプチャ開始
  bool init() override;

  // フレーム周期ステップ処理（1ms割り込みから呼ばれる）
  // ImageCopy → 輝度抽出 を段階的に処理
  // ※Step 4 段階では機械置換のみ。Step 9 で updateInput に正しく振り分ける。
  void updateOutput(SystemData& sys) override;

  // 指定座標(x, y)のピクセル輝度値を取得 (0-255)
  // x: 0-159, y: 0-119
  unsigned char getPixel(int x, int y) const;

  // 輝度バッファへの直接アクセス（読み取り専用）
  const volatile unsigned char *getImageBuffer() const;

  // 閾値変換（参考プロジェクトの shikiichi_henkan 相当）
  // 指定行の8点を取得し、閾値自動調整で2進数8ビットに変換
  // gyou: 行番号(0-119), threshold: 閾値, diff: 差分閾値
  unsigned char thresholdConvert(int gyou, int threshold, int diff) const;

  // 新フレーム準備完了フラグ
  bool isFrameReady() const;

  // フレーム準備完了フラグをクリア（読み出し後に呼ぶ）
  void clearFrameReady();

  // Vfield/Vsyncコールバック（割り込みから呼ばれる関数）
  static void vfieldCallback(DisplayBase::int_type_t int_type);
  static void vsyncCallback(DisplayBase::int_type_t int_type);

private:
  // ビデオキャプチャ開始
  void startCapture();

  // ビデオキャプチャ停止
  void stopCapture();

  // フレームバッファ切替
  void changeFrameBuffer();

  // ImageCopy: VDC5のYCbCr422生データをコピー
  // half: 0=前半(0-59行), 1=後半(60-119行)
  void imageCopy(int half);

  // 輝度抽出: YCbCr422からY成分のみを抽出
  // half: 0=前半(0-59行), 1=後半(60-119行)
  void extractBrightness(int half);

  // DisplayBaseのインスタンス
  DisplayBase display_;

  // フレーム処理ステップカウンタ
  int frameStep_;

  // トップ/ボトムフィールドトグル (0=トップ, 1=ボトム)
  volatile int fieldToggle_;

  // フレーム処理開始時にキャプチャしたフィールド値
  // 4ステップ全体で同じ値を使用するため
  int capturedField_;

  // 新フレーム準備完了フラグ
  volatile bool frameReady_;

  // 輝度データバッファ（最終出力：160x120 の1次元配列）
  volatile unsigned char imageBuffer_[CAM_PIXEL_HW * CAM_PIXEL_VW];

  // YCbCr422中間バッファ（ImageCopy用: 160*2*120）
  unsigned char ycbcrBuffer_[CAM_PIXEL_HW * 2 * CAM_PIXEL_VW];
};

// グローバルインスタンス（extern宣言）
extern Camera g_camera;

#endif /* DRIVERS_CAMERA_H_ */
