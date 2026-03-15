/*
 * test_encoder.cpp
 *
 *  Encoder ドライバのユニットテスト
 *  モックの MTU2TCNT_1 に値を書き込んで update() を呼び、
 *  カウント蓄積・フィルタリング・リセットを検証する
 */

#include <gtest/gtest.h>
#include <cmath>
#include "iodefine.h"        // モック版
#include "drivers/Encoder.h"

extern void ResetMockRegisters();

// テスト用: エンコーダインスタンスを毎回初期化する
class EncoderTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    // Encoder をリセットするためにコンストラクタ相当の初期化
    g_encoder = Encoder();
    g_encoder.init();
  }
};

// ============================================================
// 初期状態
// ============================================================

TEST_F(EncoderTest, InitialTotalCountIsZero)
{
  EXPECT_EQ(0, g_encoder.getTotalCount());
}

TEST_F(EncoderTest, InitialMagaCountIsZero)
{
  EXPECT_EQ(0, g_encoder.getMagaCount());
}

TEST_F(EncoderTest, InitialCntIsZero)
{
  EXPECT_EQ(0, g_encoder.getCnt());
}

// ============================================================
// update() — 基本的なカウント蓄積
// ============================================================

TEST_F(EncoderTest, UpdateAccumulatesTotalCount)
{
  // MTU2TCNT_1 = 10 で update() → totalCount = 10
  mock_MTU2TCNT_1 = 10;
  g_encoder.update();
  EXPECT_EQ(10, g_encoder.getTotalCount());

  // もう1回: totalCount = 20
  mock_MTU2TCNT_1 = 10;
  g_encoder.update();
  EXPECT_EQ(20, g_encoder.getTotalCount());
}

TEST_F(EncoderTest, UpdateResetsHardwareCounter)
{
  // update() が MTU2TCNT_1 を0にリセットする
  mock_MTU2TCNT_1 = 50;
  g_encoder.update();
  EXPECT_EQ(0, (int)mock_MTU2TCNT_1);
}

// ============================================================
// 負のカウント (後退)
// ============================================================

TEST_F(EncoderTest, NegativeCountSubtracts)
{
  // 符号付き16ビット: 0xFFF6 = -10 (2の補数)
  mock_MTU2TCNT_1 = (uint16_t)(-10);
  g_encoder.update();
  EXPECT_EQ(-10, g_encoder.getTotalCount());
}

// ============================================================
// clearTotal()
// ============================================================

TEST_F(EncoderTest, ClearTotalResetsCount)
{
  mock_MTU2TCNT_1 = 100;
  g_encoder.update();
  EXPECT_EQ(100, g_encoder.getTotalCount());

  g_encoder.clearTotal();
  EXPECT_EQ(0, g_encoder.getTotalCount());
}

// ============================================================
// clearMaga() — クランク用カウンタ独立リセット
// ============================================================

TEST_F(EncoderTest, ClearMagaDoesNotAffectTotal)
{
  mock_MTU2TCNT_1 = 30;
  g_encoder.update();

  g_encoder.clearMaga();

  // total は変わらない
  EXPECT_EQ(30, g_encoder.getTotalCount());
  // maga はリセット
  EXPECT_EQ(0,  g_encoder.getMagaCount());
}

// ============================================================
// 移動平均 (getCnt)
// ============================================================

TEST_F(EncoderTest, MovingAverageConvergesAfterFillBuffer)
{
  // FILTER_N 回同じ値を入れると平均はその値に一致する
  const int val = 20;
  for (int i = 0; i < Encoder::FILTER_N; i++)
  {
    mock_MTU2TCNT_1 = (uint16_t)val;
    g_encoder.update();
  }
  EXPECT_EQ(val, g_encoder.getCnt());
}

TEST_F(EncoderTest, MovingAverageOfMixedValues)
{
  // FILTER_N=4 として [10, 20, 30, 40] を入れると平均=25
  int values[] = {10, 20, 30, 40};
  for (int v : values)
  {
    mock_MTU2TCNT_1 = (uint16_t)v;
    g_encoder.update();
  }
  EXPECT_EQ(25, g_encoder.getCnt()); // (10+20+30+40)/4 = 25
}

// ============================================================
// RCフィルタ (getFilteredCnt)
// ============================================================

TEST_F(EncoderTest, RcFilterConvergesOnConstantInput)
{
  // 一定値を繰り返し入れるとRCフィルタも収束する
  const float val = 16.0f;
  for (int i = 0; i < 20; i++)
  {
    mock_MTU2TCNT_1 = (uint16_t)(int)val;
    g_encoder.update();
  }
  // 0.1 カウント以内に収束しているか検証
  EXPECT_NEAR(val, g_encoder.getFilteredCnt(), 0.1f);
}

TEST_F(EncoderTest, RcFilterInitialResponse)
{
  // 最初のupdate() 後: rc = alpha*0 + (1-alpha)*cnt = 0.5 * cnt
  mock_MTU2TCNT_1 = 10;
  g_encoder.update();
  float expected = Encoder::RC_ALPHA * 0.0f + (1.0f - Encoder::RC_ALPHA) * 10.0f;
  EXPECT_NEAR(expected, g_encoder.getFilteredCnt(), 0.01f);
}
