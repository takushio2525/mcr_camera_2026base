/*
 * Camera.cpp
 *
 *  NTSC Video Capture Driver Implementation
 *  VDC5 Channel 0 + DVDEC0 でNTSCアナログ映像を
 *  160x120 YCbCr422 でキャプチャし、輝度(Y)データを提供する
 */

#include "Camera.h"
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
    : frameStep_(0), fieldToggle_(1), fieldToggleBuf_(0), frameReady_(false) {
  memset((void *)imageBuffer_, 0, sizeof(imageBuffer_));
  memset(ycbcrBuffer_, 0, sizeof(ycbcrBuffer_));
}

// ====================================================================
// VDC5 + DVDEC 初期化
// ====================================================================
void Camera::init() {
  initVDC5();
  initDVDEC();

  // 映像信号安定待ち（約200ms）
  // ベアメタル環境なので簡易ウェイト
  for (volatile int i = 0; i < 6000000; i++) {
  }

  startCapture();

  // Vsync待ち
  s_vsyncCount = 1;
  while (s_vsyncCount > 0) {
  }

  // Vfield 2回待ち
  s_vfieldCount = 2;
  while (s_vfieldCount > 0) {
  }
}

// ====================================================================
// VDC5初期化（VideoチャネルCh0, スケーラ0を使用）
// NTSC 160x120 YCbCr422 キャプチャ設定
// mbed-gr-libs の DisplayBase 実装を参考にベアメタル化
// ====================================================================
void Camera::initVDC5() {
  // --- VDC5チャネル0のモジュールストップ解除 ---
  // STBCR9 bit1 = VDC50
  CPG.STBCR9 &= ~(1u << 1);
  volatile uint8_t dummy_rd = CPG.STBCR9;
  (void)dummy_rd;

  // --- パネルクロック設定 ---
  // NTSC入力の場合、外部同期信号を使用するため
  // パネルクロック（ICKSEL）は適当なソースを選択
  // P1φ(66.67MHz) を分周して使用: DCDR=2 → 33.33MHz
  // SYSCNT_PANEL_CLK: bit15-12=ICKSEL(0001=P1クロック), bit9-0=DCDR
  VDC50.SYSCNT_PANEL_CLK = (uint16_t)((0x01 << 12) | 0x0002);

  // 設定反映待ち
  for (volatile int i = 0; i < 1000; i++) {
  }

  // --- 入力セレクタ設定 ---
  // INP_SEL_CNT: ビデオデコーダ(VDEC)からの入力を選択
  // bit28-24: INP_SEL = 00000 (VideoDecoder Ch0 入力を選択)
  // bit16: INP_FORMAT = 0 (インターレース)
  // bit14-12: INP_PXD_EDGE = 000 (立ち上がりエッジ)
  VDC50.INP_SEL_CNT = 0x00000000;

  // --- 外部同期信号パラメータ ---
  // NTSC基準タイミング: 水平858サンプル, 垂直262/263ライン
  // INP_EXT_SYNC_CNT:
  //   bit31: INP_ENDIAN_ON = 0 (リトルエンディアン)
  //   bit28: INP_SWAP_ON = 0
  //   bit27: INP_VS_INV = 0 (Vsync 非反転)
  //   bit26: INP_HS_INV = 0 (Hsync 非反転)
  //   bit20-16: INP_FLD_DLY = 0
  //   bit15-0: INP_H_POS = 0
  VDC50.INP_EXT_SYNC_CNT = 0x00000000;

  // 垂直位相調整
  VDC50.INP_VSYNC_PH_ADJ = 0x00000000;

  // 入力遅延調整
  VDC50.INP_DLY_ADJ = 0x00000000;

  // 入力設定更新
  VDC50.INP_UPDATE = 0x00000001;

  // --- スケーラ0（SC0）設定: ビデオキャプチャの書き込み先設定 ---

  // SC0_SCL0_FRC1: 自由走行カウンタ（水平方向）
  // NTSC: 858サンプル
  VDC50.SC0_SCL0_FRC1 = 858u - 1u; // 水平カウント最大値

  // SC0_SCL0_FRC2: 垂直方向
  // NTSC: 525ライン（2フィールド）→ 1フィールド262/263
  VDC50.SC0_SCL0_FRC2 = 263u - 1u; // 垂直カウント最大値

  // SC0_SCL0_FRC3: 水平アクティブ表示開始位置
  VDC50.SC0_SCL0_FRC3 = (uint32_t)(0 << 16) | 0; // VS/HS 位置

  // SC0_SCL0_FRC4: フレームサイクル
  VDC50.SC0_SCL0_FRC4 = 0x00000000;

  // SC0_SCL0_FRC5: 表示エリア（水平開始/終了）
  VDC50.SC0_SCL0_FRC5 = 0x00000000;

  // SC0_SCL0_FRC6: 表示エリア（垂直開始/終了）
  VDC50.SC0_SCL0_FRC6 = 0x00000000;

  // SC0_SCL0_FRC7: フレーム制御
  VDC50.SC0_SCL0_FRC7 = 0x00000000;

  // 更新
  VDC50.SC0_SCL0_UPDATE = 0x00000100; // bit8: SC0_SCL0_VEN_A

  // --- スケーラ1: フレームバッファ書き込み設定 ---

  // SC0_SCL1_WR1: 書き込み制御
  //   bit25: SC0_RES_DS_WR_MD = 0
  //   bit20: SC0_RES_MD = 0 (ダウンスケーリングなし)
  //   bit15-0: 書き込みモード
  VDC50.SC0_SCL1_WR1 = 0x00000000;

  // SC0_SCL1_WR2: フレームバッファベースアドレス
  VDC50.SC0_SCL1_WR2 = (uint32_t)s_writeBuf;

  // SC0_SCL1_WR3: フレームバッファストライド
  //   ストライド = ((160*2) + 31) & ~31 = 352
  VDC50.SC0_SCL1_WR3 = CAM_VIDEO_BUFFER_STRIDE;

  // SC0_SCL1_WR4: フレームバッファ書き込みサイズ
  //   bit31-16: ライン数, bit15-0: 水平サイズ(バイト)
  VDC50.SC0_SCL1_WR4 =
      ((uint32_t)CAM_PIXEL_VW << 16) | (CAM_PIXEL_HW * CAM_DATA_SIZE_PER_PIC);

  // SC0_SCL1_WR5: フレームバッファ書き込み開始
  //   bit0: SC0_FLM_NUM = 0 (単一フレーム)
  VDC50.SC0_SCL1_WR5 = 0x00000000;

  // SC0_SCL1_WR6: YCbCr変換設定
  //   bit31-28: WR色フォーマット = 0001 (YCbCr422)
  //   bit27-24: バスポート幅 = 0000 (32bit)
  //   bit8: BSTバースト転送 = 0
  //   bit5-4: 書き込みスワップ = 10 (32_16BIT)
  VDC50.SC0_SCL1_WR6 = (0x01 << 28) | (0x02 << 4);

  // SC0_SCL1_WR7: フレームバッファ書き込みサイズ（フィールド）
  VDC50.SC0_SCL1_WR7 = 0x00000000;

  // SC0_SCL1_WR8: 書き込みバッファリング制御
  VDC50.SC0_SCL1_WR8 = 0x00000000;

  // SC0_SCL1_WR9: 書き込み制御2
  VDC50.SC0_SCL1_WR9 = 0x00000001; // 書き込みイネーブル

  // SC0_SCL1_WR10: 書き込み制御3
  VDC50.SC0_SCL1_WR10 = 0x00000000;

  // 更新
  VDC50.SC0_SCL1_UPDATE = 0x00000110; // 書き込み設定更新

  // --- VField割り込み設定 ---
  // SYSCNT_INT1: S0_VI_VSYNC割り込みクリア・イネーブル
  // SYSCNT_INT2: S0_VFIELD割り込みクリア・イネーブル
  VDC50.SYSCNT_INT1 = 0x00000000;
  VDC50.SYSCNT_INT2 = 0x00000000;

  // S0_VI_VSYNC割り込みイネーブル (bit0)
  VDC50.SYSCNT_INT4 |= (1u << 0);

  // S0_VFIELD割り込みイネーブル (bit1)
  VDC50.SYSCNT_INT4 |= (1u << 1);
}

