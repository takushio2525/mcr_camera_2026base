/*
 * SDCard.cpp
 *
 *  SDカード SPI ドライバ (RSPI2)
 *  For GR-PEACH (RZ/A1H)
 *
 *  参考: mbed SDBlockDevice + SD Physical Layer Simplified Spec
 *
 *  RSPI2 ピン:
 *    P8_5 = MOSI (Function 3: PFCAE=0, PFCE=1, PFC=0)
 *    P8_6 = MISO (Function 3: PFCAE=0, PFCE=1, PFC=0)
 *    P8_3 = RSPCK (Function 3: PFCAE=0, PFCE=1, PFC=0)
 *    P8_4 = CS (GPIO出力, 手動制御)
 *    P7_8 = CD (GPIO入力, カード検出)
 */

#include "SDCard.h"
#include "iodefine.h"
#include "Serial.h"

// R1 レスポンスビット
#define R1_IDLE_STATE       (1 << 0)
#define R1_ERASE_RESET      (1 << 1)
#define R1_ILLEGAL_COMMAND  (1 << 2)
#define R1_COM_CRC_ERROR    (1 << 3)
#define R1_ERASE_SEQ_ERROR  (1 << 4)
#define R1_ADDRESS_ERROR    (1 << 5)
#define R1_PARAMETER_ERROR  (1 << 6)

// データトークン
#define DATA_TOKEN_CMD17    0xFE
#define DATA_TOKEN_CMD24    0xFE

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

SDCard::SDCard(const SDCardConfig& cfg)
  : _config(cfg)
  , initialized_(false)
  , cardType_(CARD_NONE)
  , sectorCount_(0)
{
}

bool SDCard::init()
{
  g_serial.printf("SDCard: 初期化開始...\n");

  // RSPI2 ハードウェア初期化
  initSPI();

  // カード検出確認
  if (!isCardInserted()) {
    g_serial.printf("SDCard: カード未挿入\n");
    return false;
  }
  g_serial.printf("SDCard: カード検出OK\n");

  // SDカード初期化
  int ret = initCard();
  if (ret != 0) {
    g_serial.printf("SDCard: 初期化失敗 (err=%d)\n", ret);
    return false;
  }

  // セクタ数を取得
  sectorCount_ = readSectorCount();
  g_serial.printf("SDCard: セクタ数=%lu (%luMB)\n",
                  sectorCount_, sectorCount_ / 2048);

  initialized_ = true;
  g_serial.printf("SDCard: 初期化完了 (タイプ=%d)\n", (int)cardType_);
  return true;
}

void SDCard::updateOutput(SystemData& sys)
{
  (void)sys;
  // 未使用
}

