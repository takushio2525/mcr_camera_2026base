/*
 * Serial.h
 *
 *  Serial Communication Driver (Instant Output via SCIF2)
 *  For GR-PEACH (RZ/A1H)
 */

#ifndef DRIVERS_SERIAL_H_
#define DRIVERS_SERIAL_H_

#include "../core/IModule.h"

class Serial : public IModule {
public:
  // Constructor
  Serial();

  // Initialize SCIF2 and configure pins
  void init() override;

  // IModule dummy update
  void update() override;

  // Formatted print (instant output via polling)
  void printf(const char *fmt, ...);

  // Send a string
  void print(const char *str);

private:
  // Output a single character (waits for FIFO/Tx shift register)
  void putChar(char c);

  // Internal buffer for vsnprintf
  char buffer_[256];
};

extern Serial g_serial;

#endif /* DRIVERS_SERIAL_H_ */
