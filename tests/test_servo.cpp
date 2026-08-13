/*
 * test_servo.cpp
 *
 *  Servo ドライバのユニットテスト
 *  モックレジスタを使い、角度→PWMカウント値の変換を検証する
 *
 *  EMA 準拠版の API:
 *    setAngle() は private に降格したため、テストもファーム本体と同じく
 *    SystemData 経由（sys.srv.angleCmd → updateOutput → angleActual）で
 *    駆動する。期待値は Servo::CENTER 等の後方互換 static const ではなく
 *    SERVO_CONFIG を基準にする（実装はすべて _config を見ているため）。
 */

#include <gtest/gtest.h>
#include "iodefine.h"        // モック版
#include "core/ProjectConfig.h"
#include "core/SystemData.h"
#include "drivers/Servo.h"

extern void ResetMockRegisters();
extern void ResetSystemData();

class ServoTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    ResetSystemData();
    g_servo.init();
  }

  // ファーム本体（ISR の Output フェーズ）と同じ駆動経路
  void drive(int angle)
  {
    g_sys.srv.angleCmd = angle;
    g_servo.updateOutput(g_sys);
  }

  // setAngle() のカウント変換式（正角度 → カウント減少 → 左）
  static int expectedCount(int angle)
  {
    return SERVO_CONFIG.center - angle * SERVO_CONFIG.handleStep;
  }
};

// ============================================================
// init() 後のレジスタ状態
// ============================================================

TEST_F(ServoTest, InitSetsCenterPosition)
{
  // 初期位置は中心 (SERVO_CONFIG.center カウント)
  EXPECT_EQ(SERVO_CONFIG.center, (int)mock_MTU2TGRD_0);
  EXPECT_EQ(SERVO_CONFIG.center, (int)mock_MTU2TGRB_0);
}

TEST_F(ServoTest, InitSetsPulseRisePosition)
{
  // パルス立ち上がり位置は 2 カウント固定
  EXPECT_EQ(2, (int)mock_MTU2TGRA_0);
  EXPECT_EQ(2, (int)mock_MTU2TGRC_0);
}

TEST_F(ServoTest, InitStartsTimer)
{
  // TCNT_0 スタートビットがセットされている
  EXPECT_NE(0, mock_MTU2TSTR & 0x01);
}

TEST_F(ServoTest, InitReturnsTrue)
{
  // Servo::init() は失敗経路を持たない（常に成功）
  EXPECT_TRUE(g_servo.init());
}

// ============================================================
// 角度 0 — 中央
// ============================================================

TEST_F(ServoTest, AngleZeroIsCenter)
{
  drive(0);
  EXPECT_EQ(SERVO_CONFIG.center, (int)mock_MTU2TGRD_0);
  EXPECT_EQ(0, g_servo.getAngle());
}

// ============================================================
// 正角度 (左) — カウント減少
// ============================================================

TEST_F(ServoTest, PositiveAngleDecreasesCount)
{
  drive(10);
  EXPECT_EQ(expectedCount(10), (int)mock_MTU2TGRD_0);
  EXPECT_LT((int)mock_MTU2TGRD_0, SERVO_CONFIG.center);
  EXPECT_EQ(10, g_servo.getAngle());
}

TEST_F(ServoTest, MaxLeftAngle)
{
  drive(SERVO_CONFIG.maxAngle);
  EXPECT_EQ(expectedCount(SERVO_CONFIG.maxAngle), (int)mock_MTU2TGRD_0);
}

// ============================================================
// 負角度 (右) — カウント増加
// ============================================================

TEST_F(ServoTest, NegativeAngleIncreasesCount)
{
  drive(-10);
  EXPECT_EQ(expectedCount(-10), (int)mock_MTU2TGRD_0);
  EXPECT_GT((int)mock_MTU2TGRD_0, SERVO_CONFIG.center);
  EXPECT_EQ(-10, g_servo.getAngle());
}

TEST_F(ServoTest, MaxRightAngle)
{
  drive(-SERVO_CONFIG.maxAngle);
  EXPECT_EQ(expectedCount(-SERVO_CONFIG.maxAngle), (int)mock_MTU2TGRD_0);
}

// ============================================================
// クランプ動作
// ============================================================

TEST_F(ServoTest, ClampPositiveOver)
{
  drive(SERVO_CONFIG.maxAngle + 10);
  EXPECT_EQ(SERVO_CONFIG.maxAngle, g_servo.getAngle());
  EXPECT_EQ(expectedCount(SERVO_CONFIG.maxAngle), (int)mock_MTU2TGRD_0);
}

TEST_F(ServoTest, ClampNegativeOver)
{
  drive(-(SERVO_CONFIG.maxAngle + 10));
  EXPECT_EQ(-SERVO_CONFIG.maxAngle, g_servo.getAngle());
  EXPECT_EQ(expectedCount(-SERVO_CONFIG.maxAngle), (int)mock_MTU2TGRD_0);
}

// ============================================================
// 連続呼び出し
// ============================================================

TEST_F(ServoTest, MultipleAngles)
{
  drive(20);
  EXPECT_EQ(20, g_servo.getAngle());
  drive(-5);
  EXPECT_EQ(-5, g_servo.getAngle());
  EXPECT_EQ(expectedCount(-5), (int)mock_MTU2TGRD_0);
}

// ============================================================
// SystemData への書き戻し
// ============================================================

TEST_F(ServoTest, WritesBackActualToSystemData)
{
  drive(15);
  EXPECT_EQ(15, g_sys.srv.angleActual);
}

TEST_F(ServoTest, WritesBackClampedValue)
{
  // 書き戻されるのは指示角そのものではなくクランプ後の実効角
  drive(SERVO_CONFIG.maxAngle + 25);
  EXPECT_EQ(SERVO_CONFIG.maxAngle, g_sys.srv.angleActual);
}
