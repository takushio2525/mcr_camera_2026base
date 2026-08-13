/*
 * CameraData.h
 *
 *  Camera モジュールの Config / Data 宣言と画像サイズ定義。
 *
 *  Camera.h から分離してある理由:
 *    Camera.h は VDC5 ドライバ (video/DisplayBace.h → pinmap.h) を include し、
 *    その先で実機のレジスタ定義 (iodefine.h の INTC) を要求する。
 *    一方 SystemData.h が Camera から必要としているのは CameraData 型だけで、
 *    VDC5 の API は一切要らない。にもかかわらず SystemData.h が Camera.h を
 *    include していたため、SystemData.h を使う全モジュール（Motor / Servo /
 *    Encoder 等）が VDC5 ドライバ一式に依存していた。
 *    ホスト側ユニットテスト (tests/) がビルド不能になっていた直接の原因がこれ。
 *
 *  したがって:
 *    - Config / Data / サイズ定義  → 本ファイル（ハードウェア非依存）
 *    - Camera クラス (DisplayBase を持つ) → Camera.h
 *  EMA の「Data の所有権はモジュール側にある」というルールは維持している
 *  （Camera.h が本ファイルを include するので、Camera.h を include する側から
 *   見える宣言の集合は分離前と完全に同じ）。
 */

#ifndef DRIVERS_CAMERA_DATA_H_
#define DRIVERS_CAMERA_DATA_H_

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

#endif /* DRIVERS_CAMERA_DATA_H_ */
