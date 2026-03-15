/*
 * Servo.h
 *
 *  サーボドライバ (MTU2 チャンネル0 PWMモード1)
 *  For GR-PEACH (RZ/A1H)
 *
 *  サーボ出力: TIOC0A (P4_0)
 *  PWM周期: 16ms (P0φ/16 = 2.08MHz, SERVO_PWM_CYCLE = 33332)
 *  中心位置: SERVO_CENTER = 3090 カウント (約1.5ms)
 *  ステップ: HANDLE_STEP = 23 カウント/度
 */

#ifndef DRIVERS_SERVO_H_
#define DRIVERS_SERVO_H_

#include "../core/IModule.h"

class Servo : public IModule
{
public:
  // サーボPWM周期 (16ms / P0φ/16)
  static const int PWM_CYCLE    = 33332;
  // サーボ中心位置 (1.5ms相当のカウント値)
  static const int CENTER       = 3090;
  // 1度あたりのカウント変化量
  static const int HANDLE_STEP  = 23;
  // 最大操舵角 (度)
  static const int MAX_ANGLE    = 40;

  Servo();

  // MTU2チャンネル0 + P4_0 の初期化
  void init() override;

  // 周期処理 (現状は空: setAngle()で即時反映)
  void update() override;

  // ステアリング角度を設定
  // angle: -MAX_ANGLE(右)〜0(中央)〜+MAX_ANGLE(左) [度]
  // 注: 符号は参考プロジェクトの handle() 準拠 (正=左、負=右)
  void setAngle(int angle);

  // 現在の設定角度を取得
  int getAngle() const { return angle_; }

private:
  int angle_;

  // [-MAX_ANGLE, +MAX_ANGLE] にクランプ
  static int clamp(int val);
};

extern Servo g_servo;

#endif /* DRIVERS_SERVO_H_ */
