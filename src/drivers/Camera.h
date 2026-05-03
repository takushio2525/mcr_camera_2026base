/*
 * Camera.h
 *
 *  NTSC Video Capture Driver for GR-PEACH (RZ/A1H)
 *  EMA 準拠版: SystemData / Config ベース
 *  VDC5 Channel 0 + DVDEC0 によるNTSCキャプチャ
 *  160x120 YCbCr422 → 輝度(Y)抽出
 *
 *  Input : updateInput(sys) で 1ms ごとにフレーム段階処理を進め、
 *          フレーム完了時に sys.cam.frameReady=true / frameCount++ /
 *          imageBufferPtr を更新する。
 *          メインループ側で sys.cam.frameReady を false に戻すと
 *          内部状態もリセットされる。
 *
 *  注意: バス制約（VDC5 + DVDEC + Vfield 待ち）が大きいため、
 *        画素バッファ実体は Camera 内部に保持し、SystemData からは
 *        imageBufferPtr 経由で参照させる方針。
 */

#ifndef DRIVERS_CAMERA_H_
#define DRIVERS_CAMERA_H_

#include "../core/IModule.h"
#include "video/DisplayBace.h"
#include <stdint.h>

// ====================================================================
// CameraConfig — Camera のハードウェア / 動作設定
// (実体は ProjectConfig.h の CAMERA_CONFIG で定義)
// ====================================================================
struct CameraConfig
{
  int pixelWidth;        // 160
  int pixelHeight;       // 120
  int thresholdDefault;  // 8点センサ取得時のデフォルト閾値 (170)
  int thresholdRow;      // 8点センサ取得行 (60) — LineDetector 側でも持つ
  int thresholdDiff;     // 8点センサ閾値の差分 (8)  — LineDetector 側でも持つ
};

// ====================================================================
// CameraData — Camera の出力データ
// SystemData::cam に集約される。Camera クラスが「所有」する自身のデータ。
// ====================================================================
struct CameraData
{
  bool     frameReady;     // 新フレーム完了フラグ（メインループで消費後に false へ）
  uint32_t frameCount;     // 累積フレーム数（debug 表示用）
  int      field;          // 0=top / 1=bottom (debug 用)
  // 画素バッファ実体は Camera::imageBuffer_ にあり、ここはそれを指すポインタ。
  // 19KB のバッファを SystemData に持たせるとメモリ効率が悪いため。
  const volatile unsigned char* imageBufferPtr;
};

// 画像サイズ定義 (CameraConfig との整合のためマクロも残す)
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
  // Config を注入してインスタンス生成
  explicit Camera(const CameraConfig& cfg);

  // VDC5/DVDEC初期化、NTSCビデオキャプチャ開始
  bool init() override;

  // Input : 1ms ごとにフレーム段階処理を進め、完了時に sys.cam.* を更新。
  //         sys.cam.frameReady が false に戻されたら内部状態もリセット。
  void updateInput(SystemData& sys) override;

  // 指定座標(x, y)のピクセル輝度値を取得 (0-255)
  // x: 0-159, y: 0-119
  // バス制約のため LineDetector / runDebugLoop からは直接呼ぶ。
  unsigned char getPixel(int x, int y) const;

  // 輝度バッファへの直接アクセス（読み取り専用）
  const volatile unsigned char *getImageBuffer() const;

  // 閾値変換（参考プロジェクトの shikiichi_henkan 相当）
  // 指定行の8点を取得し、閾値自動調整で2進数8ビットに変換
  // gyou: 行番号(0-119), threshold: 閾値, diff: 差分閾値
  unsigned char thresholdConvert(int gyou, int threshold, int diff) const;

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

  CameraConfig _config;

  // DisplayBaseのインスタンス
  DisplayBase display_;

  // フレーム処理ステップカウンタ
  int frameStep_;

  // トップ/ボトムフィールドトグル (0=トップ, 1=ボトム)
  volatile int fieldToggle_;

  // フレーム処理開始時にキャプチャしたフィールド値
  // 4ステップ全体で同じ値を使用するため
  int capturedField_;

  // 新フレーム準備完了フラグ（内部）
  volatile bool frameReady_;

  // フレームカウンタ（debug 表示用）
  uint32_t frameCount_;

  // 輝度データバッファ（最終出力：160x120 の1次元配列）
  volatile unsigned char imageBuffer_[CAM_PIXEL_HW * CAM_PIXEL_VW];

  // YCbCr422中間バッファ（ImageCopy用: 160*2*120）
  unsigned char ycbcrBuffer_[CAM_PIXEL_HW * 2 * CAM_PIXEL_VW];
};

// グローバルインスタンス（実体は main 側で定義）
extern Camera g_camera;

#endif /* DRIVERS_CAMERA_H_ */
