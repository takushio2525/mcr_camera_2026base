/*
 * Servo.h
 *
 *  サーボドライバ (MTU2 チャンネル0 PWMモード1)
 *  EMA 準拠版: SystemData / Config ベース
 *  For GR-PEACH (RZ/A1H)
 *
 *  サーボ出力: TIOC0A (P4_0)
 *  PWM周期: 16ms (P0φ/16)
 *    ※ 16ms は pwmCycle=33332 ÷ (P0φ/16 = 2.083MHz) から出る設計値だが、
 *       Servo::init() は _config.pwmCycle をどのレジスタにも書いていない。
 *       実際の周期は未検証（Servo::init() の TODO(要実機確認) を参照）。
 *  中心位置: ServoConfig.center カウント (約1.5ms)
 *  ステップ: ServoConfig.handleStep カウント/度
 *
 *  Output : updateOutput(sys) → sys.srv.angleCmd を PWM デューティに反映
 *           反映後の値を sys.srv.angleActual に書き戻す
 */

#ifndef DRIVERS_SERVO_H_
#define DRIVERS_SERVO_H_

#include "../core/IModule.h"

// ====================================================================
// ServoConfig — Servo のハードウェア / 動作設定
// (実体は ProjectConfig.h の SERVO_CONFIG で定義)
// ====================================================================
struct ServoConfig
{
  int pwmCycle;       // 33332 — Servo.cpp から参照されていない（未配線）
  int center;         // 3090 (1.5ms 相当)
  int handleStep;     // 23   (カウント / 度)
  int maxAngle;       // 40   (度)
  int pwmPinFmt;
};

// ====================================================================
// ServoData — Servo の入出力データ
// SystemData::srv に集約される。Servo クラスが「所有」する自身のデータ。
// ====================================================================
struct ServoData
{
  int angleCmd    = 0;  // Logic → Servo 指示角 [-MAX..+MAX] [度]
  int angleActual = 0;  // Servo::updateOutput が反映後に書き戻し
};

class Servo : public IModule
{
public:
  // サーボPWM周期 — 後方互換のため static const も保持
  static const int PWM_CYCLE    = 33332;
  static const int CENTER       = 3090;
  static const int HANDLE_STEP  = 23;
  static const int MAX_ANGLE    = 40;

  // Config を注入してインスタンス生成
  explicit Servo(const ServoConfig& cfg);

  // MTU2チャンネル0 + P4_0 の初期化
  bool init() override;

  // Output : sys.srv.angleCmd を反映、sys.srv.angleActual に書き戻し
  void updateOutput(SystemData& sys) override;

  // 現在の設定角度を取得 (debug 用、Step 12 で SystemData 経由に移行予定)
  int getAngle() const { return angle_; }

private:
  // ステアリング角度を内部で設定
  // angle: -maxAngle(右)〜0(中央)〜+maxAngle(左) [度]
  void setAngle(int angle);

  // [-maxAngle, +maxAngle] にクランプ
  int clamp(int val) const;

  ServoConfig _config;
  int angle_;
};

extern Servo g_servo;

#endif /* DRIVERS_SERVO_H_ */
