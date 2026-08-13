/*
 * RunLogic.cpp
 *
 *  走行制御ロジック実装 (Logic レイヤー)
 *  EMA 準拠版: SystemData ベースの純関数群
 *
 *  旧 RunController.cpp の状態機械を移植したもの。
 *  全パラメータは RUN_CONFIG / MOTOR_CONFIG (ProjectConfig.h) から取得し、
 *  ハンドル / モーター / LED 出力指示は sys.srv / sys.mot / sys.ob への
 *  代入のみで行う（ハードウェア直接呼び出しなし）。
 *
 *  状態時間の管理は ModuleTimer に置換:
 *    旧 stateMs_ → sys.run.stateTimer.getNowTime()
 *    旧 totalMs_ → sys.run.totalTimer.getNowTime()
 *    旧 cnt1Ms_  → sys.run.auxTimer.getNowTime()
 *
 *  状態遷移:
 *    changePattern(sys, p) で sys.run.pattern = p; sys.run.stateTimer.setTime();
 */

#include "RunLogic.h"
#include "../core/SystemData.h"
#include "../core/ProjectConfig.h"
#include <stdlib.h>

// ====================================================================
// 内部ヘルパ (static でファイルローカル)
// ====================================================================

static void changePattern(SystemData& sys, int newPattern)
{
    sys.run.pattern = newPattern;
    sys.run.stateTimer.setTime();
}

// ハンドル / モーター / LED への出力指示ヘルパ
static inline void outHandle(SystemData& sys, int angle)
{
    sys.srv.angleCmd = angle;
}

static inline void outMotor(SystemData& sys, int left, int right)
{
    sys.mot.leftCmd  = left;
    sys.mot.rightCmd = right;
}

static inline void outColorLed(SystemData& sys, int r, int g, int b)
{
    sys.ob.ledRedCmd   = r ? 1 : 0;
    sys.ob.ledGreenCmd = g ? 1 : 0;
    sys.ob.ledBlueCmd  = b ? 1 : 0;
}

// ====================================================================
// ハンドル指示値の計算 — 旧 RunController::calcHandle()
//
// RUN_CONFIG.handleGain* と handleStraightAbsThreshold / handleGentleAbsThreshold
// で「直線・緩カーブ・急カーブ」3 段階のゲインを切り替える。
// このプロジェクトで最も頻繁に調整するパラメータ群。
// ====================================================================
static void calcHandle(SystemData& sys)
{
    // 走行中以外（停止系）は handleVal を上書きしない
    if (sys.run.pattern < RP_TRACE_NORMAL)
    {
        return;
    }

    int dev    = sys.line.deviation[RUN_CONFIG.traceRow];
    int absDev = (dev >= 0) ? dev : -dev;
    int corrected = dev + sys.run.traceOffset;

    if (sys.run.pattern != RP_TRACE_NORMAL)
    {
        // クロス/ハーフ後など pattern!=11 のとき
        sys.run.handleVal = (int)(corrected * RUN_CONFIG.handleGainNonNormal);
    }
    else if (absDev <= RUN_CONFIG.handleStraightAbsThreshold)
    {
        // 直線
        sys.run.handleVal = 0;
    }
    else if (absDev <= RUN_CONFIG.handleGentleAbsThreshold)
    {
        // 緩やかなカーブ
        sys.run.handleVal = (int)(corrected * RUN_CONFIG.handleGainGentle);
    }
    else
    {
        // 大きなカーブ
        sys.run.handleVal = (int)(corrected * RUN_CONFIG.handleGainSharp);
    }
}

// 通常走行モーター値の計算 — 旧 calcMotorVal()
// 左右とも一律 MOTOR_CONFIG.maxPower（実値 70 [%]）を指示する。
// Motor::MAX_POWER (=100) ではないので注意。
static void calcMotorVal(SystemData& sys)
{
    sys.run.leftMotor  = MOTOR_CONFIG.maxPower;
    sys.run.rightMotor = MOTOR_CONFIG.maxPower;
}

