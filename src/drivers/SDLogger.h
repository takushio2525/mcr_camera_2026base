/*
 * SDLogger.h
 *
 *  SDカード ログ統一モジュール
 *  走行中のデータをバッファに記録し、走行終了後にCSVでSDカードに書き出す。
 *
 *  使い方:
 *    1. g_sdlogger.init()     → SDカード初期化 + FatFs マウント
 *    2. g_sdlogger.record()   → 走行中に毎ループ呼んでデータを記録
 *    3. g_sdlogger.saveToSD() → 走行終了後にCSVバッチ書き出し
 *
 *  ファイル形式:
 *    /data0000.csv, /data0001.csv, ... (renban.txt で連番管理)
 *
 *  参考: mcr_shiozawa_cclass_2.38m-s の SDLOG_T + CSV出力
 */

#ifndef DRIVERS_SDLOGGER_H_
#define DRIVERS_SDLOGGER_H_

#include "../core/IModule.h"
#include <stdint.h>

// ログバッファ最大エントリ数
#define SDLOG_MAX_ENTRIES 4000

// 画像データ幅（参考プロジェクト準拠）
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
  SDLogger();

  // SDカード初期化 + FatFs マウント
  bool init() override;

  // 周期処理（未使用）
  void updateOutput(SystemData& sys) override;

  // 初期化成功したか
  bool isReady() const { return mounted_; }

  // ---- ログ記録 ----

  // 現在のログエントリへの参照を取得して直接書き込む
  // 使用例: g_sdlogger.current().pattern = 11;
  //         g_sdlogger.current().hennsa = deviation;
  //         g_sdlogger.commit(); // エントリを確定して次へ進む
  SDLOG_T& current();

  // 現在のエントリを確定して記録番地を進める
  void commit();

  // 現在の記録数を取得
  unsigned int getLogCount() const { return logCount_; }

  // ログバッファをリセット
  void resetLog();

  // ログバッファが満杯か
  bool isFull() const { return logCount_ >= SDLOG_MAX_ENTRIES; }

  // ---- CSV書き出し ----

  // バッファ内のログデータをCSVファイルとしてSDカードに保存
  // 戻り値: 0=成功, 非0=エラー
  int saveToSD();

private:
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
