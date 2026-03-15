/*
 * mock_registers.cpp
 *
 *  モックレジスタ変数の実体定義
 *  テストコードから ResetMockRegisters() を呼んで全レジスタを初期化する
 */

#include "iodefine.h"

// ============================================================
// 実体定義
// ============================================================

volatile uint8_t  mock_CPGSTBCR3  = 0xFF;

volatile uint8_t  mock_MTU2TSTR   = 0x00;

volatile uint8_t  mock_MTU2TCR_3  = 0x00;
volatile uint8_t  mock_MTU2TOCR1  = 0x00;
volatile uint8_t  mock_MTU2TOCR2  = 0x00;
volatile uint8_t  mock_MTU2TMDR_3 = 0x00;
volatile uint8_t  mock_MTU2TMDR_4 = 0x00;
volatile uint8_t  mock_MTU2TOER   = 0x00;
volatile uint16_t mock_MTU2TCNT_3 = 0;
volatile uint16_t mock_MTU2TCNT_4 = 0;
volatile uint16_t mock_MTU2TGRA_3 = 0;
volatile uint16_t mock_MTU2TGRC_3 = 0;
volatile uint16_t mock_MTU2TGRA_4 = 0;
volatile uint16_t mock_MTU2TGRC_4 = 0;
volatile uint16_t mock_MTU2TGRB_4 = 0;
volatile uint16_t mock_MTU2TGRD_4 = 0;

volatile uint8_t  mock_MTU2TCR_0   = 0x00;
volatile uint8_t  mock_MTU2TIORH_0 = 0x00;
volatile uint8_t  mock_MTU2TMDR_0  = 0x00;
volatile uint8_t  mock_MTU2TBTM_0  = 0x00;
volatile uint16_t mock_MTU2TCNT_0  = 0;
volatile uint16_t mock_MTU2TGRA_0  = 0;
volatile uint16_t mock_MTU2TGRC_0  = 0;
volatile uint16_t mock_MTU2TGRB_0  = 0;
volatile uint16_t mock_MTU2TGRD_0  = 0;

volatile uint8_t  mock_MTU2TCR_1  = 0x00;
volatile uint8_t  mock_MTU2TMDR_1 = 0x00;
volatile uint8_t  mock_MTU2TIOR_1 = 0x00;
volatile uint16_t mock_MTU2TCNT_1 = 0;

volatile uint16_t mock_GPIOPBDC4  = 0x0000;
volatile uint16_t mock_GPIOPFCAE4 = 0x0000;
volatile uint16_t mock_GPIOPFCE4  = 0x0000;
volatile uint16_t mock_GPIOPFC4   = 0x0000;
volatile uint16_t mock_GPIOP4     = 0x0000;
volatile uint16_t mock_GPIOPM4    = 0x0000;
volatile uint16_t mock_GPIOPMC4   = 0x0000;

volatile uint16_t mock_GPIOPIBC1  = 0x0000;
volatile uint16_t mock_GPIOPBDC1  = 0x0000;
volatile uint16_t mock_GPIOPM1    = 0x0000;
volatile uint16_t mock_GPIOPMC1   = 0x0000;
volatile uint16_t mock_GPIOPIPC1  = 0x0000;
volatile uint16_t mock_GPIOPFC1   = 0x0000;
volatile uint16_t mock_GPIOPFCE1  = 0x0000;

// ============================================================
// ヘルパー: テストのSetUp()で呼んで全レジスタをリセットする
// ============================================================
void ResetMockRegisters()
{
  mock_CPGSTBCR3  = 0xFF;
  mock_MTU2TSTR   = 0x00;

  mock_MTU2TCR_3  = 0x00;
  mock_MTU2TOCR1  = 0x00;
  mock_MTU2TOCR2  = 0x00;
  mock_MTU2TMDR_3 = 0x00;
  mock_MTU2TMDR_4 = 0x00;
  mock_MTU2TOER   = 0x00;
  mock_MTU2TCNT_3 = 0;
  mock_MTU2TCNT_4 = 0;
  mock_MTU2TGRA_3 = 0;
  mock_MTU2TGRC_3 = 0;
  mock_MTU2TGRA_4 = 0;
  mock_MTU2TGRC_4 = 0;
  mock_MTU2TGRB_4 = 0;
  mock_MTU2TGRD_4 = 0;

  mock_MTU2TCR_0   = 0x00;
  mock_MTU2TIORH_0 = 0x00;
  mock_MTU2TMDR_0  = 0x00;
  mock_MTU2TBTM_0  = 0x00;
  mock_MTU2TCNT_0  = 0;
  mock_MTU2TGRA_0  = 0;
  mock_MTU2TGRC_0  = 0;
  mock_MTU2TGRB_0  = 0;
  mock_MTU2TGRD_0  = 0;

  mock_MTU2TCR_1  = 0x00;
  mock_MTU2TMDR_1 = 0x00;
  mock_MTU2TIOR_1 = 0x00;
  mock_MTU2TCNT_1 = 0;

  mock_GPIOPBDC4  = 0x0000;
  mock_GPIOPFCAE4 = 0x0000;
  mock_GPIOPFCE4  = 0x0000;
  mock_GPIOPFC4   = 0x0000;
  mock_GPIOP4     = 0x0000;
  mock_GPIOPM4    = 0x0000;
  mock_GPIOPMC4   = 0x0000;

  mock_GPIOPIBC1  = 0x0000;
  mock_GPIOPBDC1  = 0x0000;
  mock_GPIOPM1    = 0x0000;
  mock_GPIOPMC1   = 0x0000;
  mock_GPIOPIPC1  = 0x0000;
  mock_GPIOPFC1   = 0x0000;
  mock_GPIOPFCE1  = 0x0000;
}