// ---- RSPI2 ハードウェア初期化 ----
void SDCard::initSPI()
{
  // 1. RSPI2 モジュールストップ解除 (STBCR10 bit5 = RSPI2)
  //    RZ/A1H: STBCR10 bit7=RSPI0, bit6=RSPI1, bit5=RSPI2, bit4=RSPI3, bit3=RSPI4
  CPG.STBCR10 &= ~(1 << 5);
  volatile uint8_t dummy = CPG.STBCR10;
  (void)dummy;

  // 2. ピンマルチプレクス設定
  // P8_3 (RSPCK2), P8_5 (MOSI2), P8_6 (MISO2) → Function 3
  // Function 3: PFCAE=0, PFCE=1, PFC=0
  uint16_t spiPins = (1 << 3) | (1 << 5) | (1 << 6); // P8_3, P8_5, P8_6

  // PBDC クリア
  GPIO.PBDC8 &= ~spiPins;

  // ポートモード (PMC = 1: 代替機能)
  GPIO.PMC8 |= spiPins;

  // Function 3 選択 (PFCAE=0, PFCE=1, PFC=0)
  GPIO.PFCAE8 &= ~spiPins;
  GPIO.PFCE8  |= spiPins;
  GPIO.PFC8   &= ~spiPins;

  // PIPC = 1 (ペリフェラルによるI/O制御)
  GPIO.PIPC8 |= spiPins;

  // P8_4 (CS) → GPIO出力に設定
  GPIO.PMC8  &= ~(1 << 4);  // GPIO モード
  GPIO.PM8   &= ~(1 << 4);  // 出力モード
  GPIO.P8    |= (1 << 4);   // CS = High (デアサート)

  // P7_8 (CD) → GPIO入力に設定
  GPIO.PMC7  &= ~(1 << 8);  // GPIO モード
  GPIO.PM7   |= (1 << 8);   // 入力モード

  // 3. RSPI2 初期化
  // まず SPI を無効化
  RSPI2.SPCR = 0x00;

  // SPI モード設定
  // SSLP: SSL0 アクティブLow（使わないがデフォルトでOK）
  RSPI2.SSLP = 0x00;

  // SPPCR: MOSI アイドルレベル = High
  RSPI2.SPPCR = 0x20;  // MOIFV=1 (MOSIアイドル値 = 1)

  // SPSCR: シーケンス長 = 1 (SPCMD0のみ使用)
  RSPI2.SPSCR = 0x00;

  // SPDCR: バイトアクセスモード
  // mbed準拠: 8bit転送時は 0x20
  RSPI2.SPDCR = 0x20;

  // SPCKD, SSLND, SPND: 最小遅延
  RSPI2.SPCKD = 0x00;
  RSPI2.SSLND = 0x00;
  RSPI2.SPND  = 0x00;

  // SPBFCR: FIFOリセット + トリガカウント設定 (mbed準拠)
  RSPI2.SPBFCR = 0xF0;  // TXRST=1, RXRST=1, トリガ: 読み1/書き1
  RSPI2.SPBFCR = 0x30;  // リセット解除、トリガカウント維持

  // SPCMD0: コマンドレジスタ設定 (mbed spi_format 準拠)
  // bit11-8: SPB = 0111 (8ビット転送, DSS=0x7)
  // bit12:   LSBF = 0 (MSBファースト)
  // bit3-2:  BRDV = 00 (分周なし)
  // bit1:    CPOL = 0 (アイドルLow)
  // bit0:    CPHA = 0 (立ち上がりでサンプル)
  // SDカード SPI モード0: CPOL=0, CPHA=0
  RSPI2.SPCMD0 = 0x0700;  // SPB=0x7 << 8 = 8bit, MSB first, SPI mode 0

  // 初期化時は低速 (100kHz ~ 400kHz)
  setSPIClock(400000);

  // SPI 有効化
  // SPCR: SPE=1, MSTR=1 (マスタモード)
  RSPI2.SPCR = 0x48;  // SPE=1 (bit6), MSTR=1 (bit3)
}

void SDCard::setSPIClock(uint32_t hz)
{
  // RSPI ボーレート = P1φ / (2 × (SPBR + 1) × 2^BRDV)
  // P1φ = 66.67MHz
  // BRDV = 0 (SPCMD0 で設定済み) とすると:
  // SPBR = P1φ / (2 × hz) - 1
  uint32_t p1clk = 66666666;
  uint32_t spbr = p1clk / (2 * hz) - 1;
  if (spbr > 255) spbr = 255;
  RSPI2.SPBR = (uint8_t)spbr;
}

uint8_t SDCard::spiTransfer(uint8_t data)
{
  // mbed spi_master_write 準拠:
  // 1. データ書き込み (SPDR.UINT8[0] で8bitアクセス)
  RSPI2.SPDR.UINT8[0] = data;

  // 2. 送信完了待ち (TEND = SPSR bit6)
  while (!(RSPI2.SPSR & (1 << 6)))
    ;

  // 3. 受信データ読み取り
  return RSPI2.SPDR.UINT8[0];
}

void SDCard::csLow()
{
  GPIO.P8 &= ~(1 << 4);
}