// ====================================================================
// DVDEC初期化（NTSC 3.58MHz カラー信号デコード）
// ====================================================================
void Camera::initDVDEC() {
  // DVDEC0（チャネル0）のモジュールストップ解除は CPG.STBCR9 のVDC5に含まれる

  // --- ビデオデコーダ ADC設定 ---
  // ADCCR1: ADC入力チャネル選択
  DVDEC0.ADCCR1 = 0x0000; // デフォルト設定（入力Ch0)

  // --- 同期信号検出設定 ---
  // SYNSCR1〜5: 同期検出パラメータ（NTSC用デフォルト）
  DVDEC0.SYNSCR1 = 0x0000; // Hカウンタリセット位置
  DVDEC0.SYNSCR2 = 0x0000; // 同期スライスレベル
  DVDEC0.SYNSCR3 = 0x0000;
  DVDEC0.SYNSCR4 = 0x0000;
  DVDEC0.SYNSCR5 = 0x0000;

  // --- タイミングジェネレータ ---
  // TGCR1〜3: NTSC 3.58MHz 基準
  DVDEC0.TGCR1 = 0x0000;
  DVDEC0.TGCR2 = 0x0000;
  DVDEC0.TGCR3 = 0x0000;

  // --- AGC (自動ゲイン制御) ---
  DVDEC0.AGCCR1 = 0x0000;
  DVDEC0.AGCCR2 = 0x0000;

  // --- カラー信号処理 ---
  // ACCCR1〜3: ACC（自動色制御）
  DVDEC0.ACCCR1 = 0x0000;
  DVDEC0.ACCCR2 = 0x0000;
  DVDEC0.ACCCR3 = 0x0000;

  // TINTCR: 色合い調整（デフォルト = 0）
  DVDEC0.TINTCR = 0x0000;

  // YCDCR: Y/C分離設定
  DVDEC0.YCDCR = 0x0000;

  // 更新レジスタ
  DVDEC0.RUPDCR = 0x0001;
}