// ブレーキモーター値の計算 — 旧 calcBrakeMotorVal()
static void calcBrakeMotorVal(SystemData& sys, int targetPower)
{
    sys.run.leftBrake  = targetPower;
    sys.run.rightBrake = targetPower;
}

// ====================================================================
// 各パターンの処理本体（旧 runXxx 関数群を移植）
// ====================================================================

// pattern 0: スイッチ入力待ち
//
// 暫定動作: スタートバー検知後の停止/前進シーケンスはスキップし、
//          ボタン押下で即 TRACE_NORMAL（通常トレース）へ遷移する。
static void runWaitSw(SystemData& sys)
{
    outMotor(sys, 0, 0);
    outHandle(sys, 0);

    // クロスライン検出時は緑（バーセットOK）、非検出時は赤（未セット）
    if (sys.line.crossLine)
    {
        outColorLed(sys, 0, 1, 0);
    }
    else
    {
        outColorLed(sys, 1, 0, 0);
    }

    // ボタン押下で即スタート
    if (sys.ob.sw)
    {
        sys.run.totalTimer.setTime();
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 1: スタートバーに向かって進む
static void runApproachBar(SystemData& sys)
{
    outColorLed(sys, 1, 1, 0); // 黄色

    // tBarWaitPreMs（実値 100ms）経過で前進開始
    if (sys.run.auxTimer.isExpired(RUN_CONFIG.tBarWaitPreMs))
    {
        outMotor(sys, RUN_CONFIG.approachMotorPower,
                       RUN_CONFIG.approachMotorPower);
    }
    else
    {
        outMotor(sys, 0, 0);
    }

    // バー手前のラインに向けて軽くハンドル制御
    outHandle(sys, sys.line.deviation[RUN_CONFIG.approachRow] / 2);

    // クロスラインを検知したら停止フェーズへ
    if (sys.line.crossLine)
    {
        outColorLed(sys, 0, 0, 1);
        changePattern(sys, RP_WAIT_AT_BAR);
        sys.run.auxTimer.setTime();
    }
}

// pattern 2: スタートバー手前で停止
static void runWaitAtBar(SystemData& sys)
{
    outMotor(sys, 0, 0);

    if (sys.run.auxTimer.getNowTime() <= RUN_CONFIG.tBarApproachWaitMs)
    {
        outHandle(sys, sys.line.deviation[RUN_CONFIG.approachRow] / 2);
    }
    else
    {
        outHandle(sys, 0);
    }

    // tBarWaitPostMs（実値 100ms）経過 + クロスライン消失 → 加速フェーズへ
    if (!sys.line.crossLine
        && sys.run.auxTimer.isExpired(RUN_CONFIG.tBarWaitPostMs))
    {
        outColorLed(sys, 0, 1, 0);
        changePattern(sys, RP_ACCEL_AFTER_BAR);
    }
}

// pattern 3: バー上昇後の加速
static void runAccelAfterBar(SystemData& sys)
{
    outHandle(sys, sys.run.handleVal);
    outMotor(sys, MOTOR_CONFIG.maxPower, MOTOR_CONFIG.maxPower);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tAfterBarGoMs))
    {
        changePattern(sys, RP_CHECK_BAR_OPEN);
        sys.run.auxTimer.setTime();
    }
}

// pattern 4: バー再判定
static void runCheckBarOpen(SystemData& sys)
{
    outMotor(sys, 0, 0);
    outHandle(sys, 0);
    if (!sys.line.crossLine)
    {
        changePattern(sys, RP_WAIT_BAR_OPEN);
    }
}

