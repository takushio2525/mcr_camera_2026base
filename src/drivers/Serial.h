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

class Serial : public IModule
{
public:
  // Constructor
  Serial();

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

  // Internal buffer for vsnprintf
  // ANSI色付き160文字行 + ヘッダ等を考慮して512バイトに拡大
  char buffer_[512];
};

extern Serial g_serial;

#endif /* DRIVERS_SERIAL_H_ */
