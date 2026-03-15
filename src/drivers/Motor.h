/*
 * Motor.h
 *
 *  モータードライバ (MTU2 チャンネル3/4 リセット同期PWMモード)
 *  For GR-PEACH (RZ/A1H)
 *
 *  左モーター: TIOC4A (P4_4), 方向: P4_6
 *  右モーター: TIOC4B (P4_5), 方向: P4_7
 *  PWM周期: 1ms (P0φ/1 = 33.33MHz, MOTOR_PWM_CYCLE = 33332)
 */

#ifndef DRIVERS_MOTOR_H_
#define DRIVERS_MOTOR_H_

#include "../core/IModule.h"

class Motor : public IModule
{
public:
  // モーターPWM周期 (1ms / P0φ/1)
  static const int PWM_CYCLE = 33332;
  // 最大出力 (%)
  static const int MAX_POWER = 100;

  Motor();

  // MTU2チャンネル3/4 + 方向ピン (P4_4〜P4_7) の初期化
  void init() override;

  // 周期処理 (現状は空: set()で即時反映)
  void update() override;

  // 左右モーターの速度を設定
  // left, right: -100(後退)〜0(停止)〜+100(前進)
  // 範囲外の値は自動クランプする
  void set(int left, int right);

  // 両モーターを停止 (set(0, 0) と同等)
  void stop();

  // 現在の設定値を取得
  int getLeft() const { return left_; }
  int getRight() const { return right_; }

private:
  int left_;
  int right_;

  // [-MAX_POWER, +MAX_POWER] にクランプ
  static int clamp(int val);

  // 左右モーターへの低レベル書き込み
  void applyLeft(int val);
  void applyRight(int val);
};

extern Motor g_motor;

#endif /* DRIVERS_MOTOR_H_ */
