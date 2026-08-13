/*
 * test_encoder.cpp
 *
 *  Encoder ドライバのユニットテスト
 *  モックの MTU2TCNT_1 に値を書き込んで updateInput() を呼び、
 *  カウント蓄積・フィルタリング・リセットを検証する
 *
 *  EMA 準拠版の API:
 *    update() は updateInput(SystemData&) に改名され、コンストラクタも
 *    explicit Encoder(const EncoderConfig&) のみになった。テスト間の状態
 *    リセットは ENCODER_CONFIG で作り直した実体を代入して行う。
 *    フィルタ係数は Encoder::RC_ALPHA ではなく ENCODER_CONFIG.rcAlpha を
 *    基準にする（実装が見ているのは _config 側のため）。
 */

#include <gtest/gtest.h>
#include <cmath>
#include "iodefine.h"        // モック版
#include "core/ProjectConfig.h"
#include "core/SystemData.h"
#include "drivers/Encoder.h"

extern void ResetMockRegisters();
extern void ResetSystemData();

// テスト用: エンコーダインスタンスを毎回初期化する
class EncoderTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ResetMockRegisters();
    ResetSystemData();
    // Encoder の内部状態をリセット（コンストラクタ相当の初期化）
    g_encoder = Encoder(ENCODER_CONFIG);
    g_encoder.init();
  }

  // ファーム本体（ISR の Input フェーズ）と同じ駆動経路。
  // ハードウェアカウンタに count を積んでから 1 周期分進める。
  void tick(int count)
  {
    mock_MTU2TCNT_1 = (uint16_t)count;
    g_encoder.updateInput(g_sys);
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

TEST_F(EncoderTest, InitStartsCounter)
{
  // カウンタスタートビット (bit1 = チャンネル1) がセットされている
  EXPECT_NE(0, mock_MTU2TSTR & 0x02);
}

TEST_F(EncoderTest, InitReturnsTrue)
{
  // Encoder::init() は失敗経路を持たない（常に成功）
  EXPECT_TRUE(g_encoder.init());
}

// ============================================================
// updateInput() — 基本的なカウント蓄積
// ============================================================

TEST_F(EncoderTest, UpdateAccumulatesTotalCount)
{
  // MTU2TCNT_1 = 10 で updateInput() → totalCount = 10
  tick(10);
  EXPECT_EQ(10, g_encoder.getTotalCount());

  // もう1回: totalCount = 20
  tick(10);
  EXPECT_EQ(20, g_encoder.getTotalCount());
}

TEST_F(EncoderTest, UpdateResetsHardwareCounter)
{
  // updateInput() が MTU2TCNT_1 を0にリセットする
  tick(50);
  EXPECT_EQ(0, (int)mock_MTU2TCNT_1);
}

// ============================================================
// 負のカウント (後退)
// ============================================================

TEST_F(EncoderTest, NegativeCountSubtracts)
{
  // 符号付き16ビット: 0xFFF6 = -10 (2の補数)
  tick(-10);
  EXPECT_EQ(-10, g_encoder.getTotalCount());
}

// ============================================================
// clearTotal()
// ============================================================

TEST_F(EncoderTest, ClearTotalResetsCount)
{
  tick(100);
  EXPECT_EQ(100, g_encoder.getTotalCount());

  g_encoder.clearTotal();
  EXPECT_EQ(0, g_encoder.getTotalCount());
}

// ============================================================
// clearMaga() — クランク用カウンタ独立リセット
// ============================================================

TEST_F(EncoderTest, ClearMagaDoesNotAffectTotal)
{
  tick(30);

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
  // filterN 回同じ値を入れると平均はその値に一致する
  const int val = 20;
  for (int i = 0; i < ENCODER_CONFIG.filterN; i++)
  {
    tick(val);
  }
  EXPECT_EQ(val, g_encoder.getCnt());
}

TEST_F(EncoderTest, MovingAverageOfMixedValues)
{
  // filterN=4 として [10, 20, 30, 40] を入れると平均=25
  ASSERT_EQ(4, ENCODER_CONFIG.filterN);
  int values[] = {10, 20, 30, 40};
  for (int v : values)
  {
    tick(v);
  }
  EXPECT_EQ(25, g_encoder.getCnt()); // (10+20+30+40)/4 = 25
}

// ============================================================
// RCフィルタ (getFilteredCnt)
// ============================================================

TEST_F(EncoderTest, RcFilterConvergesOnConstantInput)
{
  // 一定値を繰り返し入れるとRCフィルタも収束する
  const int val = 16;
  for (int i = 0; i < 20; i++)
  {
    tick(val);
  }
  // 0.1 カウント以内に収束しているか検証
  EXPECT_NEAR((float)val, g_encoder.getFilteredCnt(), 0.1f);
}

TEST_F(EncoderTest, RcFilterInitialResponse)
{
  // 最初の updateInput() 後: rc = alpha*0 + (1-alpha)*cnt
  tick(10);
  const float alpha    = ENCODER_CONFIG.rcAlpha;
  const float expected = alpha * 0.0f + (1.0f - alpha) * 10.0f;
  EXPECT_NEAR(expected, g_encoder.getFilteredCnt(), 0.01f);
}

// ============================================================
// SystemData への出力
// ============================================================

TEST_F(EncoderTest, WritesResultToSystemData)
{
  tick(12);

  EXPECT_EQ(12, g_sys.enc.cnt);
  EXPECT_EQ(12, g_sys.enc.totalCount);
  EXPECT_EQ(12, g_sys.enc.magaCount);
  // 移動平均は 1 周期目なので 12/filterN
  EXPECT_EQ(g_encoder.getCnt(), g_sys.enc.avgCnt);
  EXPECT_FLOAT_EQ(g_encoder.getFilteredCnt(), g_sys.enc.filteredCnt);
}

TEST_F(EncoderTest, SystemDataTracksAccumulation)
{
  tick(5);
  tick(7);
  EXPECT_EQ(7,  g_sys.enc.cnt);          // 直近 1 周期分
  EXPECT_EQ(12, g_sys.enc.totalCount);   // 累積
}
