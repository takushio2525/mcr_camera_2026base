/*
 * Encoder.h
 *
 *  エンコーダドライバ (MTU2 チャンネル1 位相計数モード2)
 *  For GR-PEACH (RZ/A1H)
 *
 *  A相: TCLKA (P1_0)
 *  B相: TCLKB (P1_10)
 *
 *  update() を 1ms 割り込みから呼ぶことで、10ms 平均の速度と
 *  累積カウント (走行距離) を取得できる。
 *
 *  移動平均: 直近 FILTER_N 回分のカウントを平均
 *  RCフィルタ: α=0.5 の指数移動平均
 */

#ifndef DRIVERS_ENCODER_H_
#define DRIVERS_ENCODER_H_

#include "../core/IModule.h"

class Encoder : public IModule
{
public:
  // 移動平均フィルタ段数
  static const int FILTER_N = 4;
  // RCフィルタ係数 (0.0〜1.0)
  static constexpr float RC_ALPHA = 0.5f;

  Encoder();

  // MTU2チャンネル1 (位相計数モード) + P1_0, P1_10 の初期化
  void init() override;

  // 1ms 割り込みから呼ぶカウント更新処理
  // MTU2TCNT_1 を読み取ってリセットし、各カウンタを更新する
  void update() override;

  // 累積カウントをリセット
  void clearTotal();

  // クランク検出用カウントをリセット
  void clearMaga();

  // 累積カウント取得 (走行距離換算に使用)
  int getTotalCount() const { return totalCount_; }

  // クランク検出用累積カウント取得
  int getMagaCount() const { return magaCount_; }

  // 直近 FILTER_N 回の移動平均カウントを取得 (速度フィードバック)
  int getCnt() const { return (int)avg_; }

  // RCフィルタ後の速度カウントを取得
  float getFilteredCnt() const { return rc_; }

private:
  // 累積カウント
  int   totalCount_;
  // クランク用カウント
  int   magaCount_;

  // 今回の 1ms カウント値
  int   cnt_;

  // 移動平均フィルタ
  int   filterBuf_[FILTER_N];
  int   filterIdx_;
  float avg_;

  // RCフィルタ (指数移動平均)
  float rc_;
};

extern Encoder g_encoder;

#endif /* DRIVERS_ENCODER_H_ */
