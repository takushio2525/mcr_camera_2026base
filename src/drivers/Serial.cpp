/*
 * Serial.cpp
 *
 *  Serial Communication Driver (Instant Output via SCIF2)
 *  For GR-PEACH (RZ/A1H)
 *  SCIF2: P3_0 (TxD2), P3_2 (RxD2)
 */

#include "Serial.h"
#include "iodefine.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

// SCIF2 settings for GR-PEACH USB Serial
// P0 phi = 33.3333... MHz
// Baud rate: 115200 bps
// N = ((P0 / (64 * 2^(2n-1) * B)) * 10^6) - 1
// For 115200: n=0 (clock P0), N = approx 8.04 -> 8

Serial g_serial;

Serial::Serial() {}

void Serial::init() {
  // 1. Cancel module stop for SCIF2 (bit 14 of STBCR4)
  CPG.STBCR4 &= ~(1 << 14);
  volatile uint8_t dummy = CPG.STBCR4; // Dummy read
  (void)dummy;

  // 2. Configure Pin Multiplexing for SCIF2
  // TxD2 -> P3_0 (Alt 6)
  // RxD2 -> P3_2 (Alt 4)

  // Clear PBDC
  GPIO.PBDC3 &= ~((1 << 0) | (1 << 2));

  // Set Port Mode (PMC = 1)
  GPIO.PMC3 |= ((1 << 0) | (1 << 2));

  // Select Alt function
  // P3_0: Alt 6 (PFCAE=1, PFCE=0, PFC=1)
  GPIO.PFCAE3 |= (1 << 0);
  GPIO.PFCE3 &= ~(1 << 0);
  GPIO.PFC3 |= (1 << 0);

  // P3_2: Alt 4 (PFCAE=0, PFCE=1, PFC=1)
  GPIO.PFCAE3 &= ~(1 << 2);
  GPIO.PFCE3 |= (1 << 2);
  GPIO.PFC3 |= (1 << 2);

  // Set PIPC (1 = Software I/O control not used, driven by peripheral)
  GPIO.PIPC3 |= ((1 << 0) | (1 << 2));

  // 3. Initialize SCIF2 Subsystem
  // Disable Tx/Rx (TE=0, RE=0) in SCSCR
  SCIF2.SCSCR = 0x0000;

  // Clear FIFO (TFRST=1, RFRST=1) in SCFCR
  SCIF2.SCFCR = 0x0006;

  // Set Read/Write pointers in FIFO to 0
  SCIF2.SCFSR = 0x0000;
  SCIF2.SCLSR = 0x0000;

  // SCSMR: 8-bit data, 1 stop bit, no parity, P0 clock
  SCIF2.SCSMR = 0x0000;

  // SCEMR: Default (BGDM=0) -> N calculation uses P0phi/64
  SCIF2.SCEMR = 0x0000;

  // BRR (Baud Rate Register) for 115200 target
  // Actual BRR = 8
  SCIF2.SCBRR = 8;

  // Delay for a while (at least 1 bit time)
  for (volatile int i = 0; i < 1000; i++) {
  }

  // Clear FIFO reset
  SCIF2.SCFCR = 0x0000;

  // Re-enable Tx/Rx
  SCIF2.SCSCR = 0x0030; // TE=1, RE=1
}

void Serial::update() {
  // Dummy (do nothing, polling instant transmit is used instead)
}

void Serial::putChar(char c) {
  // Wait until TDFE (Transmit Data FIFO Empty) is 1
  while ((SCIF2.SCFSR & 0x0020) == 0) {
    // Wait
  }

  // Write the character to the transmit FIFO
  SCIF2.SCFTDR = c;

  // Clear TDFE to allow transmission to start/continue
  SCIF2.SCFSR &= ~0x0020;
}

void Serial::print(const char *str) {
  while (*str != '\0') {
    // Convert \n to \r\n for standard serial terminal viewing
    if (*str == '\n') {
      putChar('\r');
    }
    putChar(*str++);
  }
}

void Serial::printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer_, sizeof(buffer_), fmt, args);
  va_end(args);

  print(buffer_);
}
