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
  // Motor::init() / Servo::init() は同じ操作を `&= 0xf7` と書いている
  // （0xf7 = ~0x08。表記が違うだけで意味は同じ）。
  CPGSTBCR3  &= ~0x08;

  // ---- P1_0 (TCLKA), P1_10 (TCLKB): 代替機能 (入力) ----
  // 0x0401 = bit10|bit0 → P1_10, P1_0（ポート内のピン番号 = ビット番号）
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
  MTU2TSTR   &= ~0x02;    // カウンタ停止 (bit1 = チャンネル1)
  MTU2TCR_1   = 0x14;     // 上: 両エッジ / 下: 外部カウンタ
  // TODO(要実機確認): `|= 0x00` はどのビットも変えない no-op であり
  //   （これは式として静的に確定する事実）、この行では動作モードが
  //   一切設定されていない。つまり「位相計数モード2」というコメントの
  //   主張はこの行の内容と一致しない。リセット後の TMDR_1 が初期値
  //   （ノーマル動作）のままなら位相計数は有効になっていないことになる。
  //   現状 Encoder は main から init() されておらず ISR にも未登録なので
  //   症状は表面化していない。有効化するときに、A/B 相を入れて
  //   MTU2TCNT_1 が実際に増減するかを実機で確認し、必要なら
  //   RZ/A1H ハードウェアマニュアルの MTU2 TMDR を見て正しい値を書くこと。
  MTU2TMDR_1 |= 0x00;     // 位相計数モード2
  // 0x0A は下位ニブル (IOA 相当) を立てる値。参考プロジェクトで「両エッジ」と
  // されている設定だが、ビット定義は mtu2_iodefine.h に無いため要マニュアル確認。
  MTU2TIOR_1 |= 0x0A;     // 両エッジ設定
  MTU2TCNT_1  = 0x00;     // カウンタリセット
  MTU2TSTR   |= 0x02;     // カウンタスタート (bit1 = チャンネル1)
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
