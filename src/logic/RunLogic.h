/*
 * RunLogic.h
 *
 *  走行制御ロジック (Logic レイヤー)
 *  EMA 準拠版: SystemData ベースの純関数群
 *
 *  旧 RunController クラスの 27 状態機械を applyDrivingPattern() という
 *  純関数に分解したもの。インスタンス変数は SystemData::run に集約済み。
 *
 *  設計思想:
 *    - Logic フェーズで毎ms 1回呼ばれる
 *    - 入力: sys.line.* (ライン検出結果) / sys.ob.sw (SW 状態) / sys.run.* (状態)
 *    - 出力: sys.mot.*Cmd, sys.srv.angleCmd, sys.ob.*LedCmd, sys.run.*
 *    - ハードウェア直接呼び出しなし (g_motor/g_servo/g_onboard は使わない)
 *    - 全パラメータは ProjectConfig.h の RUN_CONFIG / MOTOR_CONFIG から取得
 */

#ifndef LOGIC_RUN_LOGIC_H_
#define LOGIC_RUN_LOGIC_H_

#include "../core/ModuleTimer.h"   // RunData が ModuleTimer を保持するため

struct SystemData;

// ====================================================================
// RunLogicConfig — 走行ロジック全パラメータ
//
// 旧 RunController.h の static const 群と calcHandle() のリテラルを集約。
// 実体は ProjectConfig.h の RUN_CONFIG で定義。
//
// 頻繁に調整されるパラメータ群:
//   - handleGain* / handleStraightAbsThreshold / handleGentleAbsThreshold
//   - crank* / lane* / brakeTargetMotor / approachMotorPower
//   - 各 t*Ms タイマー定数
//
// 各メンバ右の数値は ProjectConfig.h:135-174 の RUN_CONFIG に入っている
// 実値の「写し」であって定義ではない。値を変えるときは ProjectConfig.h を
// 編集し、本ファイルのコメントも同時に更新すること
// （過去に片方だけ更新して 31 項目中 23 項目が食い違った実績がある）。
// ====================================================================
struct RunLogicConfig
{
    // --- 状態タイマー定数 [ms] ---
    unsigned long tLineSkipMs;          // 100    クロス/ハーフ通過時間
    unsigned long tCrankMs;             // 250    クランク旋回時間
    unsigned long tLaneTurnMs;          // 250    レーンチェンジ：寄せ時間
    unsigned long tLaneStraightMs;      // 0      レーンチェンジ：直進時間（0 = 直進フェーズを即通過）
    unsigned long tLaneCounterMs;       // 250    レーンチェンジ：戻し時間
    unsigned long tCourseTimeoutMs;     // 120000 コース1周想定時間（= 2分）
    unsigned long tBarWaitPreMs;        // 100    pattern1 で前進開始までの待機
    unsigned long tBarWaitPostMs;       // 100    pattern2 で次に進む待機
    unsigned long tAfterBarGoMs;        // 100    pattern3 でバー後加速する時間
    unsigned long tHalfAfterTimeoutMs;  // 100    ハーフライン後タイムアウト
    unsigned long tTraceLineEnableMs;   // 100    通常トレースでライン検出を有効化する経過時間
    unsigned long tBarApproachWaitMs;   // 100    pattern2 でハンドル制御するアシスト時間

    // --- 走行パラメータ ---
    int crankHandle;            // 30   クランク旋回角度 [度]
    int crankMotorOut;          // 70   クランク外輪 [%]
    int crankMotorIn;           // 70   クランク内輪 [%]
    int laneHandle;             // 25   レーンチェンジ寄せ角度 [度]
    int laneCounterHandle;      // 25   レーンチェンジ戻し角度 [度]
    int laneMotorIn;            // 60   レーンチェンジ内輪 [%]
    int laneMotorOut;           // 60   レーンチェンジ外輪 [%]
    int laneStraightMotor;      // 60   レーンチェンジ直進 [%]
    int laneCounterMotorIn;     // 60   レーンチェンジ戻し内輪 [%]
    int laneCounterMotorOut;    // 60   レーンチェンジ戻し外輪 [%]
    int brakeTargetMotor;       // 60   ブレーキ走行時の目標モーター出力 [%]
    int approachMotorPower;     // 60   バー前進時のモーター出力 [%]

    // --- 検出行 ---
    // 画像は 160x120。行番号は上端 0 → 下端 119（下ほど車体に近い）。
    int traceRow;               // 48   通常トレース時の偏差取得行
    int approachRow;            // 95   スタート時の偏差取得行（下寄り = 直近を見る）

    // --- ハンドル計算ゲイン（最頻調整対象） ---
    int   handleStraightAbsThreshold;  // 0     |dev| <= ここまで直線扱い
    int   handleGentleAbsThreshold;    // 7     |dev| <= ここまで緩カーブ
    float handleGainGentle;            // 0.24  緩カーブのゲイン
    float handleGainSharp;             // 0.24  急カーブのゲイン
    float handleGainNonNormal;         // 0.24  pattern!=TRACE_NORMAL のゲイン
};

