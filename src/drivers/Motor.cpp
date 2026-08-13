/*
 * Motor.cpp
 *
 *  モータードライバ実装
 *  EMA 準拠版: SystemData / Config ベース
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        init_MTU2_PWM_Motor() / motor()
 */

#include "Motor.h"
#include "../core/SystemData.h"
#include "iodefine.h"
#include <typedefine.h>

// グローバルインスタンスの実体定義は main 側に移動済み（EMA 準拠）。
// 後方互換用に extern 宣言は Motor.h に残してある。

Motor::Motor(const MotorConfig& cfg) : _config(cfg), left_(0), right_(0) {}

// ---------------------------------------------------------------------
// init() — MTU2 チャンネル3/4 (PWM) + P4_6/P4_7 (方向ピン) 初期化
// ---------------------------------------------------------------------
bool Motor::init()
{
  // ---- P4_4, P4_5: MTU2 代替機能 (PWM出力) ----
  //
  // ビットマスクの読み方（GPIO レジスタはポート内のピン番号 = ビット番号）:
  //   0x0030 = bit5|bit4        → P4_5, P4_4 を立てる
  //   0xffcf = ~0x0030          → P4_5, P4_4 だけを落とす（他ピンは保持）
  //   0x00c0 = bit7|bit6        → P4_7, P4_6（下段の方向ピン）
  //
  // ピンの代替機能は (PFCAE, PFCE, PFC) の 3 ビットの組み合わせで選ぶ。
  // ここは (0, 1, 0)。Servo.cpp の P4_0 は (0, 0, 1) で、同じく
  // 「第2代替機能」とコメントされているが組み合わせは異なる。
  // どちらの呼称が正しいかは RZ/A1H ハードウェアマニュアルで要確認。
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
  // 0xf7 = ~0x08。Encoder::init() は同じ操作を `&= ~0x08` と書いている
  // （表記が違うだけで意味は同じ）。
  CPGSTBCR3  &= 0xf7;

  // ---- MTU2 チャンネル3/4: リセット同期PWMモード ----
  MTU2TCR_3  = 0x20;            // TCNT クリア(TGRA), P0φ/1
  // タイマ出力制御レジスタ1。ビットフィールド定義は
  // generate/iodefines/mtu2_iodefine.h に無く、本ファイル冒頭に記載の
  // 参考プロジェクト init_MTU2_PWM_Motor() から移植した値。
  // 各ビットの意味は RZ/A1H ハードウェアマニュアルの MTU2 TOCR1 を参照。
  MTU2TOCR1  = 0x04;
  MTU2TOCR2  = 0x40;            // N: L→H / P: H→L
  MTU2TMDR_3 = 0x38;            // バッファON + リセット同期PWMモード
  MTU2TMDR_4 = 0x30;            // バッファON
  MTU2TOER   = 0xc6;            // TIOC3B, TIOC4A, TIOC4B 出力有効
  MTU2TCNT_3 = 0;
  MTU2TCNT_4 = 0;
  MTU2TGRA_3 = MTU2TGRC_3 = _config.pwmCycle; // PWM周期 (Config から)
  MTU2TGRA_4 = MTU2TGRC_4 = 0;                // 左モーター初期デューティ=0
  MTU2TGRB_4 = MTU2TGRD_4 = 0;                // 右モーター初期デューティ=0
  MTU2TSTR   |= 0x40;                          // TCNT_4 スタート
  return true;
}

// ---------------------------------------------------------------------
// updateOutput() — SystemData → ハードウェア反映
// sys.mot.leftCmd / rightCmd を読んで PWM に反映し、
// 反映後の値を sys.mot.leftActual / rightActual に書き戻す。
// ---------------------------------------------------------------------
void Motor::updateOutput(SystemData& sys)
{
  set(sys.mot.leftCmd, sys.mot.rightCmd);
  sys.mot.leftActual  = left_;
  sys.mot.rightActual = right_;
}

// ---------------------------------------------------------------------
// private: set — 左右モーター速度設定
// val: -maxPower(後退)〜0(停止)〜+maxPower(前進)
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
int Motor::clamp(int val) const
{
  if (val >  _config.maxPower) return  _config.maxPower;
  if (val < -_config.maxPower) return -_config.maxPower;
  return val;
}

// ---------------------------------------------------------------------
// private: applyLeft — 左モーターへ書き込み
//
// 左モーターは配線/ギアの都合で正転=後退となっているため、
// ソフト側で方向ピン (P4_6) の極性を反転して補正する。
// （右モーターは反転不要）
// ---------------------------------------------------------------------
void Motor::applyLeft(int val)
{
  if (val >= 0)
  {
    // 前進: P4_6 = 1（極性反転）
    GPIOP4    |= 0x0040u;
    MTU2TGRC_4 = (long)(_config.pwmCycle - 1) * val / 100;
  }
  else
  {
    // 後退: P4_6 = 0（極性反転）
    GPIOP4    &= ~0x0040u;
    MTU2TGRC_4 = (long)(_config.pwmCycle - 1) * (-val) / 100;
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
    MTU2TGRD_4 = (long)(_config.pwmCycle - 1) * val / 100;
  }
  else
  {
    // 後退: P4_7 = 1
    GPIOP4    |= 0x0080u;
    MTU2TGRD_4 = (long)(_config.pwmCycle - 1) * (-val) / 100;
  }
}