// pattern 10: バー開放待ち
static void runWaitBarOpen(SystemData& sys)
{
    outColorLed(sys, 1, 1, 0); // 黄色
    outMotor(sys, 0, 0);
    outHandle(sys, 0);

    if (!sys.line.crossLine)
    {
        // スタート！
        outColorLed(sys, 0, 0, 0);
        sys.run.totalTimer.setTime();
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 11: 通常トレース
static void runTraceNormal(SystemData& sys)
{
    outColorLed(sys, 1, 1, 1); // 白色

    sys.run.traceOffset = 0;
    outMotor(sys, sys.run.leftMotor, sys.run.rightMotor);
    outHandle(sys, sys.run.handleVal);

    // ライン検出（走行開始から tTraceLineEnableMs 経過後に有効化）
    // 参考プロジェクトは total>=700 だったが、本プロジェクトの実値は 100ms。
    if (sys.run.totalTimer.isExpired(RUN_CONFIG.tTraceLineEnableMs))
    {
        if (sys.line.crossLine)
        {
            changePattern(sys, RP_CROSSLINE_DETECTED);
        }
        else if (sys.line.rightLine)
        {
            changePattern(sys, RP_RIGHT_HALF_DETECTED);
            sys.run.traceOffset = 0;
        }
        else if (sys.line.leftLine)
        {
            changePattern(sys, RP_LEFT_HALF_DETECTED);
            sys.run.traceOffset = 0;
        }
    }

    // タイムアウト（コース1周想定 = tCourseTimeoutMs、実値 120000ms = 2分）
    if (sys.run.totalTimer.isExpired(RUN_CONFIG.tCourseTimeoutMs))
    {
        changePattern(sys, RP_FINISH);
        sys.run.auxTimer.setTime();
    }
}

// pattern 21: クロスライン検出
static void runCrosslineDetected(SystemData& sys)
{
    outColorLed(sys, 1, 0, 0);
    changePattern(sys, RP_CROSSLINE_SKIP);
}

// pattern 22: クロスラインを読み飛ばす
static void runCrosslineSkip(SystemData& sys)
{
    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);

    // 規定時間経過 + ライン消失 → 次フェーズへ
    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLineSkipMs)
        && !sys.line.leftLine
        && !sys.line.rightLine
        && !sys.line.crossLine)
    {
        changePattern(sys, RP_CROSSLINE_AFTER);
    }
}

// pattern 23: クロスライン後のトレース、クランク検出
static void runCrosslineAfter(SystemData& sys)
{
    if (sys.line.leftLine)
    {
        // 左クランク
        outColorLed(sys, 0, 1, 0);
        outHandle(sys, RUN_CONFIG.crankHandle);
        outMotor(sys, RUN_CONFIG.crankMotorIn, RUN_CONFIG.crankMotorOut);
        changePattern(sys, RP_LEFT_CRANK_ENTRY);
        return;
    }
    if (sys.line.rightLine)
    {
        // 右クランク
        outColorLed(sys, 0, 0, 1);
        outHandle(sys, -RUN_CONFIG.crankHandle);
        outMotor(sys, RUN_CONFIG.crankMotorOut, RUN_CONFIG.crankMotorIn);
        changePattern(sys, RP_RIGHT_CRANK_ENTRY);
        return;
    }

    // ハーフ/クロスがしばらく検知されないままの場合は通常トレースへ戻す
    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);
}

// pattern 30: 左クランク開始
static void runLeftCrankEntry(SystemData& sys)
{
    changePattern(sys, RP_LEFT_CRANK);
}

