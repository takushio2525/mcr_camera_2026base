/*
 * Servo.cpp
 *
 *  サーボドライバ実装
 *  EMA 準拠版: SystemData / Config ベース
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        init_MTU2_PWM_Servo() / handle()
 */

#include "Servo.h"
#include "../core/SystemData.h"
#include "iodefine.h"
#include <typedefine.h>

// グローバルインスタンスの実体定義は main 側に移動済み（EMA 準拠）。

Servo::Servo(const ServoConfig& cfg) : _config(cfg), angle_(0) {}

// ---------------------------------------------------------------------
// init() — MTU2 チャンネル0 (PWMモード1) + P4_0 初期化
// ---------------------------------------------------------------------
bool Servo::init()
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
  // 0xf7 = ~0x08（Encoder::init() の `&= ~0x08` と同じ操作の別表記）
  CPGSTBCR3  &= 0xf7;

  // ---- MTU2 チャンネル0: PWMモード1 ----
  // TODO(要実機確認): 0x02 が「TCNT クリア(TGRA)」を含むかは未確定。
  //   Motor.cpp は同じ「TCNT クリア(TGRA)」に 0x20 を使っており、0x02 と
  //   0x20 が同じ意味になることはない。0x02 は分周比 (P0φ/16) だけを指定し、
  //   TCNT クリア要因なし（フリーラン）になっている可能性がある。
  //   さらに Servo.h が謳う「PWM周期 16ms」は 33332 カウント ÷ (P0φ/16 =
  //   2.083MHz) から出る値だが、_config.pwmCycle は init() のどのレジスタにも
  //   書かれていないため、実際の周期は未検証。
  //   TIOC0A (P4_0) の波形をオシロで実測して周期を確定させること。
  //   ビット定義は generate/iodefines/mtu2_iodefine.h に無いため、
  //   RZ/A1H ハードウェアマニュアルの MTU2 TCR を参照。
  MTU2TCR_0  = 0x02;                    // TCNT クリア(TGRA), P0φ/16
  MTU2TIORH_0 = 0x52;                   // TGRA: L→H, TGRB: H→L
  MTU2TMDR_0 = 0x32;                    // TGRC/TGRD バッファモード + PWMモード1
  MTU2TBTM_0 = 0x03;                    // バッファ転送: TCNTクリア時
  MTU2TCNT_0 = 0;                       // カウンタリセット
  // パルス立ち上がり位置 [カウント]。周期の先頭で立ち上げるための最小値で、
  // 0 ではなく 2 にしている根拠は参考プロジェクト由来で不明。
  // P0φ/16 = 2.083MHz なら 2 カウント ≒ 1μs なので実質「ほぼ 0」。
  // 立ち下がりが TGRB (= center = 3090) なのでパルス幅は
  // (3090 - 2) / 2.083MHz ≒ 1.48ms となり、サーボ中立の 1.5ms とほぼ一致する。
  MTU2TGRA_0 = MTU2TGRC_0 = 2;          // パルス立ち上がり位置
  MTU2TGRB_0 = MTU2TGRD_0 = _config.center; // 中心位置 (Config から)
  MTU2TSTR   |= 0x01;                   // TCNT_0 スタート
  return true;
}

// ---------------------------------------------------------------------
// updateOutput() — SystemData → ハードウェア反映
// sys.srv.angleCmd を読んで PWM に反映、sys.srv.angleActual に書き戻し。
// ---------------------------------------------------------------------
void Servo::updateOutput(SystemData& sys)
{
  setAngle(sys.srv.angleCmd);
  sys.srv.angleActual = angle_;
}

// ---------------------------------------------------------------------
// private: setAngle — ステアリング角度設定
// angle: -maxAngle(右)〜0(中央)〜+maxAngle(左) [度]
// 参考プロジェクト handle() に準拠: 正=左 (カウント値を減らす)
// ---------------------------------------------------------------------
void Servo::setAngle(int angle)
{
  angle_      = clamp(angle);
  // 中心から angle * handleStep だけオフセット
  // 正角度 → カウント減少 → 左回転
  MTU2TGRD_0 = _config.center - angle_ * _config.handleStep;
}

// ---------------------------------------------------------------------
// private: clamp
// ---------------------------------------------------------------------
int Servo::clamp(int val) const
{
  if (val >  _config.maxAngle) return  _config.maxAngle;
  if (val < -_config.maxAngle) return -_config.maxAngle;
  return val;
}
