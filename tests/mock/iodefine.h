/*
 * tests/mock/iodefine.h
 *
 *  ハードウェアレジスタのモック定義 (ホスト側ユニットテスト用)
 *  実機の iodefine.h の代わりにインクルードされ、
 *  レジスタアクセスを通常のメモリ変数に置き換える。
 */

#pragma once
#include <stdint.h>

// ============================================================
// モックレジスタ変数 (extern 宣言)
// 実体は tests/mock/mock_registers.cpp に定義
// ============================================================

// ---- CPG (クロック) ----
extern volatile uint8_t  mock_CPGSTBCR3;

// ---- MTU2: タイマ共通 ----
extern volatile uint8_t  mock_MTU2TSTR;

// ---- MTU2 チャンネル3/4 (モーター) ----
extern volatile uint8_t  mock_MTU2TCR_3;
extern volatile uint8_t  mock_MTU2TOCR1;
extern volatile uint8_t  mock_MTU2TOCR2;
extern volatile uint8_t  mock_MTU2TMDR_3;
extern volatile uint8_t  mock_MTU2TMDR_4;
extern volatile uint8_t  mock_MTU2TOER;
extern volatile uint16_t mock_MTU2TCNT_3;
extern volatile uint16_t mock_MTU2TCNT_4;
extern volatile uint16_t mock_MTU2TGRA_3;
extern volatile uint16_t mock_MTU2TGRC_3;
extern volatile uint16_t mock_MTU2TGRA_4;
extern volatile uint16_t mock_MTU2TGRC_4;
extern volatile uint16_t mock_MTU2TGRB_4;
extern volatile uint16_t mock_MTU2TGRD_4;

// ---- MTU2 チャンネル0 (サーボ) ----
extern volatile uint8_t  mock_MTU2TCR_0;
extern volatile uint8_t  mock_MTU2TIORH_0;
extern volatile uint8_t  mock_MTU2TMDR_0;
extern volatile uint8_t  mock_MTU2TBTM_0;
extern volatile uint16_t mock_MTU2TCNT_0;
extern volatile uint16_t mock_MTU2TGRA_0;
extern volatile uint16_t mock_MTU2TGRC_0;
extern volatile uint16_t mock_MTU2TGRB_0;
extern volatile uint16_t mock_MTU2TGRD_0;

// ---- MTU2 チャンネル1 (エンコーダ) ----
extern volatile uint8_t  mock_MTU2TCR_1;
extern volatile uint8_t  mock_MTU2TMDR_1;
extern volatile uint8_t  mock_MTU2TIOR_1;
extern volatile uint16_t mock_MTU2TCNT_1;

// ---- GPIO ポート4 (モーター/サーボ) ----
extern volatile uint16_t mock_GPIOPBDC4;
extern volatile uint16_t mock_GPIOPFCAE4;
extern volatile uint16_t mock_GPIOPFCE4;
extern volatile uint16_t mock_GPIOPFC4;
extern volatile uint16_t mock_GPIOP4;
extern volatile uint16_t mock_GPIOPM4;
extern volatile uint16_t mock_GPIOPMC4;

// ---- GPIO ポート1 (エンコーダ) ----
extern volatile uint16_t mock_GPIOPIBC1;
extern volatile uint16_t mock_GPIOPBDC1;
extern volatile uint16_t mock_GPIOPM1;
extern volatile uint16_t mock_GPIOPMC1;
extern volatile uint16_t mock_GPIOPIPC1;
extern volatile uint16_t mock_GPIOPFC1;
extern volatile uint16_t mock_GPIOPFCE1;

// ============================================================
// マクロ: ドライバコードのレジスタアクセスをモック変数へ転送
// ============================================================

#define CPGSTBCR3    mock_CPGSTBCR3

#define MTU2TSTR     mock_MTU2TSTR

#define MTU2TCR_3    mock_MTU2TCR_3
#define MTU2TOCR1    mock_MTU2TOCR1
#define MTU2TOCR2    mock_MTU2TOCR2
#define MTU2TMDR_3   mock_MTU2TMDR_3
#define MTU2TMDR_4   mock_MTU2TMDR_4
#define MTU2TOER     mock_MTU2TOER
#define MTU2TCNT_3   mock_MTU2TCNT_3
#define MTU2TCNT_4   mock_MTU2TCNT_4
#define MTU2TGRA_3   mock_MTU2TGRA_3
#define MTU2TGRC_3   mock_MTU2TGRC_3
#define MTU2TGRA_4   mock_MTU2TGRA_4
#define MTU2TGRC_4   mock_MTU2TGRC_4
#define MTU2TGRB_4   mock_MTU2TGRB_4
#define MTU2TGRD_4   mock_MTU2TGRD_4

#define MTU2TCR_0    mock_MTU2TCR_0
#define MTU2TIORH_0  mock_MTU2TIORH_0
#define MTU2TMDR_0   mock_MTU2TMDR_0
#define MTU2TBTM_0   mock_MTU2TBTM_0
#define MTU2TCNT_0   mock_MTU2TCNT_0
#define MTU2TGRA_0   mock_MTU2TGRA_0
#define MTU2TGRC_0   mock_MTU2TGRC_0
#define MTU2TGRB_0   mock_MTU2TGRB_0
#define MTU2TGRD_0   mock_MTU2TGRD_0

#define MTU2TCR_1    mock_MTU2TCR_1
#define MTU2TMDR_1   mock_MTU2TMDR_1
#define MTU2TIOR_1   mock_MTU2TIOR_1
#define MTU2TCNT_1   mock_MTU2TCNT_1

#define GPIOPBDC4    mock_GPIOPBDC4
#define GPIOPFCAE4   mock_GPIOPFCAE4
#define GPIOPFCE4    mock_GPIOPFCE4
#define GPIOPFC4     mock_GPIOPFC4
#define GPIOP4       mock_GPIOP4
#define GPIOPM4      mock_GPIOPM4
#define GPIOPMC4     mock_GPIOPMC4

#define GPIOPIBC1    mock_GPIOPIBC1
#define GPIOPBDC1    mock_GPIOPBDC1
#define GPIOPM1      mock_GPIOPM1
#define GPIOPMC1     mock_GPIOPMC1
#define GPIOPIPC1    mock_GPIOPIPC1
#define GPIOPFC1     mock_GPIOPFC1
#define GPIOPFCE1    mock_GPIOPFCE1