// pattern 31: 左クランク中
static void runLeftCrank(SystemData& sys)
{
    outColorLed(sys, 0, 1, 0);
    outHandle(sys, RUN_CONFIG.crankHandle);
    outMotor(sys, RUN_CONFIG.crankMotorIn, RUN_CONFIG.crankMotorOut);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tCrankMs))
    {
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 40: 右クランク開始
static void runRightCrankEntry(SystemData& sys)
{
    changePattern(sys, RP_RIGHT_CRANK);
}

// pattern 41: 右クランク中
static void runRightCrank(SystemData& sys)
{
    outColorLed(sys, 0, 1, 0);
    outHandle(sys, -RUN_CONFIG.crankHandle);
    outMotor(sys, RUN_CONFIG.crankMotorOut, RUN_CONFIG.crankMotorIn);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tCrankMs))
    {
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 51: 右ハーフライン検出
static void runRightHalfDetected(SystemData& sys)
{
    outColorLed(sys, 0, 1, 0);
    changePattern(sys, RP_RIGHT_HALF_SKIP);
}

// pattern 52: 右ハーフを読み飛ばす
static void runRightHalfSkip(SystemData& sys)
{
    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);

    // クロスや左ラインに切り替わった場合はクロス処理へ流す
    if (sys.line.leftLine || sys.line.crossLine)
    {
        changePattern(sys, RP_CROSSLINE_SKIP);
        return;
    }

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLineSkipMs)
        && !sys.line.leftLine
        && !sys.line.rightLine)
    {
        changePattern(sys, RP_RIGHT_HALF_AFTER);
    }
}

// pattern 53: 右ハーフ後のトレース、レーンチェンジ
static void runRightHalfAfter(SystemData& sys)
{
    if (!sys.line.centerLine)
    {
        outColorLed(sys, 1, 1, 0);
        changePattern(sys, RP_RIGHT_LANE_PREPARE);
        return;
    }

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tHalfAfterTimeoutMs))
    {
        outColorLed(sys, 1, 1, 0);
        changePattern(sys, RP_TRACE_NORMAL);
        return;
    }

    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);
}

// pattern 54: 右レーンチェンジ準備
static void runRightLanePrepare(SystemData& sys)
{
    outHandle(sys, 0);
    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);

    outColorLed(sys, 1, 0, 0);
    changePattern(sys, RP_RIGHT_LANE_TURN);
}

// pattern 55: 右レーンチェンジ：曲がる
static void runRightLaneTurn(SystemData& sys)
{
    outHandle(sys, -RUN_CONFIG.laneHandle);
    outMotor(sys, RUN_CONFIG.laneMotorOut, RUN_CONFIG.laneMotorIn);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneTurnMs))
    {
        outColorLed(sys, 0, 0, 0);
        changePattern(sys, RP_RIGHT_LANE_STRAIGHT);
    }
}

// pattern 56: 右レーンチェンジ：直進
static void runRightLaneStraight(SystemData& sys)
{
    outHandle(sys, 0);
    outMotor(sys, RUN_CONFIG.laneStraightMotor, RUN_CONFIG.laneStraightMotor);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneStraightMs))
    {
        outColorLed(sys, 0, 0, 1);
        changePattern(sys, RP_RIGHT_LANE_COUNTER);
    }
}

// pattern 57: 右レーンチェンジ：戻し
static void runRightLaneCounter(SystemData& sys)
{
    outHandle(sys, RUN_CONFIG.laneCounterHandle);
    outMotor(sys, RUN_CONFIG.laneCounterMotorIn, RUN_CONFIG.laneCounterMotorOut);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneCounterMs))
    {
        outColorLed(sys, 0, 0, 1);
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 61: 左ハーフライン検出
static void runLeftHalfDetected(SystemData& sys)
{
    outColorLed(sys, 0, 1, 0);
    changePattern(sys, RP_LEFT_HALF_SKIP);
}

// pattern 62: 左ハーフを読み飛ばす
static void runLeftHalfSkip(SystemData& sys)
{
    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);

    // クロスや右ラインに切り替わった場合はクロス処理へ流す
    if (sys.line.rightLine || sys.line.crossLine)
    {
        changePattern(sys, RP_CROSSLINE_SKIP);
        return;
    }

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLineSkipMs)
        && !sys.line.leftLine
        && !sys.line.rightLine)
    {
        changePattern(sys, RP_LEFT_HALF_AFTER);
    }
}

