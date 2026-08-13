/*
 * Serial.cpp
 *
 *  Serial Communication Driver (Instant Output via SCIF2)
 *  For GR-PEACH (RZ/A1H)
 *  SCIF2: P6_3 (TxD2), P6_2 (RxD2) via DAPLink USB-Serial
 */

#include "Serial.h"
#include "iodefine.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// SCIF2 settings for GR-PEACH USB Serial
// SCIFのボーレートジェネレータは P1φ を使用（P0φではない）
// GR-PEACH Clock Mode 0: P1φ = 66,666,666 Hz
// mbed serial_api.c の serial_baud() と同一設定で 230400bps を実現

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

Serial::Serial(const SerialConfig& cfg) : _config(cfg) {}

bool Serial::init()
{
  // 1. SCIF2 のモジュールストップ解除 (STBCR4 bit5)
  CPG.STBCR4 &= ~(1 << 5);
  volatile uint8_t dummy = CPG.STBCR4; // Dummy read
  (void)dummy;

  // 2. ピンマルチプレクス設定 (SCIF2)
  // GR-PEACHでは DAPLink USB-Serial が以下のピンに接続:
  //   TxD2 -> P6_3 (Alt 7)
  //   RxD2 -> P6_2 (Alt 7)
  uint16_t pinMask = (1 << 2) | (1 << 3); // P6_2, P6_3

  // PBDC クリア
  GPIO.PBDC6 &= ~pinMask;

  // ポートモード (PMC = 1: 代替機能)
  GPIO.PMC6 |= pinMask;

  // Alt 7 選択 (PFCAE=1, PFCE=1, PFC=0)
  GPIO.PFCAE6 |= pinMask;
  GPIO.PFCE6 |= pinMask;
  GPIO.PFC6 &= ~pinMask;

  // PIPC = 1 (ペリフェラルによるI/O制御)
  GPIO.PIPC6 |= pinMask;

  // 3. Initialize SCIF2 Subsystem
  // Disable Tx/Rx (TE=0, RE=0) in SCSCR
  SCIF2.SCSCR = 0x0000;

  // Clear FIFO (TFRST=1, RFRST=1) in SCFCR
  SCIF2.SCFCR = 0x0006;

  // 上の SCFCR への書き込みで送受信 FIFO のリード/ライトポインタが 0 に戻る。
  //
  // ステータスフラグのクリア。SCFSR / SCLSR は W0C（1 が読めたビットに 0 を
  // 書くとクリアされる）なので、read-modify-write の &= ~mask で
  // 「mask のビットにだけ 0 を書き戻す」形になりクリアが成立する。
  // 逆に mask 外のビットは読んだ値がそのまま書き戻されるため、
  // ここでクリアされるのは下のマスクに含まれるフラグだけである。
  SCIF2.SCFSR &= ~0x00F3; // ER,TEND,TDFE,BRK,RDF,DR をクリア
  SCIF2.SCLSR &= ~0x0001; // ORER をクリア

  // SCSMR: 8-bit data, 1 stop bit, no parity, P0 clock
  SCIF2.SCSMR = 0x0000;

  // SCEMR: mbed serial_api.c と完全一致の設定
  // BGDM=1 (bit7), ABCS=1 (bit0) → 除算比 = 8
  // B = P1φ / (8 × (BRR+1))
  SCIF2.SCEMR = 0x0081;

  // BRR (Baud Rate Register) for 230400bps target
  // B = 66666666 / (8 × 36) = 231481 ≈ 230400bps (誤差0.47%)
  // mbed の DL = (P1CLK + 4*baud) / (8*baud) - 1 = 35 と同一
  SCIF2.SCBRR = 35;

  // Delay for a while (at least 1 bit time)
  for (volatile int i = 0; i < 1000; i++)
  {
  }

  // Clear FIFO reset
  SCIF2.SCFCR = 0x0000;

  // Re-enable Tx/Rx
  SCIF2.SCSCR = 0x0030; // TE=1, RE=1
  return true;
}

void Serial::updateOutput(SystemData& sys)
{
  (void)sys;
  // Dummy (do nothing, polling instant transmit is used instead)
}

void Serial::putChar(char c)
{
  // SCIF2 の送信FIFOに空きがあるまで待つ
  // SCFDR は上位バイト(bit15-8)=送信FIFO 格納数 T、
  //          下位バイト(bit7-0) =受信FIFO 格納数 R の 2 段構成。
  // ここは送信側を見るので >> 8 で上位バイトを取り出す（FIFO 段数は 16）。
  // FIFOに空きがあれば待たずに即座に書き込み
  while ((SCIF2.SCFDR >> 8) >= 16)
  {
    // 送信FIFO満杯の場合のみ待つ
  }

  // 送信FIFOに書き込み
  SCIF2.SCFTDR = c;

  // TDFEフラグをクリア（送信継続のため）
  SCIF2.SCFSR &= ~0x0060; // TDFE, TENDをクリア
}

void Serial::print(const char *str)
{
  while (*str != '\0')
  {
    // Convert \n to \r\n for standard serial terminal viewing
    if (*str == '\n')
    {
      putChar('\r');
    }
    putChar(*str++);
  }
}

void Serial::printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer_, sizeof(buffer_), fmt, args);
  va_end(args);

  print(buffer_);
}

extern "C" void c_printf(const char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  g_serial.print(buf);
}
