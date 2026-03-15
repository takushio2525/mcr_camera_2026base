/*
 * test_servo.cpp
 *
 *  Servo ドライバのユニットテスト
 *  モックレジスタを使い、角度→PWMカウント値の変換を検証する
 */

#include <gtest/gtest.h>
#include "iodefine.h"        // モック版
#include "drivers/Servo.h"

extern void ResetMockRegisters();

class ServoTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    g_servo.init();
  }
};

// ============================================================
// init() 後のレジスタ状態
// ============================================================

TEST_F(ServoTest, InitSetsCenterPosition)
{
  // 初期位置は中心 (CENTER カウント)
  EXPECT_EQ(Servo::CENTER, (int)mock_MTU2TGRD_0);
}

TEST_F(ServoTest, InitStartsTimer)
{
  // TCNT_0 スタートビットがセットされている
  EXPECT_NE(0, mock_MTU2TSTR & 0x01);
}

// ============================================================
// setAngle(0) — 中央
// ============================================================

TEST_F(ServoTest, AngleZeroIsCenter)
{
  g_servo.setAngle(0);
  EXPECT_EQ(Servo::CENTER, (int)mock_MTU2TGRD_0);
  EXPECT_EQ(0, g_servo.getAngle());
}

// ============================================================
// 正角度 (左) — カウント減少
// ============================================================

TEST_F(ServoTest, PositiveAngleDecreasesCount)
{
  g_servo.setAngle(10);
  int expected = Servo::CENTER - 10 * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
  EXPECT_EQ(10, g_servo.getAngle());
}

TEST_F(ServoTest, MaxLeftAngle)
{
  g_servo.setAngle(Servo::MAX_ANGLE);
  int expected = Servo::CENTER - Servo::MAX_ANGLE * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
}

// ============================================================
// 負角度 (右) — カウント増加
// ============================================================

TEST_F(ServoTest, NegativeAngleIncreasesCount)
{
  g_servo.setAngle(-10);
  int expected = Servo::CENTER - (-10) * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
  EXPECT_EQ(-10, g_servo.getAngle());
}

TEST_F(ServoTest, MaxRightAngle)
{
  g_servo.setAngle(-Servo::MAX_ANGLE);
  int expected = Servo::CENTER + Servo::MAX_ANGLE * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
}

// ============================================================
// クランプ動作
// ============================================================

TEST_F(ServoTest, ClampPositiveOver)
{
  g_servo.setAngle(Servo::MAX_ANGLE + 10);
  EXPECT_EQ(Servo::MAX_ANGLE, g_servo.getAngle());
  int expected = Servo::CENTER - Servo::MAX_ANGLE * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
}

TEST_F(ServoTest, ClampNegativeOver)
{
  g_servo.setAngle(-(Servo::MAX_ANGLE + 10));
  EXPECT_EQ(-Servo::MAX_ANGLE, g_servo.getAngle());
  int expected = Servo::CENTER + Servo::MAX_ANGLE * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
}

// ============================================================
// 連続呼び出し
// ============================================================

TEST_F(ServoTest, MultipleSetAngles)
{
  g_servo.setAngle(20);
  EXPECT_EQ(20, g_servo.getAngle());
  g_servo.setAngle(-5);
  EXPECT_EQ(-5, g_servo.getAngle());
  int expected = Servo::CENTER + 5 * Servo::HANDLE_STEP;
  EXPECT_EQ(expected, (int)mock_MTU2TGRD_0);
}
