/*
 * Serial.h
 *
 *  Serial Communication Driver (Instant Output via SCIF2)
 *  For GR-PEACH (RZ/A1H)
 *  SCIF2: P6_3 (TxD2), P6_2 (RxD2) via DAPLink USB-Serial
 */

#ifndef DRIVERS_SERIAL_H_
#define DRIVERS_SERIAL_H_

#include "../core/IModule.h"
#include <stdint.h>

// ====================================================================
// SerialConfig — SCIF2 のボーレート / ピン / バッファ設定
// (実体は ProjectConfig.h の SERIAL_CONFIG で定義)
// ====================================================================
struct SerialConfig
{
  uint32_t baud;        // 230400
  int      txPinFmt;    // P6_3 TxD2
  int      rxPinFmt;    // P6_2 RxD2
  int      bufferSize;  // 512 (現状はクラス内固定配列で 512 バイト)
};

// ====================================================================
// SerialData — Serial の出力データ（debug 用）
// SystemData::ser に集約される。Serial クラスが「所有」する自身のデータ。
// ====================================================================
struct SerialData
{
  // 送信文字累計（任意・debug 用）
  // 現状どこからも更新されない。putChar() は SystemData を経由せず
  // 直接 SCIF2 を叩き、updateOutput() は空実装のため。
  uint32_t txCount;
};

class Serial : public IModule
{
public:
  // Config を注入してインスタンス生成
  explicit Serial(const SerialConfig& cfg);

  // Initialize SCIF2 and configure pins
  bool init() override;

  // IModule dummy update
  void updateOutput(SystemData& sys) override;

  // Formatted print (instant output via polling)
  void printf(const char *fmt, ...);

  // Send a string
  void print(const char *str);

private:
  // Output a single character (waits for FIFO/Tx shift register)
  void putChar(char c);

  SerialConfig _config;

  // Internal buffer for vsnprintf
  // ANSI色付き160文字行 + ヘッダ等を考慮して512バイトに拡大
  char buffer_[512];
};

extern Serial g_serial;

#endif /* DRIVERS_SERIAL_H_ */