void SDCard::csHigh()
{
  GPIO.P8 |= (1 << 4);
  // CS解放後にダミークロック1バイト
  spiTransfer(0xFF);
}

bool SDCard::isCardInserted()
{
  // CD ピン: Low = カードあり
  return !(GPIO.PPR7 & (1 << 8));
}

// ---- SDカード初期化シーケンス ----
int SDCard::initCard()
{
  // CS = High の状態で 80クロック以上送る（SDモードからSPIモードへ切替）
  csHigh();
  for (int i = 0; i < 16; i++) {
    spiTransfer(0xFF);
  }

  // CMD0: GO_IDLE_STATE (リセット)
  int r = sendCommand(0, 0);
  if (r != R1_IDLE_STATE) {
    g_serial.printf("SDCard: CMD0 失敗 (r=%d)\n", r);
    return -1;
  }

  // CMD8: SEND_IF_COND (電圧チェック, SD v2判定)
  r = sendCMD8();
  if (r == R1_IDLE_STATE) {
    // SD v2.x カード
    return initCardV2();
  } else if (r == (R1_IDLE_STATE | R1_ILLEGAL_COMMAND)) {
    // SD v1.x カード
    return initCardV1();
  } else {
    g_serial.printf("SDCard: CMD8 異常応答 (r=%d)\n", r);
    return -2;
  }
}

int SDCard::initCardV1()
{
  // ACMD41 を繰り返してアイドル状態を抜ける
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    int r = sendAppCommand(41, 0);
    if (r == 0) {
      cardType_ = CARD_V1;
      // ブロックサイズを512に設定 (CMD16)
      if (sendCommand(16, 512) != 0) {
        return -3;
      }
      // データ転送速度に変更
      setSPIClock(10000000); // 10MHz
      return 0;
    }
  }
  return -4; // タイムアウト
}

int SDCard::initCardV2()
{
  // ACMD41 (HCS=1) を繰り返してアイドル状態を抜ける
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    // 短い遅延
    for (volatile int j = 0; j < 1000; j++) ;

    int r = sendAppCommand(41, 0x40000000);  // HCS=1
    if (r == 0) {
      // CMD58 で CCS (Card Capacity Status) を確認
      uint32_t ocr = 0;
      if (sendCMD58(&ocr) != 0) {
        return -5;
      }

      if (ocr & 0x40000000) {
        // SDHC/SDXC (ブロックアドレス)
        cardType_ = CARD_V2_HC;
      } else {
        // SD v2 標準容量 (バイトアドレス)
        cardType_ = CARD_V2_SC;
        // ブロックサイズを512に設定
        if (sendCommand(16, 512) != 0) {
          return -6;
        }
      }

      // データ転送速度に変更
      setSPIClock(10000000); // 10MHz
      return 0;
    }
  }
  return -7; // タイムアウト
}

// ---- コマンド送信 ----
int SDCard::sendCommand(uint8_t cmd, uint32_t arg)
{
  // ビジーウェイト
  waitReady();

  csLow();

  // コマンドバイト: 0x40 | cmd
  spiTransfer(0x40 | cmd);
  spiTransfer((uint8_t)(arg >> 24));
  spiTransfer((uint8_t)(arg >> 16));
  spiTransfer((uint8_t)(arg >> 8));
  spiTransfer((uint8_t)(arg));

  // CRC (CMD0 と CMD8 のみ正しいCRCが必要)
  uint8_t crc = 0xFF;
  if (cmd == 0) crc = 0x95;
  if (cmd == 8) crc = 0x87;
  spiTransfer(crc);

  // レスポンス待ち (MSBが0になるまで)
  int response = -1;
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    uint8_t r = spiTransfer(0xFF);
    if (!(r & 0x80)) {
      response = r;
      break;
    }
  }

  csHigh();
  return response;
}

