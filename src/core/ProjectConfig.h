/*
 * ProjectConfig.h
 *
 *  プロジェクト全体の Config インスタンス一括定義ハブ。
 *  Embedded-Module-Architecture (EMA) の ProjectConfig.h に相当する。
 *
 *  各 Config 構造体宣言は対応するモジュールヘッダ (drivers/Camera.h 等) に
 *  あり、本ファイルではそれらを include して **値の実体を 1 箇所で管理する**。
 *
 *  値を変更したい場合は基本的にこのファイルだけを編集すればよい。
 *  特に頻繁に調整するもの:
 *    - RUN_CONFIG.handleGain* / handleStraightAbsThreshold / handleGentleAbsThreshold
 *    - RUN_CONFIG.crankHandle / crankMotor* / laneHandle / laneMotor*
 *    - LINE_DETECTOR_CONFIG.devBrightnessRatio / 各 patternXxx
 */

#ifndef CORE_PROJECT_CONFIG_H_
#define CORE_PROJECT_CONFIG_H_

#include <stdint.h>

// 各モジュールの Config 構造体宣言を取得
#include "../drivers/Camera.h"
#include "../drivers/LineDetector.h"
#include "../drivers/Motor.h"
#include "../drivers/Servo.h"
#include "../drivers/Onboard.h"
#include "../drivers/Encoder.h"
#include "../drivers/Serial.h"
#include "../drivers/SDCard.h"
#include "../drivers/SDLogger.h"
#include "../logic/RunLogic.h"

// (RunLogicConfig 構造体宣言は src/logic/RunLogic.h に移動済み)

// ====================================================================
// インスタンス定義（実値はここで一括管理）
//
// inline const は C++17 機能。e2 studio のコンパイラ指定が古い場合は
// extern const 宣言 + ProjectConfig.cpp で実体定義に切り替えること。
// ====================================================================

inline const CameraConfig CAMERA_CONFIG = {
    /*pixelWidth*/       160,
    /*pixelHeight*/      120,
    /*thresholdDefault*/ 170,
    /*thresholdRow*/     60,
    /*thresholdDiff*/    8
};

inline const LineDetectorConfig LINE_DETECTOR_CONFIG = {
    /*detectRowDefault*/      57,
    /*detectHeightDefault*/   3,

    /*devBrightnessRatio*/    0.87f,
    /*devMinasDiffTh*/        -8,
    /*devPlusDiffTh*/         8,
    /*devOutlierThY*/         5,
    /*devLeftDefaultX*/       70,
    /*devRightDefaultX*/      90,
    /*devBottomCenterOffset*/ 10,

    // patternNormal (pattern==TRACE_NORMAL=11)
    //  crosslineWidth, detectHeight, crossCountThreshold, brightnessRatio, brightnessAbs, centerWidth
    {   70,             3,            -1 /*=width-1*/,     0.69f,           0,             45 },

    // patternStart (pattern==1 or 2)
    {   60,             10,           59,                  0.0f,            90,            100 },

    // patternOther (それ以外)
    //   crosslineWidth: -1 → detectRow_ + 20 を実行時に算出
    //   crossCountThreshold: -10 → crosslineWidth - 10
    {   -1,             3,            -10,                 0.68f,           0,             45 },

    /*centerRowNum*/         47,
    /*centerCountThreshold*/ 10,
    /*centerBrightnessAbs*/  170,

    /*sensorBinRow*/         60,
    /*sensorBinThreshold*/   180,
    /*sensorBinDiff*/        8
};

inline const MotorConfig MOTOR_CONFIG = {
    /*pwmCycle*/        33332,
    /*maxPower*/        100,
    /*leftPwmPinFmt*/   4,    // P4_4 (TIOC4A)
    /*rightPwmPinFmt*/  5,    // P4_5 (TIOC4B)
    /*leftDirPinFmt*/   6,    // P4_6
    /*rightDirPinFmt*/  7     // P4_7
};

inline const ServoConfig SERVO_CONFIG = {
    /*pwmCycle*/   33332,
    /*center*/     3090,
    /*handleStep*/ 23,
    /*maxAngle*/   40,
    /*pwmPinFmt*/  0          // P4_0 (TIOC0A)
};

inline const OnboardConfig ONBOARD_CONFIG = {
    /*swPinFmt*/         0,   // P6_0
    /*ledUserPinFmt*/    12,  // P6_12
    /*ledRedPinFmt*/     13,  // P6_13
    /*ledGreenPinFmt*/   14,  // P6_14
    /*ledBluePinFmt*/    15   // P6_15
};

inline const EncoderConfig ENCODER_CONFIG = {
    /*filterN*/  4,
    /*rcAlpha*/  0.5f,
    /*aPinFmt*/  0,    // P1_0  TCLKA
    /*bPinFmt*/  10    // P1_10 TCLKB
};

inline const SerialConfig SERIAL_CONFIG = {
    /*baud*/       230400,
    /*txPinFmt*/   3,         // P6_3 TxD2
    /*rxPinFmt*/   2,         // P6_2 RxD2
    /*bufferSize*/ 512
};

inline const SDCardConfig SDCARD_CONFIG = {
    /*csPinFmt*/   4,    // P8_4
    /*cdPinFmt*/   8,    // P7_8
    /*mosiPinFmt*/ 5,    // P8_5
    /*misoPinFmt*/ 6,    // P8_6
    /*sckPinFmt*/  3     // P8_3
};

inline const SDLoggerConfig SDLOGGER_CONFIG = {
    /*maxEntries*/       4000,
    /*imageWidth*/       160,
    /*recordImageRow*/   45,
    /*patternMinForLog*/ 11,   // RP_TRACE_NORMAL
    /*finishPattern*/    101   // RP_FINISH
};

inline const RunLogicConfig RUN_CONFIG = {
    // --- 状態タイマー定数 [ms] ---
    /*tLineSkipMs*/         100,
    /*tCrankMs*/            440,
    /*tLaneTurnMs*/         340,
    /*tLaneStraightMs*/     0,
    /*tLaneCounterMs*/      250,
    /*tCourseTimeoutMs*/    60000,
    /*tBarWaitPreMs*/       500,
    /*tBarWaitPostMs*/      1000,
    /*tAfterBarGoMs*/       100,
    /*tHalfAfterTimeoutMs*/ 1000,
    /*tTraceLineEnableMs*/  700,
    /*tBarApproachWaitMs*/  100,

    // --- 走行パラメータ ---
    /*crankHandle*/         42,
    /*crankMotorOut*/       100,
    /*crankMotorIn*/        100,
    /*laneHandle*/          23,
    /*laneCounterHandle*/   38,
    /*laneMotorIn*/         37,
    /*laneMotorOut*/        55,
    /*laneStraightMotor*/   100,
    /*laneCounterMotorIn*/  20,
    /*laneCounterMotorOut*/ 20,
    /*brakeTargetMotor*/    43,
    /*approachMotorPower*/  18,

    // --- 検出行 ---
    /*traceRow*/            45,
    /*approachRow*/         95,

    // --- ハンドル計算ゲイン ---
    /*handleStraightAbsThreshold*/ 0,
    /*handleGentleAbsThreshold*/   7,
    /*handleGainGentle*/           0.37f,
    /*handleGainSharp*/            0.5f,
    /*handleGainNonNormal*/        0.5f
};

#endif /* CORE_PROJECT_CONFIG_H_ */
