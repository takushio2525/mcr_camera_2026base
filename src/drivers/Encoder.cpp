/*
 * Encoder.cpp
 *
 *  エンコーダドライバ実装
 *  EMA 準拠版: SystemData / Config ベース
 *  参考: mcr_shiozawa_cclass_2.38m-s/Encoder.cpp (Mishima, 2022)
 */

#include "Encoder.h"
#include "../core/SystemData.h"
#include "iodefine.h"
#include <typedefine.h>

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

Encoder::Encoder(const EncoderConfig& cfg)
  : _config(cfg),
    totalCount_(0), magaCount_(0), cnt_(0),
    filterIdx_(0), avg_(0.0f), rc_(0.0f)
{
  for (int i = 0; i < FILTER_N; i++)
  {
    filterBuf_[i] = 0;
  }
}

// ---------------------------------------------------------------------
// init() — MTU2 チャンネル1 位相計数モード2 + P1_0, P1_10 初期化
// ---------------------------------------------------------------------
bool Encoder::init()
{
  // ---- MTU2 スタンバイ解除 (STBCR3 bit3 = MTU2) ----
  CPGSTBCR3  &= ~0x08;

  // ---- P1_0 (TCLKA), P1_10 (TCLKB): 代替機能 (入力) ----
  GPIOPIBC1  &= ~0x0401;  // 入力バッファ無効
  GPIOPBDC1  &= ~0x0401;  // 双方向モード無効
  GPIOPM1    |=  0x0401;  // 入力モード
  GPIOPMC1   &= ~0x0401;  // GPIOモード (初期化のため一旦クリア)
  GPIOPIPC1  &= ~0x0401;  // ペリフェラル入力制御 クリア

  GPIOPBDC1  &= ~0x0401;
  GPIOPFC1   |=  0x0400;  // P1_10: 代替機能選択
  GPIOPFCE1  |=  0x0401;  // P1_0, P1_10: 代替機能選択

  GPIOPIPC1  |=  0x0401;  // ペリフェラル入力制御 有効
  GPIOPMC1   |=  0x0401;  // ペリフェラルモード (double)

  // ---- MTU2 チャンネル1: 位相計数モード2 ----
  MTU2TSTR   &= ~0x02;    // カウンタ停止
  MTU2TCR_1   = 0x14;     // 上: 両エッジ / 下: 外部カウンタ
  MTU2TMDR_1 |= 0x00;     // 位相計数モード2
  MTU2TIOR_1 |= 0x0A;     // 両エッジ設定
  MTU2TCNT_1  = 0x00;     // カウンタリセット
  MTU2TSTR   |= 0x02;     // カウンタスタート
  return true;
}

// ---------------------------------------------------------------------
// updateInput() — 1ms 割り込みから呼ぶ
// MTU2TCNT_1 を読み取ってリセットし、各カウンタ・フィルタを更新後、
// SystemData の sys.enc.* に結果を格納する。
// ---------------------------------------------------------------------
void Encoder::updateInput(SystemData& sys)
{
  // カウンタ値を取得してリセット
  cnt_         = (int)(short)MTU2TCNT_1; // 符号付きで解釈
  MTU2TCNT_1  = 0x0000;

  // 累積カウント更新
  totalCount_ += cnt_;
  magaCount_  += cnt_;

  // 移動平均フィルタの実効段数を _config から取得 (上限は FILTER_N)
  int n = _config.filterN;
  if (n > FILTER_N) n = FILTER_N;
  if (n < 1)        n = 1;

  filterBuf_[filterIdx_] = cnt_;
  filterIdx_++;
  if (filterIdx_ >= n)
  {
    filterIdx_ = 0;
  }
  float sum = 0.0f;
  for (int i = 0; i < n; i++)
  {
    sum += filterBuf_[i];
  }
  avg_ = sum / (float)n;

  // RCフィルタ (指数移動平均) 更新 — Config の rcAlpha を使用
  rc_ = _config.rcAlpha * rc_ + (1.0f - _config.rcAlpha) * (float)cnt_;

  // SystemData へ書き込み
  sys.enc.totalCount  = totalCount_;
  sys.enc.magaCount   = magaCount_;
  sys.enc.cnt         = cnt_;
  sys.enc.avgCnt      = (int)avg_;
  sys.enc.filteredCnt = rc_;
}

void Encoder::clearTotal()
{
  totalCount_ = 0;
}

void Encoder::clearMaga()
{
  magaCount_ = 0;
}
