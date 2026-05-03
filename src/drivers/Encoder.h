/*
 * Encoder.h
 *
 *  エンコーダドライバ (MTU2 チャンネル1 位相計数モード2)
 *  EMA 準拠版: SystemData / Config ベース
 *  For GR-PEACH (RZ/A1H)
 *
 *  A相: TCLKA (P1_0)
 *  B相: TCLKB (P1_10)
 *
 *  Input : updateInput(sys) で MTU2TCNT_1 を読み取って各カウンタ・
 *          フィルタを更新し、結果を sys.enc.* に格納する。
 *
 *  移動平均: 直近 EncoderConfig.filterN 回分のカウントを平均
 *           (FILTER_N が配列上限)
 *  RCフィルタ: α = EncoderConfig.rcAlpha の指数移動平均
 *
 *  注意: Step 8 段階では main での init() / s_inputModules への登録は
 *  保留。SDLogger 連携が必要になる Step 12 以降で有効化予定。
 */

#ifndef DRIVERS_ENCODER_H_
#define DRIVERS_ENCODER_H_

#include "../core/IModule.h"

// ====================================================================
// EncoderConfig — エンコーダのフィルタ / ピン設定
// (実体は ProjectConfig.h の ENCODER_CONFIG で定義)
// ====================================================================
struct EncoderConfig
{
  int   filterN;     // 4   (移動平均段数)
  float rcAlpha;     // 0.5 (RC フィルタ係数)
  int   aPinFmt;     // P1_0  TCLKA
  int   bPinFmt;     // P1_10 TCLKB
};

// ====================================================================
// EncoderData — Encoder の出力データ
// SystemData::enc に集約される。Encoder クラスが「所有」する自身のデータ。
// ====================================================================
struct EncoderData
{
  int   totalCount;     // 走行距離換算用累積カウント
  int   magaCount;      // クランク検出用累積カウント
  int   cnt;            // 直近 1ms 当たりカウント
  int   avgCnt;         // 移動平均カウント
  float filteredCnt;    // RC フィルタ後のカウント
};

class Encoder : public IModule
{
public:
  // 移動平均フィルタ段数の上限 (配列サイズ)
  // 実際の段数は _config.filterN で指定（<= FILTER_N）
  static const int FILTER_N = 4;
  // 後方互換用 RC フィルタ係数 (実値は _config.rcAlpha)
  static constexpr float RC_ALPHA = 0.5f;

  // Config を注入してインスタンス生成
  explicit Encoder(const EncoderConfig& cfg);

  // MTU2チャンネル1 (位相計数モード) + P1_0, P1_10 の初期化
  bool init() override;

  // Input : 1ms 割り込みから呼び、MTU2TCNT_1 を読み取って sys.enc.* に格納
  void updateInput(SystemData& sys) override;

  // 累積カウントをリセット
  void clearTotal();

  // クランク検出用カウントをリセット
  void clearMaga();

  // 後方互換 getter (将来は sys.enc.* を直読推奨)
  int   getTotalCount() const { return totalCount_; }
  int   getMagaCount()  const { return magaCount_; }
  int   getCnt()        const { return (int)avg_; }
  float getFilteredCnt() const { return rc_; }

private:
  EncoderConfig _config;

  int   totalCount_;
  int   magaCount_;
  int   cnt_;

  int   filterBuf_[FILTER_N];
  int   filterIdx_;
  float avg_;

  float rc_;
};

extern Encoder g_encoder;

#endif /* DRIVERS_ENCODER_H_ */
