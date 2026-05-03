/*
 * ProjectConfig.h
 *
 *  プロジェクト全体のハードウェア設定・走行パラメータ・カメラ閾値などを
 *  一括管理する Config ハブ。
 *  Embedded-Module-Architecture (EMA) の ProjectConfig.h に相当する。
 *
 *  値を 1 箇所で管理することで、ハンドルゲインや走行パラメータの
 *  チューニングを「ここだけ書き換えれば反映される」状態にする。
 *
 *  注意:
 *    - GR-PEACH はベアメタルでレジスタ直接操作のため、ピン番号は
 *      形式的な意味合いが強い項目もある（実際のピン割り当ては各
 *      ドライバ実装内のレジスタ設定が支配する）。
 *    - 暫定的に各 *Config 構造体宣言もこのファイル内に置いている。
 *      最終的には各モジュールヘッダに移動予定。
 */

#ifndef CORE_PROJECT_CONFIG_H_
#define CORE_PROJECT_CONFIG_H_

#include <stdint.h>

// ====================================================================
// Camera
// ====================================================================
struct CameraConfig
{
    int pixelWidth;        // 160
    int pixelHeight;       // 120
    int thresholdDefault;  // 8点センサ取得時のデフォルト閾値 (170)
    int thresholdRow;      // 8点センサ取得行 (60)
    int thresholdDiff;     // 8点センサ閾値の差分 (8)
};

// ====================================================================
// LineDetector — パターン別パラメータ
// ====================================================================
// 旧 calcLineFlags() の if-else 分岐を Config 駆動にするための構造体。
// brightnessAbs > 0 のときは絶対値を使い、それ以外は brightnessRatio を使う。
// crosslineWidth < 0 のときは「detectRow + |crosslineWidth| を超えて補正」、
// crossCountThreshold < 0 のときは crosslineWidth + |crossCountThreshold| を使う、
// など特殊扱いを LineDetector 側で解釈する余地を持たせている。
struct LineDetectorPatternParams
{
    int   crosslineWidth;        // クロスライン検出幅
    int   detectHeight;          // 検出高さ
    int   crossCountThreshold;   // クロスカウント閾値
    float brightnessRatio;       // max輝度に対する閾値倍率（brightnessAbs>0 ならこちらは無視）
    int   brightnessAbs;         // 絶対閾値（>0 のとき brightnessRatio より優先）
    int   centerWidth;           // センターライン検出幅
};

// ====================================================================
// LineDetector
// ====================================================================
struct LineDetectorConfig
{
    int   detectRowDefault;       // ライン検出行 デフォルト (57)
    int   detectHeightDefault;    // 検出高さ デフォルト (3)

    // --- 偏差計算パラメータ（旧 calcDeviation のリテラル群）---
    float devBrightnessRatio;     // 0.87 : maxBrightness * 倍率 = 準二値化閾値
    int   devMinasDiffTh;         // -8   : 左エッジ検出（白→黒）閾値
    int   devPlusDiffTh;          // +8   : 右エッジ検出（黒→白）閾値
    int   devOutlierThY;          // 5    : 行間外れ値検出閾値
    int   devLeftDefaultX;        // 70   : 検出失敗時の左エッジデフォルト位置
    int   devRightDefaultX;       // 90   : 検出失敗時の右エッジデフォルト位置
    int   devBottomCenterOffset;  // 10   : 最下行固定幅（CENTER±10）

    // --- ライン検出パラメータ（パターン別）---
    LineDetectorPatternParams patternNormal;   // pattern == TRACE_NORMAL (11)
    LineDetectorPatternParams patternStart;    // pattern == 1 or 2
    LineDetectorPatternParams patternOther;    // それ以外

    // --- センターライン検出パラメータ ---
    int   centerRowNum;            // 47
    int   centerCountThreshold;    // 10
    int   centerBrightnessAbs;     // 170

    // --- 8点センサ生成パラメータ (Camera::thresholdConvert に渡す) ---
    int   sensorBinRow;            // 60   8点センサ取得行
    int   sensorBinThreshold;      // 180  輝度閾値
    int   sensorBinDiff;           // 8    隣接差分閾値
};

// ====================================================================
// Motor
// ====================================================================
struct MotorConfig
{
    int pwmCycle;       // 33332 (1ms / P0φ/1)
    int maxPower;       // 100   (出力 % 上限)
    // 以下ピン番号は形式的（実際の MTU2 + ピン設定はドライバ内）
    int leftPwmPinFmt;
    int rightPwmPinFmt;
    int leftDirPinFmt;
    int rightDirPinFmt;
};

// ====================================================================
// Servo
// ====================================================================
struct ServoConfig
{
    int pwmCycle;       // 33332
    int center;         // 3090 (1.5ms 相当)
    int handleStep;     // 23   (カウント / 度)
    int maxAngle;       // 40   (度)
    int pwmPinFmt;
};

// ====================================================================
// Onboard
// ====================================================================
struct OnboardConfig
{
    int swPinFmt;        // P6_0
    int ledUserPinFmt;   // P6_12
    int ledRedPinFmt;    // P6_13
    int ledGreenPinFmt;  // P6_14
    int ledBluePinFmt;   // P6_15
};

