/*
 * Servo.cpp
 *
 *  サーボドライバ実装
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        init_MTU2_PWM_Servo() / handle()
 */

#include "Servo.h"
#include "iodefine.h"
#include <typedefine.h>

// グローバルインスタンス
Servo g_servo;

Servo::Servo() : angle_(0) {}

// ---------------------------------------------------------------------
// init() — MTU2 チャンネル0 (PWMモード1) + P4_0 初期化
// ---------------------------------------------------------------------
void Servo::init()
{
  // ---- P4_0: MTU2 代替機能 (PWM出力) ----
  GPIOPBDC4  = 0x0000;   // 双方向モード無効
  GPIOPFCAE4 &= 0xfffe;  // P4_0: 代替機能選択 (bit0 クリア)
  GPIOPFCE4  &= 0xfffe;  // P4_0: 代替機能選択 (bit0 クリア)
  GPIOPFC4   |= 0x0001;  // P4_0: 第2代替機能 (bit0 セット)
  GPIOP4     &= 0xfffe;  // P4_0: 初期値クリア
  GPIOPM4    &= 0xfffe;  // P4_0: 出力モード
  GPIOPMC4   |= 0x0001;  // P4_0: ペリフェラル制御 (double)

  // ---- MTU2 スタンバイ解除 (STBCR3 bit3 = MTU2) ----
  CPGSTBCR3  &= 0xf7;

  // ---- MTU2 チャンネル0: PWMモード1 ----
  MTU2TCR_0  = 0x02;            // TCNT クリア(TGRA), P0φ/16
  MTU2TIORH_0 = 0x52;          // TGRA: L→H, TGRB: H→L
  MTU2TMDR_0 = 0x32;            // TGRC/TGRD バッファモード + PWMモード1
  MTU2TBTM_0 = 0x03;            // バッファ転送: TCNTクリア時
  MTU2TCNT_0 = 0;               // カウンタリセット
  MTU2TGRA_0 = MTU2TGRC_0 = 2; // パルス立ち上がり位置
  MTU2TGRB_0 = MTU2TGRD_0 = CENTER; // 中心位置 (1.5ms)
  MTU2TSTR   |= 0x01;           // TCNT_0 スタート
}

void Servo::update()
{
  // setAngle() で即時反映するため、ここでは何もしない
}

// ---------------------------------------------------------------------
// setAngle() — ステアリング角度設定
// angle: -MAX_ANGLE(右)〜0(中央)〜+MAX_ANGLE(左) [度]
// 参考プロジェクト handle() に準拠: 正=左 (カウント値を減らす)
// ---------------------------------------------------------------------
void Servo::setAngle(int angle)
{
  angle_      = clamp(angle);
  // 中心から angle * HANDLE_STEP だけオフセット
  // 正角度 → カウント減少 → 左回転
  MTU2TGRD_0 = CENTER - angle_ * HANDLE_STEP;
}

// ---------------------------------------------------------------------
// private: clamp
// ---------------------------------------------------------------------
int Servo::clamp(int val)
{
  if (val >  MAX_ANGLE) return  MAX_ANGLE;
  if (val < -MAX_ANGLE) return -MAX_ANGLE;
  return val;
}