// ====================================================================
// RunData — 走行ロジック (Logic レイヤー) が「所有」する状態データ
// SystemData::run に集約される。
// 旧 RunController クラスのインスタンス変数群をすべてここに集約。
// applyDrivingPattern() は SystemData::run を読み書きする純関数となる。
// ====================================================================
struct RunData
{
    int           pattern       = 0;     // 現在の走行パターン番号 (RunPattern を int 化)
    int           prevPattern   = 0;     // 直前のパターン（遷移検知用）
    int           handleVal     = 0;     // 直近のハンドル指示値（debug 表示用）
    int           traceOffset   = 0;     // ハンドル計算オフセット
    int           leftMotor     = 0;     // calcMotorVal() の結果
    int           rightMotor    = 0;
    int           leftBrake     = 0;     // calcBrakeMotorVal() の結果
    int           rightBrake    = 0;
    int           lastHandle101 = 0;     // FINISH 時のハンドル維持値
    bool          finished      = false; // 走行終了フラグ

    ModuleTimer   stateTimer;       // 旧 stateMs_ : 現パターンに入ってからの経過時間
    ModuleTimer   totalTimer;       // 旧 totalMs_ : 走行スタートからの経過時間
    ModuleTimer   auxTimer;         // 旧 cnt1Ms_  : 補助タイマー（バー検知等）
};

// ====================================================================
// 走行パターン番号 (旧 RunController::Pattern を移植)
// ====================================================================
enum RunPattern
{
    RP_WAIT_SW              = 0,    // スイッチ入力待ち
    RP_APPROACH_BAR         = 1,    // スタートバーに向かって前進
    RP_WAIT_AT_BAR          = 2,    // スタートバー手前で停止
    RP_ACCEL_AFTER_BAR      = 3,    // バー上昇後の加速
    RP_CHECK_BAR_OPEN       = 4,    // バーが開いたか確認
    RP_WAIT_BAR_OPEN        = 10,   // バー開放待ち
    RP_TRACE_NORMAL         = 11,   // 通常トレース

    RP_CROSSLINE_DETECTED   = 21,   // クロスライン検出時の処理開始
    RP_CROSSLINE_SKIP       = 22,   // クロスラインを読み飛ばす
    RP_CROSSLINE_AFTER      = 23,   // クロスライン後のトレース、クランク検出

    RP_LEFT_CRANK_ENTRY     = 30,   // 左クランク開始
    RP_LEFT_CRANK           = 31,   // 左クランク中

    RP_RIGHT_CRANK_ENTRY    = 40,   // 右クランク開始
    RP_RIGHT_CRANK          = 41,   // 右クランク中

    RP_RIGHT_HALF_DETECTED  = 51,   // 右ハーフライン検出時
    RP_RIGHT_HALF_SKIP      = 52,   // 右ハーフラインを読み飛ばす
    RP_RIGHT_HALF_AFTER     = 53,   // 右ハーフライン後のトレース、レーンチェンジ
    RP_RIGHT_LANE_PREPARE   = 54,   // 右レーンチェンジ準備
    RP_RIGHT_LANE_TURN      = 55,   // 右レーンチェンジ：曲がる
    RP_RIGHT_LANE_STRAIGHT  = 56,   // 右レーンチェンジ：直進
    RP_RIGHT_LANE_COUNTER   = 57,   // 右レーンチェンジ：戻し

    RP_LEFT_HALF_DETECTED   = 61,   // 左ハーフライン検出時
    RP_LEFT_HALF_SKIP       = 62,   // 左ハーフラインを読み飛ばす
    RP_LEFT_HALF_AFTER      = 63,   // 左ハーフライン後のトレース、レーンチェンジ
    RP_LEFT_LANE_PREPARE    = 64,   // 左レーンチェンジ準備
    RP_LEFT_LANE_TURN       = 65,   // 左レーンチェンジ：曲がる
    RP_LEFT_LANE_STRAIGHT   = 66,   // 左レーンチェンジ：直進
    RP_LEFT_LANE_COUNTER    = 67,   // 左レーンチェンジ：戻し

    RP_FINISH               = 101   // 走行終了（ログ保存中）
};

// ====================================================================
// Logic 関数群
// ====================================================================

// 起動時に1回呼ぶ初期化
// sys.run.* と初期出力 (sys.mot/srv/ob) を初期値にリセット
void runLogicInit(SystemData& sys);

// Logic フェーズで毎周期呼ぶ
// sys.run.pattern に応じた状態機械を進行させ、出力指示 (sys.mot/srv/ob) を更新
void applyDrivingPattern(SystemData& sys);

#endif /* LOGIC_RUN_LOGIC_H_ */
