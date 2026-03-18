/*
 * SDLogger.cpp
 *
 *  SDカード ログ統一モジュール
 *  走行データをバッファに記録し、走行終了後にCSVでバッチ書き出しする。
 *
 *  参考: mcr_shiozawa_cclass_2.38m-s のログ保存処理
 */

#include "SDLogger.h"
#include "SDCard.h"
#include "Serial.h"
#include "fatfs/ff.h"
#include <stdio.h>
#include <string.h>

// グローバルインスタンス
SDLogger g_sdlogger;

// ログバッファ（約 2.7MB: 680bytes × 4000エントリ）
// SDLOG_T の imageData0[160] = 640bytes + その他フィールド = 約680bytes
SDLOG_T g_logData[SDLOG_MAX_ENTRIES];

// FatFs ワークエリア
static FATFS fatfs;

// CSV書き出し用バッファ
static char csvBuf[512];

SDLogger::SDLogger()
  : mounted_(false)
  , logCount_(0)
{
}

void SDLogger::init()
{
  g_serial.printf("SDLogger: 初期化開始...\n");

  // SDカードドライバ初期化
  g_sdcard.init();

  if (!g_sdcard.isReady()) {
    g_serial.printf("SDLogger: SDカード初期化失敗\n");
    return;
  }

  // FatFs マウント
  FRESULT res = f_mount(&fatfs, "", 1);  // 即時マウント
  if (res != FR_OK) {
    g_serial.printf("SDLogger: マウント失敗 (err=%d)\n", (int)res);
    return;
  }

  mounted_ = true;
  g_serial.printf("SDLogger: マウント完了\n");

  // ログバッファをクリア
  resetLog();
}

void SDLogger::update()
{
  // 未使用
}

SDLOG_T& SDLogger::current()
{
  // バッファが満杯の場合は最後のエントリを返す（上書き防止）
  if (logCount_ >= SDLOG_MAX_ENTRIES) {
    return g_logData[SDLOG_MAX_ENTRIES - 1];
  }
  return g_logData[logCount_];
}

void SDLogger::commit()
{
  if (logCount_ < SDLOG_MAX_ENTRIES) {
    logCount_++;
  }
}

void SDLogger::resetLog()
{
  logCount_ = 0;
  memset(g_logData, 0, sizeof(g_logData));
}

// ---- CSV 書き出し ----
int SDLogger::saveToSD()
{
  if (!mounted_) {
    g_serial.printf("SDLogger: 未マウント\n");
    return -1;
  }

  if (logCount_ == 0) {
    g_serial.printf("SDLogger: 記録データなし\n");
    return -2;
  }

  g_serial.printf("SDLogger: 保存開始 (%u エントリ)...\n", logCount_);

  // 連番を取得
  int fileNum = readRenban();
  if (fileNum < 0 || fileNum > 9999) {
    fileNum = 0;
  }

  // 連番を更新
  writeRenban(fileNum + 1);

  // CSVファイルを作成
  char filename[32];
  snprintf(filename, sizeof(filename), "data%04d.csv", fileNum);

  FIL fil;
  FRESULT res = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
  if (res != FR_OK) {
    g_serial.printf("SDLogger: ファイルオープン失敗 (%s, err=%d)\n",
                    filename, (int)res);
    return -3;
  }

  // ヘッダ行
  f_printf(&fil, "Log %d\n", fileNum);

  // データ行
  UINT bw;
  for (unsigned int i = 0; i < logCount_; i++) {
    SDLOG_T *log = &g_logData[i];

    // 基本フィールド (13項目 + インデックス)
    int len = snprintf(csvBuf, sizeof(csvBuf),
                       "%u,%u,%u,%d,%d,%u,%d,%d,%d,%d,%d,%d,%d,%u",
                       log->cnt_msdwritetime,
                       log->pattern,
                       log->convertBCD,
                       log->handle,
                       log->hennsa,
                       log->encoder,
                       log->motorL,
                       log->motorR,
                       log->flagL,
                       log->flagK,
                       log->flagR,
                       log->flagS,
                       log->total,
                       i);
    f_write(&fil, csvBuf, len, &bw);

    // 画像データ (160ピクセル)
    for (int t = 0; t < SDLOG_IMAGE_WIDTH; t++) {
      len = snprintf(csvBuf, sizeof(csvBuf), "%d,", log->imageData0[t]);
      f_write(&fil, csvBuf, len, &bw);
    }

    f_write(&fil, "\n", 1, &bw);

    // 進捗表示 (500エントリごと)
    if ((i + 1) % 500 == 0) {
      g_serial.printf("SDLogger: %u/%u 書き込み中...\n", i + 1, logCount_);
    }
  }

  // ファイルクローズ
  res = f_close(&fil);
  if (res != FR_OK) {
    g_serial.printf("SDLogger: クローズ失敗 (err=%d)\n", (int)res);
    return -4;
  }

  g_serial.printf("SDLogger: 保存完了 → %s\n", filename);
  return 0;
}

// ---- 連番管理 ----
int SDLogger::readRenban()
{
  FIL fil;
  if (f_open(&fil, "renban.txt", FA_READ) != FR_OK) {
    return 0;  // ファイルなし → 0から開始
  }

  char buf[16];
  UINT br;
  f_read(&fil, buf, sizeof(buf) - 1, &br);
  buf[br] = '\0';
  f_close(&fil);

  int num = 0;
  // 簡易 atoi
  for (int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++) {
    num = num * 10 + (buf[i] - '0');
  }
  return num;
}

void SDLogger::writeRenban(int num)
{
  FIL fil;
  if (f_open(&fil, "renban.txt", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
    return;
  }

  char buf[16];
  int len = snprintf(buf, sizeof(buf), "%d", num);
  UINT bw;
  f_write(&fil, buf, len, &bw);
  f_close(&fil);
}
