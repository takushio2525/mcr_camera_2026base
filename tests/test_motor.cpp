/*
 * test_motor.cpp
 *
 *  Motor ドライバのユニットテスト
 *  モックレジスタを使い、PWMデューティ・方向ピン・クランプ動作を検証する
 *
 *  EMA 準拠版の API:
 *    set() / stop() は private に降格したため、テストもファーム本体と同じく
 *    SystemData 経由（sys.mot.leftCmd/rightCmd → updateOutput → *Actual）で
 *    駆動する。期待値は Motor::PWM_CYCLE / MAX_POWER ではなく MOTOR_CONFIG を
 *    基準にする（クランプ・デューティ計算はすべて _config を見ているため。
 *    Motor::MAX_POWER=100 は MOTOR_CONFIG.maxPower=70 と一致していない）。
 */

#include <gtest/gtest.h>
#include "iodefine.h"        // モック版
#include "core/ProjectConfig.h"
#include "core/SystemData.h"
#include "drivers/Motor.h"

// テスト用宣言（tests/mock/mock_registers.cpp, tests/test_globals.cpp）
extern void ResetMockRegisters();
extern void ResetSystemData();

class MotorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    ResetSystemData();
    g_motor.init();
  }

  // ファーム本体（ISR の Output フェーズ）と同じ駆動経路
  void drive(int left, int right)
  {
    g_sys.mot.leftCmd  = left;
    g_sys.mot.rightCmd = right;
    g_motor.updateOutput(g_sys);
  }

  // applyLeft / applyRight のデューティ計算式
  static int expectedDuty(int absPower)
  {
    return (int)((long)(MOTOR_CONFIG.pwmCycle - 1) * absPower / 100);
  }
};

// ============================================================
// init() 後のレジスタ状態
// ============================================================

TEST_F(MotorTest, InitSetsPwmCycle)
{
  // MTU2TGRA_3 / TGRC_3 に PWM周期がセットされている
  EXPECT_EQ(MOTOR_CONFIG.pwmCycle, (int)mock_MTU2TGRA_3);
  EXPECT_EQ(MOTOR_CONFIG.pwmCycle, (int)mock_MTU2TGRC_3);
}

TEST_F(MotorTest, InitZerosDuty)
{
  // 初期デューティは0
  EXPECT_EQ(0, (int)mock_MTU2TGRC_4);
  EXPECT_EQ(0, (int)mock_MTU2TGRD_4);
}

TEST_F(MotorTest, InitStartsTimer)
{
  // TCNT_4 スタートビットがセットされている
  EXPECT_NE(0, mock_MTU2TSTR & 0x40);
}

TEST_F(MotorTest, InitReturnsTrue)
{
  // Motor::init() は失敗経路を持たない（常に成功）
  EXPECT_TRUE(g_motor.init());
}

// ============================================================
// クランプ動作（上限は MOTOR_CONFIG.maxPower）
// ============================================================

TEST_F(MotorTest, ClampPositiveOver)
{
  drive(MOTOR_CONFIG.maxPower + 80, 0);
  EXPECT_EQ(MOTOR_CONFIG.maxPower, g_motor.getLeft());
}

TEST_F(MotorTest, ClampNegativeOver)
{
  drive(-(MOTOR_CONFIG.maxPower + 80), 0);
  EXPECT_EQ(-MOTOR_CONFIG.maxPower, g_motor.getLeft());
}

TEST_F(MotorTest, ClampExact)
{
  // 上限ちょうどはクランプされない
  drive(MOTOR_CONFIG.maxPower, -MOTOR_CONFIG.maxPower);
  EXPECT_EQ( MOTOR_CONFIG.maxPower, g_motor.getLeft());
  EXPECT_EQ(-MOTOR_CONFIG.maxPower, g_motor.getRight());
}

// ============================================================
// 前進時のPWMデューティと方向ピン
//
// 左モーターは配線/ギアの都合で正転=後退のため、ドライバ側で
// 方向ピン (P4_6) の極性を反転している（前進で 1、後退で 0）。
// 右モーター (P4_7) は反転なし（前進で 0、後退で 1）。
// ============================================================

TEST_F(MotorTest, ForwardLeftDuty50)
{
  drive(50, 0);
  EXPECT_EQ(expectedDuty(50), (int)mock_MTU2TGRC_4);
  // 前進: P4_6 = 1（極性反転）
  EXPECT_NE(0, (int)(mock_GPIOP4 & 0x0040));
}

TEST_F(MotorTest, ForwardRightDutyMax)
{
  drive(0, MOTOR_CONFIG.maxPower);
  EXPECT_EQ(expectedDuty(MOTOR_CONFIG.maxPower), (int)mock_MTU2TGRD_4);
  // 前進: P4_7 = 0
  EXPECT_EQ(0, (int)(mock_GPIOP4 & 0x0080));
}

// ============================================================
// 後退時のPWMデューティと方向ピン
// ============================================================

TEST_F(MotorTest, ReverseLeftDuty50)
{
  drive(-50, 0);
  EXPECT_EQ(expectedDuty(50), (int)mock_MTU2TGRC_4);
  // 後退: P4_6 = 0（極性反転）
  EXPECT_EQ(0, (int)(mock_GPIOP4 & 0x0040));
}

TEST_F(MotorTest, ReverseRightDuty60)
{
  drive(0, -60);
  EXPECT_EQ(expectedDuty(60), (int)mock_MTU2TGRD_4);
  // 後退: P4_7 = 1
  EXPECT_NE(0, (int)(mock_GPIOP4 & 0x0080));
}

// ============================================================
// 指示値 0（停止）
// ============================================================

TEST_F(MotorTest, ZeroCmdZerosDuty)
{
  drive(MOTOR_CONFIG.maxPower, MOTOR_CONFIG.maxPower);
  drive(0, 0);
  EXPECT_EQ(0, (int)mock_MTU2TGRC_4);
  EXPECT_EQ(0, (int)mock_MTU2TGRD_4);
  EXPECT_EQ(0, g_motor.getLeft());
  EXPECT_EQ(0, g_motor.getRight());
}

// ============================================================
// SystemData への書き戻し
// ============================================================

TEST_F(MotorTest, WritesBackActualToSystemData)
{
  drive(30, -60);
  EXPECT_EQ( 30, g_sys.mot.leftActual);
  EXPECT_EQ(-60, g_sys.mot.rightActual);
  // getter とも一致する
  EXPECT_EQ( 30, g_motor.getLeft());
  EXPECT_EQ(-60, g_motor.getRight());
}

TEST_F(MotorTest, WritesBackClampedValue)
{
  // 書き戻されるのは指示値そのものではなくクランプ後の実効値
  drive(MOTOR_CONFIG.maxPower + 30, 0);
  EXPECT_EQ(MOTOR_CONFIG.maxPower, g_sys.mot.leftActual);
}