int SDCard::sendAppCommand(uint8_t cmd, uint32_t arg)
{
  // CMD55 (アプリケーションコマンドプレフィクス)
  sendCommand(55, 0);
  return sendCommand(cmd, arg);
}

int SDCard::sendCMD8()
{
  waitReady();
  csLow();

  // CMD8: チェックパターン 0x1AA, 電圧 2.7-3.6V
  spiTransfer(0x40 | 8);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0x01);  // 電圧範囲
  spiTransfer(0xAA);  // チェックパターン
  spiTransfer(0x87);  // CRC

  // R7 レスポンス (R1 + 4バイト)
  int response = -1;
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    uint8_t r = spiTransfer(0xFF);
    if (!(r & 0x80)) {
      response = r;
      // 残り4バイトを読み捨て
      spiTransfer(0xFF);
      spiTransfer(0xFF);
      spiTransfer(0xFF);
      spiTransfer(0xFF);
      break;
    }
  }

  csHigh();
  return response;
}

int SDCard::sendCMD58(uint32_t *ocr)
{
  waitReady();
  csLow();

  spiTransfer(0x40 | 58);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0xFF);

  // R3 レスポンス (R1 + 4バイトOCR)
  int response = -1;
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    uint8_t r = spiTransfer(0xFF);
    if (!(r & 0x80)) {
      response = r;
      uint32_t o = 0;
      o  = (uint32_t)spiTransfer(0xFF) << 24;
      o |= (uint32_t)spiTransfer(0xFF) << 16;
      o |= (uint32_t)spiTransfer(0xFF) << 8;
      o |= (uint32_t)spiTransfer(0xFF);
      if (ocr) *ocr = o;
      break;
    }
  }

  csHigh();
  return response;
}

// ---- ブロック読み書き ----
int SDCard::readBlocks(uint8_t *buf, uint32_t block, uint32_t count)
{
  if (!initialized_) return -1;

  // 標準容量カードはバイトアドレス
  uint32_t addr = (cardType_ == CARD_V2_HC) ? block : block * 512;

  for (uint32_t i = 0; i < count; i++) {
    waitReady();
    csLow();

    // CMD17: READ_SINGLE_BLOCK
    spiTransfer(0x40 | 17);
    spiTransfer((uint8_t)(addr >> 24));
    spiTransfer((uint8_t)(addr >> 16));
    spiTransfer((uint8_t)(addr >> 8));
    spiTransfer((uint8_t)(addr));
    spiTransfer(0xFF);

    // R1 レスポンス待ち
    int r = -1;
    for (int j = 0; j < CMD_TIMEOUT; j++) {
      uint8_t resp = spiTransfer(0xFF);
      if (!(resp & 0x80)) {
        r = resp;
        break;
      }
    }
    if (r != 0) {
      csHigh();
      return -2;
    }

    // データトークン待ち (0xFE)
    if (readData(buf + i * 512, 512) != 0) {
      csHigh();
      return -3;
    }

    csHigh();

    if (cardType_ == CARD_V2_HC) {
      addr++;
    } else {
      addr += 512;
    }
  }
  return 0;
}

int SDCard::writeBlocks(const uint8_t *buf, uint32_t block, uint32_t count)
{
  if (!initialized_) return -1;

  uint32_t addr = (cardType_ == CARD_V2_HC) ? block : block * 512;

  for (uint32_t i = 0; i < count; i++) {
    waitReady();
    csLow();

    // CMD24: WRITE_BLOCK
    spiTransfer(0x40 | 24);
    spiTransfer((uint8_t)(addr >> 24));
    spiTransfer((uint8_t)(addr >> 16));
    spiTransfer((uint8_t)(addr >> 8));
    spiTransfer((uint8_t)(addr));
    spiTransfer(0xFF);

    // R1 レスポンス待ち
    int r = -1;
    for (int j = 0; j < CMD_TIMEOUT; j++) {
      uint8_t resp = spiTransfer(0xFF);
      if (!(resp & 0x80)) {
        r = resp;
        break;
      }
    }
    if (r != 0) {
      csHigh();
      return -2;
    }

    // データ送信
    if (writeData(buf + i * 512, DATA_TOKEN_CMD24) != 0) {
      csHigh();
      return -3;
    }

    csHigh();

    if (cardType_ == CARD_V2_HC) {
      addr++;
    } else {
      addr += 512;
    }
  }
  return 0;
}