// ====================================================================
// ビデオキャプチャ開始
// ====================================================================
void Camera::startCapture() {
  // SC0_SCL1_WR9: bit0 = 書き込みイネーブル
  VDC50.SC0_SCL1_WR9 |= 0x00000001;
  VDC50.SC0_SCL1_UPDATE = 0x00000110;
}

// ====================================================================
// ビデオキャプチャ停止
// ====================================================================
void Camera::stopCapture() {
  VDC50.SC0_SCL1_WR9 &= ~0x00000001u;
  VDC50.SC0_SCL1_UPDATE = 0x00000110;
}

// ====================================================================
// フレームバッファ切替（ダブルバッファ方式）
// ====================================================================
void Camera::changeFrameBuffer() {
  if (s_writeBuf == s_frameBufA) {
    s_writeBuf = s_frameBufB;
    s_saveBuf = s_frameBufA;
  } else {
    s_writeBuf = s_frameBufA;
    s_saveBuf = s_frameBufB;
  }

  // 新しいバッファアドレスをVDC5に設定
  VDC50.SC0_SCL1_WR2 = (uint32_t)s_writeBuf;
  VDC50.SC0_SCL1_UPDATE = 0x00000110;
}

// ====================================================================
// Vfieldコールバック（VDC5割り込みから呼ばれる）
// ====================================================================
void Camera::vfieldCallback() {
  if (s_vfieldCount > 0) {
    s_vfieldCount--;
  }
  // トップ/ボトムフィールドのトグル
  s_vfieldToggle = (s_vfieldToggle == 0) ? 1 : 0;
}

// ====================================================================
// Vsyncコールバック
// ====================================================================
void Camera::vsyncCallback() {
  if (s_vsyncCount > 0) {
    s_vsyncCount--;
  }
}

// ====================================================================
// update(): フレーム周期ステップ処理
// 1ms割り込みから毎回呼ばれ、ステップごとに画像処理を分割実行
// 参考プロジェクトの intTimer() 内 switch(counter++) と同等
// ====================================================================
void Camera::update() {
  // フィールド切替検出: 新しいフレームが来たらステップをリセット
  if (s_vfieldToggle != fieldToggleBuf_) {
    fieldToggleBuf_ = s_vfieldToggle;
    fieldToggle_ = s_vfieldToggle;
    frameStep_ = 0;
  }

  switch (frameStep_++) {
  case 0:
    // ステップ0: YCbCr422 生データのコピー（前半60行）
    imageCopy(0);
    break;

  case 1:
    // ステップ1: YCbCr422 生データのコピー（後半60行）
    imageCopy(1);
    break;

  case 2:
    // ステップ2: 輝度抽出（前半60行）
    extractBrightness(0);
    break;

  case 3:
    // ステップ3: 輝度抽出（後半60行）→ フレーム完了
    extractBrightness(1);
    frameReady_ = true;
    break;

  default:
    // フレーム処理完了後は何もしない（次のVfieldリセットを待つ）
    break;
  }
}

