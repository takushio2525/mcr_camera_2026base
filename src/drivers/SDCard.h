/*
 * SDCard.h
 *
 *  SDカード SPI ドライバ (RSPI2)
 *  For GR-PEACH (RZ/A1H)
 *
 *  ピン割り当て:
 *    MOSI : P8_5  (RSPI2 MOSI, Function 3)
 *    MISO : P8_6  (RSPI2 MISO, Function 3)
 *    CLK  : P8_3  (RSPI2 RSPCK, Function 3)
 *    CS   : P8_4  (GPIO, 手動制御)
 *    CD   : P7_8  (GPIO入力, カード検出)
 *
 *  SDカードの初期化シーケンス:
 *    CMD0 → CMD8 → ACMD41 → CMD58 → CMD16
 *
 *  ブロック単位 (512バイト) の読み書きを提供する。
 *  FatFs の diskio 層から呼ばれる。
 */

#ifndef DRIVERS_SDCARD_H_
#define DRIVERS_SDCARD_H_

#include "../core/IModule.h"
#include <stdint.h>

// ====================================================================
// SDCardConfig — SDカード SPI のピン設定
// (実体は ProjectConfig.h の SDCARD_CONFIG で定義)
// ====================================================================
struct SDCardConfig
{
  int csPinFmt;       // P8_4 (CS, GPIO 手動制御)
  int cdPinFmt;       // P7_8 (CD, カード検出)
  int mosiPinFmt;     // P8_5
  int misoPinFmt;     // P8_6
  int sckPinFmt;      // P8_3
};

class SDCard : public IModule
{
public:
  // カードタイプ
  enum CardType {
    CARD_NONE = 0,
    CARD_V1,     // SD v1.x (標準容量)
    CARD_V2_SC,  // SD v2.x 標準容量
    CARD_V2_HC   // SDHC/SDXC (高容量)
  };

  // Config を注入してインスタンス生成
  explicit SDCard(const SDCardConfig& cfg);

  // RSPI2 初期化 + SDカード初期化
  bool init() override;

  // 周期処理（未使用）
  void updateOutput(SystemData& sys) override;

  // 初期化成功したか
  bool isReady() const { return initialized_; }

  // カードタイプを取得
  CardType getCardType() const { return cardType_; }

  // ブロック読み取り (512バイト × count)
  // block: ブロック番号（セクタ番号）
  // 戻り値: 0=成功, 非0=エラー
  int readBlocks(uint8_t *buf, uint32_t block, uint32_t count);

  // ブロック書き込み (512バイト × count)
  int writeBlocks(const uint8_t *buf, uint32_t block, uint32_t count);

  // セクタ数を取得
  uint32_t getSectorCount() const { return sectorCount_; }

private:
  SDCardConfig _config;
  bool initialized_;
  CardType cardType_;
  uint32_t sectorCount_;

  // RSPI2 ハードウェア初期化
  void initSPI();

  // SPI クロック速度設定
  void setSPIClock(uint32_t hz);

  // SPI 1バイト送受信
  uint8_t spiTransfer(uint8_t data);

  // CS 制御
  void csLow();
  void csHigh();

  // カード検出ピン読み取り (Low = カードあり)
  bool isCardInserted();

  // SDカード初期化シーケンス
  int initCard();

  // SDコマンド送信
  // 戻り値: R1レスポンス (-1 = タイムアウト)
  int sendCommand(uint8_t cmd, uint32_t arg);

  // アプリケーションコマンド送信 (CMD55 + ACMDxx)
  int sendAppCommand(uint8_t cmd, uint32_t arg);

  // CMD8 (電圧チェック)
  int sendCMD8();

  // CMD58 (OCR読み取り)
  int sendCMD58(uint32_t *ocr);

  // データブロック受信 (512バイト)
  int readData(uint8_t *buf, uint32_t len);

  // データブロック送信 (512バイト)
  int writeData(const uint8_t *buf, uint8_t token);

  // ビジーウェイト
  void waitReady();

  // CSD からセクタ数を計算
  uint32_t readSectorCount();

  // V1/V2 カード初期化
  int initCardV1();
  int initCardV2();

  // コマンドタイムアウト
  static const int CMD_TIMEOUT = 5000;
};

extern SDCard g_sdcard;

#endif /* DRIVERS_SDCARD_H_ */
