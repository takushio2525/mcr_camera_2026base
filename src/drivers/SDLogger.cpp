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
#include "../core/SystemData.h"
#include "../core/ModuleTimer.h"   // g_timer_1ms 参照用
#include "fatfs/ff.h"
#include <stdio.h>
#include <string.h>

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

// ログバッファ（約 2.7MB: 680bytes × 4000エントリ）
SDLOG_T g_logData[SDLOG_MAX_ENTRIES];

// FatFs ワークエリア
static FATFS fatfs;

// CSV書き出し用バッファ
static char csvBuf[512];

SDLogger::SDLogger(const SDLoggerConfig& cfg)
  : _config(cfg)
  , mounted_(false)
  , logCount_(0)
{
}

bool SDLogger::init()
{
  g_serial.printf("SDLogger: 初期化開始...\n");

  // SDカードドライバ初期化
  g_sdcard.init();

  if (!g_sdcard.isReady()) {
    g_serial.printf("SDLogger: SDカード初期化失敗\n");
    return false;
  }

  // FatFs マウント
  FRESULT res = f_mount(&fatfs, "", 1);  // 即時マウント
  if (res != FR_OK) {
    g_serial.printf("SDLogger: マウント失敗 (err=%d)\n", (int)res);
    return false;
  }

  mounted_ = true;
  g_serial.printf("SDLogger: マウント完了\n");

  // ログバッファをクリア
  resetLog();
  return true;
}

// ====================================================================
// updateOutput() — メインループから毎周期呼ばれる Output 処理
// 1. sys.sd.* に現在の状態を反映
// 2. 走行中ならログエントリを自動追加
// 3. sys.sd.saveRequested == true なら saveToSD() 実行
// ====================================================================
void SDLogger::updateOutput(SystemData& sys)
{
  // 状態反映
  sys.sd.ready    = mounted_;
  sys.sd.logCount = logCount_;
  sys.sd.full     = isFull();

  // ---- 走行中ログ自動記録 ----
  // pattern が patternMinForLog 以上 (TRACE_NORMAL 以降) かつ FINISH 以外
  int pat = sys.run.pattern;
  if (mounted_
      && !isFull()
      && pat >= _config.patternMinForLog
      && pat != _config.finishPattern)
  {
    SDLOG_T& e = current();
    e.cnt_msdwritetime = (unsigned int)g_timer_1ms;
    e.pattern          = (unsigned int)pat;
    e.convertBCD       = (unsigned int)sys.line.sensorBin;
    e.handle           = sys.run.handleVal;
    e.hennsa           = sys.line.deviation[_config.recordImageRow];
    e.encoder          = (unsigned int)sys.enc.totalCount;
    e.motorL           = sys.mot.leftActual;
    e.motorR           = sys.mot.rightActual;
    e.flagL            = sys.line.leftLine   ? 1 : 0;
    e.flagK            = sys.line.crossLine  ? 1 : 0;
    e.flagR            = sys.line.rightLine  ? 1 : 0;
    e.flagS            = sys.line.centerLine ? 1 : 0;
    e.total            = (signed int)g_timer_1ms;

    // 画像データ (1行分) を sys.cam.imageBufferPtr 経由で取得
    int width = (_config.imageWidth <= SDLOG_IMAGE_WIDTH)
                ? _config.imageWidth : SDLOG_IMAGE_WIDTH;
    if (sys.cam.imageBufferPtr)
    {
      const volatile unsigned char* img = sys.cam.imageBufferPtr;
      int row = _config.recordImageRow;
      for (int i = 0; i < width; i++)
      {
        e.imageData0[i] = img[row * SDLOG_IMAGE_WIDTH + i];
      }
    }
    commit();
  }

  // ---- 保存リクエスト処理 ----
  if (sys.sd.saveRequested && !sys.sd.saveDone)
  {
    sys.sd.saveRequested = false;
    int rc = saveToSD();
    if (rc == 0)
    {
      sys.sd.saveDone = true;
    }
  }
}

SDLOG_T& SDLogger::current()
{
  // バッファが満杯の場合は最後のエントリを返す（上書き防止）
  unsigned int cap = (unsigned int)_config.maxEntries;
  if (cap > SDLOG_MAX_ENTRIES) cap = SDLOG_MAX_ENTRIES;
  if (logCount_ >= cap) {
    return g_logData[cap - 1];
  }
  return g_logData[logCount_];
}

void SDLogger::commit()
{
  unsigned int cap = (unsigned int)_config.maxEntries;
  if (cap > SDLOG_MAX_ENTRIES) cap = SDLOG_MAX_ENTRIES;
  if (logCount_ < cap) {
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