// pattern 63: 左ハーフ後のトレース、レーンチェンジ
static void runLeftHalfAfter(SystemData& sys)
{
    if (!sys.line.centerLine)
    {
        outColorLed(sys, 1, 1, 0);
        changePattern(sys, RP_LEFT_LANE_PREPARE);
        return;
    }

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tHalfAfterTimeoutMs))
    {
        outColorLed(sys, 1, 1, 0);
        changePattern(sys, RP_TRACE_NORMAL);
        return;
    }

    calcBrakeMotorVal(sys, RUN_CONFIG.brakeTargetMotor);
    outMotor(sys, sys.run.leftBrake, sys.run.rightBrake);
    outHandle(sys, sys.run.handleVal);
}

// pattern 64: 左レーンチェンジ準備
static void runLeftLanePrepare(SystemData& sys)
{
    outHandle(sys, 0);
    outMotor(sys, RUN_CONFIG.laneStraightMotor, RUN_CONFIG.laneStraightMotor);

    outColorLed(sys, 1, 0, 0);
    changePattern(sys, RP_LEFT_LANE_TURN);
}

// pattern 65: 左レーンチェンジ：曲がる
static void runLeftLaneTurn(SystemData& sys)
{
    outHandle(sys, RUN_CONFIG.laneHandle);
    outMotor(sys, RUN_CONFIG.laneMotorIn, RUN_CONFIG.laneMotorOut);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneTurnMs))
    {
        outColorLed(sys, 0, 0, 0);
        changePattern(sys, RP_LEFT_LANE_STRAIGHT);
    }
}

// pattern 66: 左レーンチェンジ：直進
static void runLeftLaneStraight(SystemData& sys)
{
    outHandle(sys, 0);
    outMotor(sys, RUN_CONFIG.laneStraightMotor, RUN_CONFIG.laneStraightMotor);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneStraightMs))
    {
        outColorLed(sys, 0, 0, 1);
        changePattern(sys, RP_LEFT_LANE_COUNTER);
    }
}

// pattern 67: 左レーンチェンジ：戻し
static void runLeftLaneCounter(SystemData& sys)
{
    outHandle(sys, -RUN_CONFIG.laneCounterHandle);
    outMotor(sys, RUN_CONFIG.laneCounterMotorOut, RUN_CONFIG.laneCounterMotorIn);

    if (sys.run.stateTimer.isExpired(RUN_CONFIG.tLaneCounterMs))
    {
        outColorLed(sys, 0, 0, 1);
        changePattern(sys, RP_TRACE_NORMAL);
    }
}

// pattern 101: 走行終了（ログ保存中）
static void runFinish(SystemData& sys)
{
    outColorLed(sys, 0, 1, 1);

    // 緩やかに停止：状態遷移直後の値を「直前のハンドル」として固定保持する
    // (旧: stateMs_ <= 1 のとき lastHandle101 を保存)
    if (sys.run.stateTimer.getNowTime() <= 1)
    {
        sys.run.lastHandle101 = sys.run.handleVal;
    }
    outHandle(sys, sys.run.lastHandle101);

    // ボタン押下で再加速、それ以外は弱いブレーキ
    if (sys.ob.sw)
    {
        outMotor(sys, MOTOR_CONFIG.maxPower, MOTOR_CONFIG.maxPower);
    }
    else
    {
        outMotor(sys, 0, 0);
    }

    sys.run.finished = true;
}

