/*
 * test_motor.cpp
 *
 *  Motor ドライバのユニットテスト
 *  モックレジスタを使い、PWMデューティ・方向ピン・クランプ動作を検証する
 */

#include <gtest/gtest.h>
#include "iodefine.h"        // モック版
#include "drivers/Motor.h"

// テスト用宣言
extern void ResetMockRegisters();

class MotorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    g_motor.init();
  }
};

// ============================================================
// init() 後のレジスタ状態
// ============================================================

TEST_F(MotorTest, InitSetsPwmCycle)
{
  // MTU2TGRA_3 に PWM周期がセットされている
  EXPECT_EQ(Motor::PWM_CYCLE, (int)mock_MTU2TGRA_3);
  EXPECT_EQ(Motor::PWM_CYCLE, (int)mock_MTU2TGRC_3);
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

// ============================================================
// クランプ動作
// ============================================================

TEST_F(MotorTest, ClampPositiveOver)
{
  g_motor.set(150, 0);
  EXPECT_EQ(Motor::MAX_POWER, g_motor.getLeft());
}

TEST_F(MotorTest, ClampNegativeOver)
{
  g_motor.set(-150, 0);
  EXPECT_EQ(-Motor::MAX_POWER, g_motor.getLeft());
}

TEST_F(MotorTest, ClampExact)
{
  g_motor.set(100, -100);
  EXPECT_EQ(100,  g_motor.getLeft());
  EXPECT_EQ(-100, g_motor.getRight());
}

// ============================================================
// 前進時のPWMデューティと方向ピン
// ============================================================

TEST_F(MotorTest, ForwardLeftDuty50)
{
  g_motor.set(50, 0);
  // 期待デューティ = (PWM_CYCLE - 1) * 50 / 100
  int expected = (Motor::PWM_CYCLE - 1) * 50 / 100;
  EXPECT_EQ(expected, (int)mock_MTU2TGRC_4);
  // 前進: P4_6 = 0
  EXPECT_EQ(0, (int)(mock_GPIOP4 & 0x0040));
}

TEST_F(MotorTest, ForwardRightDuty100)
{
  g_motor.set(0, 100);
  int expected = (Motor::PWM_CYCLE - 1) * 100 / 100;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_4);
  // 前進: P4_7 = 0
  EXPECT_EQ(0, (int)(mock_GPIOP4 & 0x0080));
}

// ============================================================
// 後退時のPWMデューティと方向ピン
// ============================================================

TEST_F(MotorTest, ReverseLeftDuty50)
{
  g_motor.set(-50, 0);
  int expected = (Motor::PWM_CYCLE - 1) * 50 / 100;
  EXPECT_EQ(expected, (int)mock_MTU2TGRC_4);
  // 後退: P4_6 = 1
  EXPECT_NE(0, (int)(mock_GPIOP4 & 0x0040));
}

TEST_F(MotorTest, ReverseRightDuty75)
{
  g_motor.set(0, -75);
  int expected = (Motor::PWM_CYCLE - 1) * 75 / 100;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_4);
  // 後退: P4_7 = 1
  EXPECT_NE(0, (int)(mock_GPIOP4 & 0x0080));
}

// ============================================================
// stop()
// ============================================================

TEST_F(MotorTest, StopZerosDuty)
{
  g_motor.set(80, 80);
  g_motor.stop();
  EXPECT_EQ(0, (int)mock_MTU2TGRC_4);
  EXPECT_EQ(0, (int)mock_MTU2TGRD_4);
  EXPECT_EQ(0, g_motor.getLeft());
  EXPECT_EQ(0, g_motor.getRight());
}

// ============================================================
// getLeft / getRight
// ============================================================

TEST_F(MotorTest, GettersReflectSetValue)
{
  g_motor.set(30, -70);
  EXPECT_EQ(30,  g_motor.getLeft());
  EXPECT_EQ(-70, g_motor.getRight());
}

// ============================================================
// 0入力
// ============================================================

TEST_F(MotorTest, ZeroDuty)
{
  g_motor.set(0, 0);
  EXPECT_EQ(0, (int)mock_MTU2TGRC_4);
  EXPECT_EQ(0, (int)mock_MTU2TGRD_4);
}