// ====================================================================
// ImageCopy: VDC5フレームバッファから中間バッファへコピー
// インターレース対応：トップ/ボトムフィールドを交互に取得
// half: 0=前半(0-59行), 1=後半(60-119行)
// ====================================================================
void Camera::imageCopy(int half) {
  const int hwTwice = CAM_PIXEL_HW * 2; // YCbCr422は2バイト/ピクセル
  const int startY = (half == 0) ? fieldToggle_ : CAM_PIXEL_VW / 2;
  const int endY = (half == 0) ? (int)(CAM_PIXEL_VW / 2) : (int)CAM_PIXEL_VW;
  const volatile uint8_t *src = s_saveBuf;

  // フィールドライン（2行飛ばし）でコピー
  for (int y = startY; y < endY; y += 2) {
    for (int x = 0; x < hwTwice; x++) {
      ycbcrBuffer_[y * hwTwice + x] = src[y * hwTwice + x];
    }
  }

  // 後半コピー時：もう一方のフィールドラインを補間（黒で埋め）
  if (half == 1) {
    int otherField = (fieldToggle_ == 0) ? 1 : 0;
    for (int y = otherField; y < (int)CAM_PIXEL_VW; y += 2) {
      for (int x = 0; x < hwTwice; x += 2) {
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
void Camera::extractBrightness(int half) {
  const int hwTwice = CAM_PIXEL_HW * 2;
  const int startField = fieldToggle_;
  const int startY =
      (half == 0) ? startField : (int)(CAM_PIXEL_VW / 2) + startField;
  const int endY = (half == 0) ? (int)(CAM_PIXEL_VW / 2) : (int)CAM_PIXEL_VW;

  // 自フィールドのY成分を抽出
  for (int y = startY; y < endY; y += 2) {
    int px = 0;
    for (int x = 0; x < hwTwice; x += 2) {
      imageBuffer_[y * CAM_PIXEL_HW + px] = ycbcrBuffer_[y * hwTwice + x];
      px++;
    }
  }

  // 他フィールド行をバイリニア補間で埋める
  int otherField = (startField == 0) ? 1 : 0;
  int startYOther =
      (half == 0) ? otherField : (int)(CAM_PIXEL_VW / 2) + otherField;
  for (int y = startYOther; y < endY; y += 2) {
    for (int x = 0; x < (int)CAM_PIXEL_HW; x++) {
      if (y <= 0) {
        imageBuffer_[y * CAM_PIXEL_HW + x] =
            imageBuffer_[(y + 1) * CAM_PIXEL_HW + x];
      } else if (y < endY - 1 && y > 0) {
        imageBuffer_[y * CAM_PIXEL_HW + x] =
            (unsigned char)(((int)imageBuffer_[(y - 1) * CAM_PIXEL_HW + x] +
                             (int)imageBuffer_[(y + 1) * CAM_PIXEL_HW + x]) /
                            2);
      } else {
        imageBuffer_[y * CAM_PIXEL_HW + x] =
            imageBuffer_[(y - 1) * CAM_PIXEL_HW + x];
      }
    }
  }
}

// ====================================================================
// getPixel: 指定座標のピクセル輝度値を取得
// ====================================================================
unsigned char Camera::getPixel(int x, int y) const {
  if (x < 0 || x >= (int)CAM_PIXEL_HW || y < 0 || y >= (int)CAM_PIXEL_VW) {
    return 0;
  }
  return imageBuffer_[x + CAM_PIXEL_HW * y];
}

// ====================================================================
// getImageBuffer: 輝度バッファへの直接アクセス
// ====================================================================
const volatile unsigned char *Camera::getImageBuffer() const {
  return imageBuffer_;
}

// ====================================================================
// isFrameReady: 新しいフレームが準備できたかチェック
// ====================================================================
bool Camera::isFrameReady() const { return frameReady_; }

// ====================================================================
// thresholdConvert: 閾値変換
// 参考プロジェクトの shikiichi_henkan と同等の処理
// 指定行の8点を取得し、閾値を自動調整、2進数8ビットに変換
// gyou: 行番号(0-119), threshold: 閾値, diff: 隣接差分閾値
// ====================================================================
unsigned char Camera::thresholdConvert(int gyou, int threshold,
                                       int diff) const {
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
  for (int i = 1; i < 8; i++) {
    if (maxVal <= d[i])
      maxVal = d[i];
    if (minVal >= d[i])
      minVal = d[i];
  }

  // 隣同士の差の絶対値
  int sa[7];
  for (int i = 0; i < 7; i++) {
    sa[i] = abs(d[i + 1] - d[i]);
  }

  // 閾値の自動調整
  int shikiVal;
  if (maxVal >= threshold) {
    // 最大値が閾値以上→閾値をそのまま使用
    shikiVal = threshold;
  } else {
    // 隣同士の差が1つでもdiff以上なら動的閾値
    bool hasDiff = false;
    for (int i = 0; i < 7; i++) {
      if (sa[i] >= diff) {
        hasDiff = true;
        break;
      }
    }
    if (hasDiff) {
      // (最大値 - 最小値) * 0.7 + 最小値
      shikiVal = (maxVal - minVal) * 7 / 10 + minVal;
    } else {
      // すべて0にする（閾値を最大に）
      shikiVal = 256;
    }
  }

  // 8ビット変換: d[7]→bit7 ... d[0]→bit0
  unsigned char ret = 0;
  for (int i = 7; i >= 0; i--) {
    ret <<= 1;
    ret |= (d[i] >= shikiVal ? 1 : 0);
  }

  return ret;
}