// ====================================================================
// applyDrivingPattern() — Logic フェーズで毎周期呼ぶ
// ====================================================================
void applyDrivingPattern(SystemData& sys)
{
    // USER_LED は中央2点センサ反応で点灯
    sys.ob.userLedCmd = (sys.line.sensorBin & 0x18) ? 1 : 0;

    // 通常時のハンドル/モーター値を毎周期計算しておく
    // （各 case 内では手動で上書きすることもある）
    calcHandle(sys);
    calcMotorVal(sys);

    // パターンごとの処理に分岐
    switch (sys.run.pattern)
    {
    case RP_WAIT_SW:             runWaitSw(sys);             break;
    case RP_APPROACH_BAR:        runApproachBar(sys);        break;
    case RP_WAIT_AT_BAR:         runWaitAtBar(sys);          break;
    case RP_ACCEL_AFTER_BAR:     runAccelAfterBar(sys);      break;
    case RP_CHECK_BAR_OPEN:      runCheckBarOpen(sys);       break;
    case RP_WAIT_BAR_OPEN:       runWaitBarOpen(sys);        break;
    case RP_TRACE_NORMAL:        runTraceNormal(sys);        break;
    case RP_CROSSLINE_DETECTED:  runCrosslineDetected(sys);  break;
    case RP_CROSSLINE_SKIP:      runCrosslineSkip(sys);      break;
    case RP_CROSSLINE_AFTER:     runCrosslineAfter(sys);     break;
    case RP_LEFT_CRANK_ENTRY:    runLeftCrankEntry(sys);     break;
    case RP_LEFT_CRANK:          runLeftCrank(sys);          break;
    case RP_RIGHT_CRANK_ENTRY:   runRightCrankEntry(sys);    break;
    case RP_RIGHT_CRANK:         runRightCrank(sys);         break;
    case RP_RIGHT_HALF_DETECTED: runRightHalfDetected(sys);  break;
    case RP_RIGHT_HALF_SKIP:     runRightHalfSkip(sys);      break;
    case RP_RIGHT_HALF_AFTER:    runRightHalfAfter(sys);     break;
    case RP_RIGHT_LANE_PREPARE:  runRightLanePrepare(sys);   break;
    case RP_RIGHT_LANE_TURN:     runRightLaneTurn(sys);      break;
    case RP_RIGHT_LANE_STRAIGHT: runRightLaneStraight(sys);  break;
    case RP_RIGHT_LANE_COUNTER:  runRightLaneCounter(sys);   break;
    case RP_LEFT_HALF_DETECTED:  runLeftHalfDetected(sys);   break;
    case RP_LEFT_HALF_SKIP:      runLeftHalfSkip(sys);       break;
    case RP_LEFT_HALF_AFTER:     runLeftHalfAfter(sys);      break;
    case RP_LEFT_LANE_PREPARE:   runLeftLanePrepare(sys);    break;
    case RP_LEFT_LANE_TURN:      runLeftLaneTurn(sys);       break;
    case RP_LEFT_LANE_STRAIGHT:  runLeftLaneStraight(sys);   break;
    case RP_LEFT_LANE_COUNTER:   runLeftLaneCounter(sys);    break;
    case RP_FINISH:              runFinish(sys);             break;
    default:                     changePattern(sys, RP_WAIT_SW); break;
    }

    sys.run.prevPattern = sys.run.pattern;
}

// ====================================================================
// runLogicInit() — 起動時の初期化
// ====================================================================
void runLogicInit(SystemData& sys)
{
    sys.run.pattern        = RP_WAIT_SW;
    sys.run.prevPattern    = RP_WAIT_SW;
    sys.run.handleVal      = 0;
    sys.run.traceOffset    = 0;
    sys.run.leftMotor      = 0;
    sys.run.rightMotor     = 0;
    sys.run.leftBrake      = 0;
    sys.run.rightBrake     = 0;
    sys.run.lastHandle101  = 0;
    sys.run.finished       = false;
    sys.run.stateTimer.setTime();
    sys.run.totalTimer.setTime();
    sys.run.auxTimer.setTime();

    // 初期出力指示も明示的にゼロクリア
    sys.mot.leftCmd  = 0;
    sys.mot.rightCmd = 0;
    sys.srv.angleCmd = 0;
    sys.ob.ledRedCmd   = 0;
    sys.ob.ledGreenCmd = 0;
    sys.ob.ledBlueCmd  = 0;
    sys.ob.userLedCmd  = 0;
}
