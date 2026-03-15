/*
 * Motor.cpp
 *
 *  モータードライバ実装
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        init_MTU2_PWM_Motor() / motor()
 */

#include "Motor.h"
#include "iodefine.h"
#include <typedefine.h>

// グローバルインスタンス
Motor g_motor;

Motor::Motor() : left_(0), right_(0) {}

// ---------------------------------------------------------------------
// init() — MTU2 チャンネル3/4 (PWM) + P4_6/P4_7 (方向ピン) 初期化
// ---------------------------------------------------------------------
void Motor::init()
{
  // ---- P4_4, P4_5: MTU2 代替機能 (PWM出力) ----
  GPIOPBDC4  = 0x0000;   // 双方向モード無効
  GPIOPFCAE4 &= 0xffcf;  // P4_4, P4_5: 代替機能選択 (bit4,5 クリア)
  GPIOPFCE4  |= 0x0030;  // P4_4, P4_5: 代替機能選択 (bit4,5 セット)
  GPIOPFC4   &= 0xffcf;  // P4_4, P4_5: 第2代替機能
  GPIOP4     &= 0xffcf;  // P4_4, P4_5: 初期値クリア
  GPIOPM4    &= 0xffcf;  // P4_4, P4_5: 出力モード
  GPIOPMC4   |= 0x0030;  // P4_4, P4_5: ペリフェラル制御 (double)

  // ---- P4_6, P4_7: GPIO出力 (方向制御) ----
  GPIOPMC4   &= ~0x00c0; // P4_6, P4_7: GPIOモード (ペリフェラル解除)
  GPIOPM4    &= ~0x00c0; // P4_6, P4_7: 出力モード
  GPIOP4     &= ~0x00c0; // P4_6, P4_7: 初期値=0 (前進)

  // ---- MTU2 スタンバイ解除 (STBCR3 bit3 = MTU2) ----
  CPGSTBCR3  &= 0xf7;

  // ---- MTU2 チャンネル3/4: リセット同期PWMモード ----
  MTU2TCR_3  = 0x20;            // TCNT クリア(TGRA), P0φ/1
  MTU2TOCR1  = 0x04;
  MTU2TOCR2  = 0x40;            // N: L→H / P: H→L
  MTU2TMDR_3 = 0x38;            // バッファON + リセット同期PWMモード
  MTU2TMDR_4 = 0x30;            // バッファON
  MTU2TOER   = 0xc6;            // TIOC3B, TIOC4A, TIOC4B 出力有効
  MTU2TCNT_3 = 0;
  MTU2TCNT_4 = 0;
  MTU2TGRA_3 = MTU2TGRC_3 = PWM_CYCLE; // PWM周期 (1ms)
  MTU2TGRA_4 = MTU2TGRC_4 = 0;         // 左モーター初期デューティ=0
  MTU2TGRB_4 = MTU2TGRD_4 = 0;         // 右モーター初期デューティ=0
  MTU2TSTR   |= 0x40;                   // TCNT_4 スタート
}

void Motor::update()
{
  // set() で即時反映するため、ここでは何もしない
}

// ---------------------------------------------------------------------
// set() — 左右モーター速度設定
// val: -100(後退)〜0(停止)〜+100(前進)
// ---------------------------------------------------------------------
void Motor::set(int left, int right)
{
  left_  = clamp(left);
  right_ = clamp(right);
  applyLeft(left_);
  applyRight(right_);
}

void Motor::stop()
{
  set(0, 0);
}

// ---------------------------------------------------------------------
// private: clamp
// ---------------------------------------------------------------------
int Motor::clamp(int val)
{
  if (val >  MAX_POWER) return  MAX_POWER;
  if (val < -MAX_POWER) return -MAX_POWER;
  return val;
}

// ---------------------------------------------------------------------
// private: applyLeft — 左モーターへ書き込み
// ---------------------------------------------------------------------
void Motor::applyLeft(int val)
{
  if (val >= 0)
  {
    // 前進: P4_6 = 0
    GPIOP4    &= ~0x0040u;
    MTU2TGRC_4 = (long)(PWM_CYCLE - 1) * val / 100;
  }
  else
  {
    // 後退: P4_6 = 1
    GPIOP4    |= 0x0040u;
    MTU2TGRC_4 = (long)(PWM_CYCLE - 1) * (-val) / 100;
  }
}

// ---------------------------------------------------------------------
// private: applyRight — 右モーターへ書き込み
// ---------------------------------------------------------------------
void Motor::applyRight(int val)
{
  if (val >= 0)
  {
    // 前進: P4_7 = 0
    GPIOP4    &= ~0x0080u;
    MTU2TGRD_4 = (long)(PWM_CYCLE - 1) * val / 100;
  }
  else
  {
    // 後退: P4_7 = 1
    GPIOP4    |= 0x0080u;
    MTU2TGRD_4 = (long)(PWM_CYCLE - 1) * (-val) / 100;
  }
}