int SDCard::readData(uint8_t *buf, uint32_t len)
{
  // データトークン (0xFE) 待ち
  for (int i = 0; i < CMD_TIMEOUT * 10; i++) {
    uint8_t token = spiTransfer(0xFF);
    if (token == 0xFE) {
      // データ受信
      for (uint32_t j = 0; j < len; j++) {
        buf[j] = spiTransfer(0xFF);
      }
      // CRC (2バイト、無視)
      spiTransfer(0xFF);
      spiTransfer(0xFF);
      return 0;
    }
    if (token != 0xFF) {
      // エラートークン
      return -1;
    }
  }
  return -2; // タイムアウト
}

int SDCard::writeData(const uint8_t *buf, uint8_t token)
{
  // ダミーバイト
  spiTransfer(0xFF);

  // データトークン
  spiTransfer(token);

  // データ送信
  for (uint32_t i = 0; i < 512; i++) {
    spiTransfer(buf[i]);
  }

  // ダミー CRC
  spiTransfer(0xFF);
  spiTransfer(0xFF);

  // データレスポンストークン確認
  uint8_t resp = spiTransfer(0xFF);
  if ((resp & 0x1F) != 0x05) {
    return -1; // 書き込みエラー
  }

  // 書き込み完了待ち (ビジー = 0x00)
  for (int i = 0; i < CMD_TIMEOUT * 100; i++) {
    if (spiTransfer(0xFF) != 0x00) {
      return 0;
    }
  }
  return -2; // タイムアウト
}

void SDCard::waitReady()
{
  // ビジー解除待ち（0xFF が返ればレディ）
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    if (spiTransfer(0xFF) == 0xFF) {
      return;
    }
  }
}

uint32_t SDCard::readSectorCount()
{
  waitReady();
  csLow();

  // CMD9: SEND_CSD
  spiTransfer(0x40 | 9);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0x00);
  spiTransfer(0xFF);

  // R1 レスポンス待ち
  for (int i = 0; i < CMD_TIMEOUT; i++) {
    uint8_t r = spiTransfer(0xFF);
    if (!(r & 0x80)) {
      if (r != 0) {
        csHigh();
        return 0;
      }
      break;
    }
  }

  // CSD データ (16バイト) を読み取り
  uint8_t csd[16];
  if (readData(csd, 16) != 0) {
    csHigh();
    return 0;
  }
  csHigh();

  // CSD バージョンに応じてセクタ数を計算
  uint8_t csdVer = (csd[0] >> 6) & 0x03;

  if (csdVer == 0) {
    // CSD v1.0 (SD v1.x / v2.x 標準容量)
    uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10)
                    | ((uint32_t)csd[7] << 2)
                    | ((uint32_t)(csd[8] >> 6) & 0x03);
    uint32_t c_size_mult = ((uint32_t)(csd[9] & 0x03) << 1)
                         | ((uint32_t)(csd[10] >> 7) & 0x01);
    uint32_t read_bl_len = csd[5] & 0x0F;

    uint32_t block_len = 1 << read_bl_len;
    uint32_t mult = 1 << (c_size_mult + 2);
    uint32_t capacity = (c_size + 1) * mult * block_len;
    return capacity / 512;
  } else if (csdVer == 1) {
    // CSD v2.0 (SDHC/SDXC)
    uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16)
                    | ((uint32_t)csd[8] << 8)
                    | (uint32_t)csd[9];
    return (c_size + 1) * 1024;
  }

  return 0;
}

