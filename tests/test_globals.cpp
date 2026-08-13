/*
 * test_globals.cpp
 *
 *  ホスト側テスト用のグローバル実体定義。
 *
 *  ファーム側ではこれらの実体は src/mcr_camera_2026base.cpp が定義しているが、
 *  テストバイナリは main.cpp をリンクしない（VDC5 / SCIF2 / GIC 等の実機依存が
 *  丸ごと入ってしまうため）。そのためテスト対象ドライバが参照するグローバルを
 *  ここで定義して未定義参照を解消する。
 *
 *  Config はファームと同じ ProjectConfig.h の実値を使う。テスト側で別の値を
 *  作らないことで、「ProjectConfig.h を書き換えたらテストの期待値も追随する」
 *  状態を保つ。
 */

#include "core/ProjectConfig.h"
#include "drivers/Encoder.h"
#include "drivers/Motor.h"
#include "drivers/Servo.h"

// ModuleTimer (core/ModuleTimer.h) がタイムソースとして参照する 1ms 累積カウンタ。
// ファームでは OSTM0 割り込みが ++ するが、テストでは手動で進める。
volatile unsigned long g_timer_1ms = 0;

// EMA の共有データハブ。テストからは SystemData 経由で
// updateInput / updateOutput を呼ぶ。
SystemData g_sys;

// テスト対象ドライバのグローバル実体（Config 注入はファームと同一）
Motor   g_motor  (MOTOR_CONFIG);
Servo   g_servo  (SERVO_CONFIG);
Encoder g_encoder(ENCODER_CONFIG);

// SystemData を初期状態へ戻す（各テストの SetUp から呼ぶ）
void ResetSystemData()
{
  g_sys       = SystemData();
  g_timer_1ms = 0;
}
