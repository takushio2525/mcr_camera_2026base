/*
 * Motor.h
 *
 *  モータードライバ (MTU2 チャンネル3/4 リセット同期PWMモード)
 *  EMA 準拠版: SystemData / Config ベース
 *  For GR-PEACH (RZ/A1H)
 *
 *  左モーター: TIOC4A (P4_4), 方向: P4_6
 *  右モーター: TIOC4B (P4_5), 方向: P4_7
 *  PWM周期: 1ms (P0φ/1 = 33.33MHz, MotorConfig.pwmCycle = 33332)
 *
 *  Output : updateOutput(sys) → sys.mot.leftCmd/rightCmd を PWM/方向に反映
 *           反映後の値を sys.mot.leftActual/rightActual に書き戻す
 */

#ifndef DRIVERS_MOTOR_H_
#define DRIVERS_MOTOR_H_

#include "../core/IModule.h"

// ====================================================================
// MotorConfig — Motor のハードウェア / 動作設定
// (実体は ProjectConfig.h の MOTOR_CONFIG で定義)
// ====================================================================
struct MotorConfig
{
  int pwmCycle;       // 33332 (1ms / P0φ/1)
  int maxPower;       // 70    (出力 % 上限。実値は ProjectConfig.h:82)
  int leftPwmPinFmt;  // ピン番号は形式的（実際の MTU2 + ピン設定はドライバ内）
  int rightPwmPinFmt;
  int leftDirPinFmt;
  int rightDirPinFmt;
};

// ====================================================================
// MotorData — Motor の入出力データ
// SystemData::mot に集約される。Motor クラスが「所有」する自身のデータ。
// ====================================================================
struct MotorData
{
  int leftCmd;       // Logic → Motor 指示値 [-100..+100]
  int rightCmd;
  int leftActual;    // Motor::updateOutput が反映後に書き戻し（debug 用）
  int rightActual;
};

class Motor : public IModule
{
public:
  // モーターPWM周期 (1ms / P0φ/1) — 後方互換のため static const も保持
  static const int PWM_CYCLE = 33332;
  // 最大出力 (%) — 後方互換のため static const も保持
  //
  // 注意: この値は MOTOR_CONFIG.maxPower（実値 70）と一致していない。
  // ファーム側のクランプ・走行ロジックはすべて _config.maxPower を使うため
  // 実走行に影響はなく、現在この定数を参照しているのは tests/ のみ。
  // 値を変えるときは ProjectConfig.h 側が正であることに注意。
  static const int MAX_POWER = 100;

  // Config を注入してインスタンス生成
  explicit Motor(const MotorConfig& cfg);

  // MTU2チャンネル3/4 + 方向ピン (P4_4〜P4_7) の初期化
  bool init() override;

  // Output : sys.mot.leftCmd/rightCmd を内部に転記して PWM 反映、
  //          反映後の値を sys.mot.leftActual/rightActual に書き戻し
  void updateOutput(SystemData& sys) override;

  // 現在の設定値を取得 (debug / 旧 SDLogger 用、Step 12 で SystemData 経由に移行予定)
  int getLeft() const { return left_; }
  int getRight() const { return right_; }

private:
  // 左右モーターの速度を内部で設定
  // left, right: -100(後退)〜0(停止)〜+100(前進)
  // 範囲外の値は自動クランプする
  void set(int left, int right);

  // 両モーターを停止 (set(0, 0) と同等)
  void stop();

  // [-maxPower, +maxPower] にクランプ
  int clamp(int val) const;

  // 左右モーターへの低レベル書き込み
  void applyLeft(int val);
  void applyRight(int val);

  MotorConfig _config;
  int left_;
  int right_;
};

extern Motor g_motor;

#endif /* DRIVERS_MOTOR_H_ */
