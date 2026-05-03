/*
 * SystemData.h
 *
 *  全モジュールの入出力データを集約するシステム共有データ構造。
 *  Embedded-Module-Architecture (EMA) の SystemData に相当する。
 *
 *  3フェーズ実行モデルにおけるデータの流れ:
 *    Input  : ハードウェア → SystemData (各 IModule の updateInput が書く)
 *    Logic  : SystemData → SystemData (logic/RunLogic.cpp の関数群が判断)
 *    Output : SystemData → ハードウェア (各 IModule の updateOutput が読む)
 *
 *  各モジュールは「自身の Data 構造体だけ」読み書きするのが基本ルール。
 *  他モジュールのデータを参照するのは Logic フェーズの関数のみ。
 */

#ifndef CORE_SYSTEM_DATA_H_
#define CORE_SYSTEM_DATA_H_

#include <stdint.h>
#include "ModuleTimer.h"

// ====================================================================
// Camera
// ====================================================================
struct CameraData
{
    bool     frameReady;        // 新フレーム完了フラグ（メインループで消費後に false へ）
    uint32_t frameCount;        // 累積フレーム数（debug 表示用）
    int      field;             // 0=top / 1=bottom (debug 用)
    // 画素バッファ実体は Camera::imageBuffer_ にあり、ここはそれを指すポインタ。
    // 19KB のバッファを SystemData に持たせるとメモリ効率が悪いため。
    const volatile unsigned char* imageBufferPtr;
};

// ====================================================================
// LineDetector
// ====================================================================
struct LineDetectorData
{
    int           deviation[120];   // 各行の偏差（正=ライン左寄り、負=ライン右寄り）
    bool          crossLine;        // クロスライン検出フラグ
    bool          leftLine;         // 左ハーフライン検出フラグ
    bool          rightLine;        // 右ハーフライン検出フラグ
    bool          centerLine;       // センターライン検出フラグ
    unsigned char sensorBin;        // 8点センサ値
    int           detectRow;        // 直近で使用された検出行（debug 表示用）
};

// ====================================================================
// Motor
// ====================================================================
struct MotorData
{
    int leftCmd;       // Logic → Motor 指示値 [-100..+100]
    int rightCmd;
    int leftActual;    // Motor::updateOutput が反映後に書き戻し（debug 用）
    int rightActual;
};

// ====================================================================
// Servo
// ====================================================================
struct ServoData
{
    int angleCmd;      // Logic → Servo 指示角 [-MAX..+MAX] [度]
    int angleActual;   // Servo::updateOutput が反映後に書き戻し
};

// ====================================================================
// Onboard
// ====================================================================
struct OnboardData
{
    // Input
    int sw;              // 1=Pressed, 0=Released
    // Output
    int ledRedCmd;       // 0=OFF, 1=ON
    int ledGreenCmd;
    int ledBlueCmd;
    int userLedCmd;
};

// ====================================================================
// Encoder
// ====================================================================
struct EncoderData
{
    int   totalCount;     // 走行距離換算用累積カウント
    int   magaCount;      // クランク検出用累積カウント
    int   cnt;            // 直近 1ms 当たりカウント
    int   avgCnt;         // 移動平均カウント
    float filteredCnt;    // RC フィルタ後のカウント
};

// ====================================================================
// Serial（実体は薄いラッパなので debug 用フィールドのみ）
// ====================================================================
struct SerialData
{
    uint32_t txCount;     // 送信文字累計（任意・debug 用）
};

// ====================================================================
// SDLogger
// ====================================================================
struct SDLoggerData
{
    bool     ready;          // 初期化成功フラグ
    uint32_t logCount;       // 現在の記録エントリ数
    bool     full;           // バッファ満杯フラグ
    bool     saveRequested;  // メインループから「SD 書き出し」をリクエストするフラグ
    bool     saveDone;       // SD 書き出し完了フラグ
};

// ====================================================================
// Run（走行ロジックの実行状態）
//
// 旧 RunController クラスのインスタンス変数群をすべてここに集約する。
// applyDrivingPattern() は SystemData を読み書きする純関数となる。
// ====================================================================
struct RunData
{
    int         pattern;          // 現在の走行パターン番号（RunPattern を int 化）
    int         prevPattern;      // 直前のパターン（遷移検知用）
    int         handleVal;        // 直近のハンドル指示値（debug 表示用）
    int         traceOffset;      // ハンドル計算オフセット
    int         leftMotor;        // calcMotorVal() の結果
    int         rightMotor;
    int         leftBrake;        // calcBrakeMotorVal() の結果
    int         rightBrake;
    int         lastHandle101;    // FINISH 時のハンドル維持値
    bool        finished;         // 走行終了フラグ

    ModuleTimer stateTimer;       // 旧 stateMs_ : 現パターンに入ってからの経過時間
    ModuleTimer totalTimer;       // 旧 totalMs_ : 走行スタートからの経過時間
    ModuleTimer auxTimer;         // 旧 cnt1Ms_  : 補助タイマー（バー検知等）
};

// ====================================================================
// SystemData 集約
// ====================================================================
struct SystemData
{
    CameraData       cam;
    LineDetectorData line;
    MotorData        mot;
    ServoData        srv;
    OnboardData      ob;
    EncoderData      enc;
    SerialData       ser;
    SDLoggerData     sd;
    RunData          run;
};

// グローバル SystemData インスタンス（main.cpp で実体定義）
extern SystemData g_sys;

#endif /* CORE_SYSTEM_DATA_H_ */
