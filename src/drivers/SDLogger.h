/*
 * SDLogger.h
 *
 *  SDカード ログ統一モジュール
 *  EMA 準拠版: SystemData / Config ベース
 *
 *  Output : updateOutput(sys) で
 *           - 走行中 (sys.run.pattern >= cfg.patternMinForLog かつ FINISH 以外)
 *             ならログエントリを自動追加
 *           - sys.sd.saveRequested == true なら saveToSD() を実行
 *           - sys.sd.ready/logCount/full/saveDone を毎周期更新
 *
 *  本モジュールは ISR ではなく **メインループから** 呼び出すこと
 *  (SD バス処理 + FatFs はタイミング制約があるため)。
 *
 *  ファイル形式: /data0000.csv, /data0001.csv, ... (renban.txt で連番管理)
 */

#ifndef DRIVERS_SDLOGGER_H_
#define DRIVERS_SDLOGGER_H_

#include "../core/IModule.h"
#include <stdint.h>

// ====================================================================
// SDLoggerConfig — SDLogger の動作設定
// (実体は ProjectConfig.h の SDLOGGER_CONFIG で定義)
// ====================================================================
struct SDLoggerConfig
{
  int maxEntries;       // 4000 (バッファサイズ)
  int imageWidth;       // 160  (画像 1 行記録時の幅)
  // 45  (画像 1 行記録時の行番号。e.hennsa もこの行の偏差を記録する)
  // 注意: かつては RUN_CONFIG.traceRow と同値だったが、現在は traceRow=48 で
  //       **一致していない**。走行制御が見ている行とログに残る偏差の行が
  //       別になっているので、ログ解析時は取り違えないこと。
  int recordImageRow;
  int patternMinForLog; // 11   (TRACE_NORMAL 以上で記録開始)
  int finishPattern;    // 101  (FINISH パターン番号 — 記録対象外)
};

// ====================================================================
// SDLoggerData — SDLogger の入出力データ
// SystemData::sd に集約される。SDLogger クラスが「所有」する自身のデータ。
// ====================================================================
struct SDLoggerData
{
  bool     ready;          // 初期化成功フラグ
  uint32_t logCount;       // 現在の記録エントリ数
  bool     full;           // バッファ満杯フラグ
  bool     saveRequested;  // メインループから「SD 書き出し」をリクエストするフラグ
  bool     saveDone;       // SD 書き出し完了フラグ
};

// ログバッファ最大エントリ数 (SDLoggerConfig.maxEntries の上限。配列サイズ用)
#define SDLOG_MAX_ENTRIES 4000

// 画像データ幅（参考プロジェクト準拠 / SDLoggerConfig.imageWidth の上限）
#define SDLOG_IMAGE_WIDTH 160

// ログ1エントリの構造体（参考プロジェクトの SDLOG_T 準拠）
typedef struct {
  unsigned int cnt_msdwritetime;    // タイムスタンプ (ms)
  unsigned int pattern;             // 走行パターン番号
  unsigned int convertBCD;          // センサBCD値
  signed int handle;                // ハンドル角度
  signed int hennsa;                // 偏差値
  unsigned int encoder;             // エンコーダカウント
  signed int motorL;                // 左モーター出力
  signed int motorR;                // 右モーター出力
  signed int flagL;                 // 左ラインフラグ
  signed int flagK;                 // クロスラインフラグ
  signed int flagR;                 // 右ラインフラグ
  signed int flagS;                 // スタートラインフラグ
  signed int total;                 // 累計値
  signed int imageData0[SDLOG_IMAGE_WIDTH]; // 画像生データ (1行分)
} SDLOG_T;

class SDLogger : public IModule
{
public:
  // Config を注入してインスタンス生成
  explicit SDLogger(const SDLoggerConfig& cfg);

  // SDカード初期化 + FatFs マウント
  bool init() override;

  // Output : 走行中ログ自動記録 + sys.sd.saveRequested で保存
  void updateOutput(SystemData& sys) override;

  // 初期化成功したか
  bool isReady() const { return mounted_; }

  // ---- ログ記録 (内部 / 後方互換用) ----

  // 現在のログエントリへの参照を取得（updateOutput 内部で使用）
  SDLOG_T& current();

  // 現在のエントリを確定して記録番地を進める
  void commit();

  // 現在の記録数を取得
  unsigned int getLogCount() const { return logCount_; }

  // ログバッファをリセット
  void resetLog();

  // ログバッファが満杯か
  bool isFull() const { return logCount_ >= (unsigned int)_config.maxEntries; }

  // ---- CSV書き出し ----

  // バッファ内のログデータをCSVファイルとしてSDカードに保存
  // 戻り値: 0=成功, 非0=エラー
  int saveToSD();

private:
  SDLoggerConfig _config;
  bool mounted_;              // FatFs マウント済みフラグ
  unsigned int logCount_;     // 記録済みエントリ数

  // 連番管理
  int readRenban();
  void writeRenban(int num);
};

extern SDLogger g_sdlogger;

// ログバッファ（大容量のため extern で共有）
extern SDLOG_T g_logData[SDLOG_MAX_ENTRIES];

#endif /* DRIVERS_SDLOGGER_H_ */