// ====================================================================
// Encoder
// ====================================================================
struct EncoderConfig
{
    int   filterN;     // 4   (移動平均段数)
    float rcAlpha;     // 0.5 (RC フィルタ係数)
    int   aPinFmt;     // P1_0  TCLKA
    int   bPinFmt;     // P1_10 TCLKB
};

// ====================================================================
// Serial
// ====================================================================
struct SerialConfig
{
    uint32_t baud;        // 230400
    int      txPinFmt;    // P6_3
    int      rxPinFmt;    // P6_2
    int      bufferSize;  // 512
};

// ====================================================================
// SDCard
// ====================================================================
struct SDCardConfig
{
    int csPinFmt;       // P8_4
    int cdPinFmt;       // P7_8
    int mosiPinFmt;     // P8_5
    int misoPinFmt;     // P8_6
    int sckPinFmt;      // P8_3
};

// ====================================================================
// SDLogger
// ====================================================================
struct SDLoggerConfig
{
    int maxEntries;       // 4000
    int imageWidth;       // 160
    int recordImageRow;   // 45 (画像 1 行記録時の行番号)
    int patternMinForLog; // 11 (TRACE_NORMAL 以上で記録開始)
};

// ====================================================================
// RunLogic — 走行ロジック全パラメータ
//
// このプロジェクトで最も頻繁に調整されるパラメータ群。
// ハンドルゲイン、各状態の滞在時間、クランク/レーンチェンジの角度・出力など、
// 旧 RunController.h の static const 群と calcHandle() のリテラルを
// すべてここに集約している。
// ====================================================================
struct RunLogicConfig
{
    // --- 状態タイマー定数 [ms] ---
    unsigned long tLineSkipMs;          // 100   クロス/ハーフ通過時間
    unsigned long tCrankMs;             // 440   クランク旋回時間
    unsigned long tLaneTurnMs;          // 340   レーンチェンジ：寄せ時間
    unsigned long tLaneStraightMs;      // 0     レーンチェンジ：直進時間
    unsigned long tLaneCounterMs;       // 250   レーンチェンジ：戻し時間
    unsigned long tCourseTimeoutMs;     // 60000 コース1周想定時間
    unsigned long tBarWaitPreMs;        // 500   pattern1 で前進開始までの待機
    unsigned long tBarWaitPostMs;       // 1000  pattern2 で次に進む待機
    unsigned long tAfterBarGoMs;        // 100   pattern3 でバー後加速する時間
    unsigned long tHalfAfterTimeoutMs;  // 1000  ハーフライン後タイムアウト
    unsigned long tTraceLineEnableMs;   // 700   通常トレースでライン検出を有効化する経過時間
    unsigned long tBarApproachWaitMs;   // 100   pattern2 でハンドル制御するアシスト時間

    // --- 走行パラメータ ---
    int crankHandle;            // 42   クランク旋回角度 [度]
    int crankMotorOut;          // 100  クランク外輪
    int crankMotorIn;           // 100  クランク内輪
    int laneHandle;             // 23   レーンチェンジ寄せ角度 [度]
    int laneCounterHandle;      // 38   レーンチェンジ戻し角度 [度]
    int laneMotorIn;            // 37   レーンチェンジ内輪
    int laneMotorOut;           // 55   レーンチェンジ外輪
    int laneStraightMotor;      // 100  レーンチェンジ直進
    int laneCounterMotorIn;     // 20   レーンチェンジ戻し内輪
    int laneCounterMotorOut;    // 20   レーンチェンジ戻し外輪
    int brakeTargetMotor;       // 43   ブレーキ走行時の目標モーター出力
    int approachMotorPower;     // 18   バー前進時のモーター出力

    // --- 検出行 ---
    int traceRow;               // 45   通常トレース時の偏差取得行
    int approachRow;            // 95   スタート時の偏差取得行

    // --- ハンドル計算ゲイン（最頻調整対象） ---
    int   handleStraightAbsThreshold;  // 0     |dev| <= ここまで直線扱い
    int   handleGentleAbsThreshold;    // 7     |dev| <= ここまで緩カーブ
    float handleGainGentle;            // 0.37  緩カーブのゲイン
    float handleGainSharp;             // 0.5   急カーブのゲイン
    float handleGainNonNormal;         // 0.5   pattern!=TRACE_NORMAL のゲイン
};

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

    // patternNormal (pattern==TRACE_NORMAL)
    /*  crosslineWidth, detectHeight, crossCountThreshold, brightnessRatio, brightnessAbs, centerWidth */
    {   70,             3,            -1 /*=width-1*/,     0.69f,           0,             45 },

    // patternStart (pattern==1 or 2)
    {   60,             10,           59,                  0.0f,            90,            100 },

    // patternOther (それ以外)
    //   crosslineWidth: -1 → detectRow + 20 を実行時に算出
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
    /*patternMinForLog*/ 11   // TRACE_NORMAL
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
