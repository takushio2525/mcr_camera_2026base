/*
 * RunController.cpp
 *
 *  メイン走行制御モジュール（初心者向けベース）の実装
 *
 *  参考プロジェクト mcr_shiozawa_cclass_2.38m-s/main.cpp の intTimer() 内
 *  switch(pattern) を移植したもの。
 *
 *  オリジナルは「エンコーダ累積パルス」で距離管理していたが、
 *  本実装ではすべて「経過時間 (ms)」で管理する。
 *  状態に入ったタイミングで stateMs_ = 0 にリセットされ、
 *  1ms 割り込みごとに 1 ずつ加算される。
 */

#include "RunController.h"
#include "Motor.h"
#include "Servo.h"
#include "LineDetector.h"
#include "Onboard.h"

#include <stdlib.h>

// グローバルインスタンス
RunController g_runController;

// 状態に入る前の偏差を保持する補助変数（pattern==101 で前回ハンドル維持に使用）
static int s_lastHandle101 = 0;

// ====================================================================
// コンストラクタ
// ====================================================================
RunController::RunController()
    : pattern_(WAIT_SW),
      stateMs_(0),
      totalMs_(0),
      cnt1Ms_(0),
      finished_(false),
      handleVal_(0),
      traceOffset_(0),
      leftMotor_(0),
      rightMotor_(0),
      leftBrake_(0),
      rightBrake_(0)
{
}

// ====================================================================
// init()
// ====================================================================
bool RunController::init()
{
    pattern_      = WAIT_SW;
    stateMs_      = 0;
    totalMs_      = 0;
    cnt1Ms_       = 0;
    finished_     = false;
    handleVal_    = 0;
    traceOffset_  = 0;
    leftMotor_    = 0;
    rightMotor_   = 0;
    leftBrake_    = 0;
    rightBrake_   = 0;

    motor(0, 0);
    handle(0);
    setColorLed(0, 0, 0);
    return true;
}

// ====================================================================
// updateOutput() —— 1ms 割り込みから呼ばれる
// （Step 4 段階では機械置換のみ。Step 10 で RunLogic.cpp の関数群へ分解予定）
// ====================================================================
void RunController::updateOutput(SystemData& sys)
{
    (void)sys;
    // タイマー進行
    stateMs_++;
    cnt1Ms_++;

    // 走行スタート後（pattern>=11）から総経過時間をカウント
    if (pattern_ >= TRACE_NORMAL && pattern_ != FINISH)
    {
        totalMs_++;
    }

    // ライン検出側に現在のパターンを通知（参考プロジェクトの
    // createLineFlag() は pattern によりパラメータを切り替えるため必須）
    g_lineDetector.setPattern((int)pattern_);

    // USER_LED (P6_12) は中央2点センサ反応で点灯（参考プロジェクトの
    // USER_LED = sensor_inp(0x18) != 0 ? 1 : 0 と同等）。
    // 中心線検出をリアルタイムで目視確認するために毎周期反映する。
    g_onboard.setUserLed(g_lineDetector.sensorInput(0x18) != 0 ? 1 : 0);

    // 通常時のハンドル/モーター値を毎周期計算しておく
    // （各 case 内では手動で上書きすることもある）
    calcHandle();
    calcMotorVal();

    // パターンごとの処理に分岐
    switch (pattern_)
    {
    case WAIT_SW:             runWaitSw();             break;
    case APPROACH_BAR:        runApproachBar();        break;
    case WAIT_AT_BAR:         runWaitAtBar();          break;
    case ACCEL_AFTER_BAR:     runAccelAfterBar();      break;
    case CHECK_BAR_OPEN:      runCheckBarOpen();       break;
    case WAIT_BAR_OPEN:       runWaitBarOpen();        break;
    case TRACE_NORMAL:        runTraceNormal();        break;
    case CROSSLINE_DETECTED:  runCrosslineDetected();  break;
    case CROSSLINE_SKIP:      runCrosslineSkip();      break;
    case CROSSLINE_AFTER:     runCrosslineAfter();     break;
    case LEFT_CRANK_ENTRY:    runLeftCrankEntry();     break;
    case LEFT_CRANK:          runLeftCrank();          break;
    case RIGHT_CRANK_ENTRY:   runRightCrankEntry();    break;
    case RIGHT_CRANK:         runRightCrank();         break;
    case RIGHT_HALF_DETECTED: runRightHalfDetected();  break;
    case RIGHT_HALF_SKIP:     runRightHalfSkip();      break;
    case RIGHT_HALF_AFTER:    runRightHalfAfter();     break;
    case RIGHT_LANE_PREPARE:  runRightLanePrepare();   break;
    case RIGHT_LANE_TURN:     runRightLaneTurn();      break;
    case RIGHT_LANE_STRAIGHT: runRightLaneStraight();  break;
    case RIGHT_LANE_COUNTER:  runRightLaneCounter();   break;
    case LEFT_HALF_DETECTED:  runLeftHalfDetected();   break;
    case LEFT_HALF_SKIP:      runLeftHalfSkip();       break;
    case LEFT_HALF_AFTER:     runLeftHalfAfter();      break;
    case LEFT_LANE_PREPARE:   runLeftLanePrepare();    break;
    case LEFT_LANE_TURN:      runLeftLaneTurn();       break;
    case LEFT_LANE_STRAIGHT:  runLeftLaneStraight();   break;
    case LEFT_LANE_COUNTER:   runLeftLaneCounter();    break;
    case FINISH:              runFinish();             break;
    default:                  changePattern(WAIT_SW);  break;
    }
}

// ====================================================================
// forceStop()
// ====================================================================
void RunController::forceStop()
{
    motor(0, 0);
    handle(0);
    setColorLed(1, 0, 0);
    changePattern(FINISH);
}

// ====================================================================
// changePattern() —— 状態遷移時の共通処理
// ====================================================================
void RunController::changePattern(Pattern p)
{
    pattern_ = p;
    stateMs_ = 0;
}

// ====================================================================
// 出力ヘルパー（参考プロジェクトの handle()/motor()/led_m() 相当）
// ====================================================================
void RunController::handle(int angle)
{
    g_servo.setAngle(angle);
}

void RunController::motor(int left, int right)
{
    g_motor.set(left, right);
}

void RunController::setColorLed(int r, int g, int b)
{
    g_onboard.setColorLed(r, g, b);
}

// ====================================================================
// calcHandle() —— ハンドル指示値の計算
//
// 参考プロジェクトの createHandleVal() を初心者向けにシンプル化したもの。
// エンコーダ速度 (getCnt) によるトレース行切替や係数変更は廃止し、
// 「行 TRACE_ROW の偏差」と「直線/カーブの大ざっぱ判定」だけで決定する。
// ====================================================================
void RunController::calcHandle()
{
    // 走行中以外（停止系）は handleVal_ を上書きしない
    if (pattern_ < TRACE_NORMAL)
    {
        return;
    }

    int dev = g_lineDetector.getDeviation(TRACE_ROW);
    int absDev = (dev >= 0) ? dev : -dev;

    if (pattern_ != TRACE_NORMAL)
    {
        // クロス/ハーフ後など pattern!=11 のとき：単純に偏差 * 0.5
        handleVal_ = (dev + traceOffset_) / 2;
    }
    else if (absDev <= 0)
    {
        // 直線
        handleVal_ = 0;
    }
    else if (absDev <= 7)
    {
        // 緩やかなカーブ：係数 0.37
        handleVal_ = (int)((dev + traceOffset_) * 0.37f);
    }
    else
    {
        // 大きなカーブ：偏差そのままに 0.5 倍
        handleVal_ = (dev + traceOffset_) / 2;
    }
}

// ====================================================================
// calcMotorVal() —— 通常走行モーター値の計算
//
// 参考プロジェクトでは encoder 速度に応じた回生ブレーキを行っていたが、
// 初心者向けには「常に最大値で前進」する単純な実装にする。
// ====================================================================
void RunController::calcMotorVal()
{
    leftMotor_  = Motor::MAX_POWER;
    rightMotor_ = Motor::MAX_POWER;
}

// ====================================================================
// calcBrakeMotorVal() —— ブレーキモーター値の計算
//
// 参考プロジェクトの createBrakeMotorVal() は速度差に応じたPI制御だったが、
// 初心者向けには「ターゲット出力 = targetPower 固定」に簡略化する。
// ====================================================================
void RunController::calcBrakeMotorVal(int targetPower)
{
    leftBrake_  = targetPower;
    rightBrake_ = targetPower;
}

// ====================================================================
// 各パターンの処理本体
// 参考プロジェクト main.cpp の switch(pattern) case の構造を保ちつつ、
// エンコーダ距離 → stateMs_ に置換している。
// ====================================================================

// pattern 0: スイッチ入力待ち
//
// 暫定動作: スタートバー検知後の停止/前進シーケンスはスキップし、
//          ボタン押下で即 TRACE_NORMAL（通常トレース）へ遷移する。
//          ただし LED 色はクロスライン検出状態に応じて切り替えるため、
//          バーを正しく置けたかをLEDで確認できる（参考プロジェクト準拠）。
void RunController::runWaitSw()
{
    motor(0, 0);
    handle(0);

    // クロスライン検出時は緑（バーセットOK）、非検出時は赤（未セット）
    if (g_lineDetector.isCrossLine())
    {
        setColorLed(0, 1, 0);
    }
    else
    {
        setColorLed(1, 0, 0);
    }

    // ボタン押下で即スタート
    if (g_onboard.sw())
    {
        totalMs_ = 0;
        changePattern(TRACE_NORMAL);
    }

    // // === 元のバー検知あり版（参考プロジェクト準拠） ===
    // if (!g_lineDetector.isCrossLine())
    // {
    //     // スタートバー反応なし → 赤色LED、ボタン押下で前進開始
    //     setColorLed(1, 0, 0);
    //     if (g_onboard.sw())
    //     {
    //         changePattern(APPROACH_BAR);
    //         cnt1Ms_ = 0;
    //     }
    // }
    // else
    // {
    //     // スタートバー反応あり → 緑色LED、ボタン押下で CHECK_BAR_OPEN へ
    //     setColorLed(0, 1, 0);
    //     if (g_onboard.sw())
    //     {
    //         changePattern(CHECK_BAR_OPEN);
    //         cnt1Ms_ = 0;
    //     }
    // }
}

// pattern 1: スタートバーに向かって進む
void RunController::runApproachBar()
{
    setColorLed(1, 1, 0); // 黄色

    // 500ms 経過で前進開始
    if (cnt1Ms_ >= T_BAR_WAIT_PRE_MS)
    {
        motor(18, 18);
    }
    else
    {
        motor(0, 0);
    }

    // バー手前のラインに向けて軽くハンドル制御
    handle(g_lineDetector.getDeviation(APPROACH_ROW) / 2);

    // クロスラインを検知したら停止フェーズへ
    if (g_lineDetector.isCrossLine())
    {
        setColorLed(0, 0, 1);
        changePattern(WAIT_AT_BAR);
        cnt1Ms_ = 0;
    }
}

// pattern 2: スタートバー手前で停止
void RunController::runWaitAtBar()
{
    motor(0, 0);

    if (cnt1Ms_ <= 100)
    {
        handle(g_lineDetector.getDeviation(APPROACH_ROW) / 2);
    }
    else
    {
        handle(0);
    }

    // 1秒経過 + クロスライン消失 → 加速フェーズへ
    if (!g_lineDetector.isCrossLine() && cnt1Ms_ >= T_BAR_WAIT_POST_MS)
    {
        setColorLed(0, 1, 0);
        changePattern(ACCEL_AFTER_BAR);
    }
}

// pattern 3: バー上昇後の加速
void RunController::runAccelAfterBar()
{
    handle(handleVal_);
    motor(Motor::MAX_POWER, Motor::MAX_POWER);

    if (stateMs_ >= T_AFTER_BAR_GO_MS)
    {
        changePattern(CHECK_BAR_OPEN);
        cnt1Ms_ = 0;
    }
}

// pattern 4: バー再判定
void RunController::runCheckBarOpen()
{
    motor(0, 0);
    handle(0);
    if (!g_lineDetector.isCrossLine())
    {
        changePattern(WAIT_BAR_OPEN);
    }
}

// pattern 10: バー開放待ち
void RunController::runWaitBarOpen()
{
    setColorLed(1, 1, 0); // 黄色
    motor(0, 0);
    handle(0);

    if (!g_lineDetector.isCrossLine())
    {
        // スタート！
        setColorLed(0, 0, 0);
        totalMs_ = 0;
        changePattern(TRACE_NORMAL);
    }
}

// pattern 11: 通常トレース
void RunController::runTraceNormal()
{
    setColorLed(1, 1, 1); // 白色

    traceOffset_ = 0;
    motor(leftMotor_, rightMotor_);
    handle(handleVal_);

    // ライン検出（700ms 以降に有効化、参考プロジェクトの total>=700 相当）
    if (totalMs_ >= 700)
    {
        if (g_lineDetector.isCrossLine())
        {
            changePattern(CROSSLINE_DETECTED);
        }
        else if (g_lineDetector.isRightLine())
        {
            changePattern(RIGHT_HALF_DETECTED);
            traceOffset_ = 0;
        }
        else if (g_lineDetector.isLeftLine())
        {
            changePattern(LEFT_HALF_DETECTED);
            traceOffset_ = 0;
        }
    }

    // タイムアウト（コース1周想定）
    if (totalMs_ >= T_COURSE_TIMEOUT_MS)
    {
        changePattern(FINISH);
        cnt1Ms_ = 0;
    }
}

// pattern 21: クロスライン検出
void RunController::runCrosslineDetected()
{
    setColorLed(1, 0, 0);
    changePattern(CROSSLINE_SKIP);
}

// pattern 22: クロスラインを読み飛ばす
void RunController::runCrosslineSkip()
{
    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);

    // 規定時間経過 + ライン消失 → 次フェーズへ
    if (stateMs_ >= T_LINE_SKIP_MS
        && !g_lineDetector.isLeftLine()
        && !g_lineDetector.isRightLine()
        && !g_lineDetector.isCrossLine())
    {
        changePattern(CROSSLINE_AFTER);
    }
}

// pattern 23: クロスライン後のトレース、クランク検出
void RunController::runCrosslineAfter()
{
    if (g_lineDetector.isLeftLine())
    {
        // 左クランク
        setColorLed(0, 1, 0);
        handle(CRANK_HANDLE);
        motor(CRANK_MOTOR_IN, CRANK_MOTOR_OUT);
        changePattern(LEFT_CRANK_ENTRY);
        return;
    }
    if (g_lineDetector.isRightLine())
    {
        // 右クランク
        setColorLed(0, 0, 1);
        handle(-CRANK_HANDLE);
        motor(CRANK_MOTOR_OUT, CRANK_MOTOR_IN);
        changePattern(RIGHT_CRANK_ENTRY);
        return;
    }

    // ハーフ/クロスがしばらく検知されないままの場合は通常トレースへ戻す
    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);
}

// pattern 30: 左クランク開始
void RunController::runLeftCrankEntry()
{
    changePattern(LEFT_CRANK);
}

// pattern 31: 左クランク中
void RunController::runLeftCrank()
{
    setColorLed(0, 1, 0);
    handle(CRANK_HANDLE);
    motor(CRANK_MOTOR_IN, CRANK_MOTOR_OUT);

    if (stateMs_ >= T_CRANK_MS)
    {
        changePattern(TRACE_NORMAL);
    }
}

// pattern 40: 右クランク開始
void RunController::runRightCrankEntry()
{
    changePattern(RIGHT_CRANK);
}

// pattern 41: 右クランク中
void RunController::runRightCrank()
{
    setColorLed(0, 1, 0);
    handle(-CRANK_HANDLE);
    motor(CRANK_MOTOR_OUT, CRANK_MOTOR_IN);

    if (stateMs_ >= T_CRANK_MS)
    {
        changePattern(TRACE_NORMAL);
    }
}

// pattern 51: 右ハーフライン検出
void RunController::runRightHalfDetected()
{
    setColorLed(0, 1, 0);
    changePattern(RIGHT_HALF_SKIP);
}

// pattern 52: 右ハーフを読み飛ばす
void RunController::runRightHalfSkip()
{
    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);

    // クロスや左ラインに切り替わった場合はクロス処理へ流す
    if (g_lineDetector.isLeftLine() || g_lineDetector.isCrossLine())
    {
        changePattern(CROSSLINE_SKIP);
        return;
    }

    if (stateMs_ >= T_LINE_SKIP_MS
        && !g_lineDetector.isLeftLine()
        && !g_lineDetector.isRightLine())
    {
        changePattern(RIGHT_HALF_AFTER);
    }
}

// pattern 53: 右ハーフ後のトレース、レーンチェンジ
void RunController::runRightHalfAfter()
{
    if (!g_lineDetector.isCenterLine())
    {
        setColorLed(1, 1, 0);
        changePattern(RIGHT_LANE_PREPARE);
        return;
    }

    if (stateMs_ >= 1000)
    {
        // タイムアウトで通常へ戻す
        setColorLed(1, 1, 0);
        changePattern(TRACE_NORMAL);
        return;
    }

    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);
}

// pattern 54: 右レーンチェンジ準備
void RunController::runRightLanePrepare()
{
    handle(0);
    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);

    setColorLed(1, 0, 0);
    changePattern(RIGHT_LANE_TURN);
}

// pattern 55: 右レーンチェンジ：曲がる
void RunController::runRightLaneTurn()
{
    handle(-LANE_HANDLE);
    motor(LANE_MOTOR_OUT, LANE_MOTOR_IN);

    if (stateMs_ >= T_LANE_TURN_MS)
    {
        setColorLed(0, 0, 0);
        changePattern(RIGHT_LANE_STRAIGHT);
    }
}

// pattern 56: 右レーンチェンジ：直進
void RunController::runRightLaneStraight()
{
    handle(0);
    motor(LANE_STRAIGHT_MOTOR, LANE_STRAIGHT_MOTOR);

    if (stateMs_ >= T_LANE_STRAIGHT_MS)
    {
        setColorLed(0, 0, 1);
        changePattern(RIGHT_LANE_COUNTER);
    }
}

// pattern 57: 右レーンチェンジ：戻し
void RunController::runRightLaneCounter()
{
    handle(LANE_COUNTER_HANDLE);
    motor(LANE_COUNTER_MOTOR_IN, LANE_COUNTER_MOTOR_OUT);

    if (stateMs_ >= T_LANE_COUNTER_MS)
    {
        setColorLed(0, 0, 1);
        changePattern(TRACE_NORMAL);
    }
}

// pattern 61: 左ハーフライン検出
void RunController::runLeftHalfDetected()
{
    setColorLed(0, 1, 0);
    changePattern(LEFT_HALF_SKIP);
}

// pattern 62: 左ハーフを読み飛ばす
void RunController::runLeftHalfSkip()
{
    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);

    // クロスや右ラインに切り替わった場合はクロス処理へ流す
    if (g_lineDetector.isRightLine() || g_lineDetector.isCrossLine())
    {
        changePattern(CROSSLINE_SKIP);
        return;
    }

    if (stateMs_ >= T_LINE_SKIP_MS
        && !g_lineDetector.isLeftLine()
        && !g_lineDetector.isRightLine())
    {
        changePattern(LEFT_HALF_AFTER);
    }
}

// pattern 63: 左ハーフ後のトレース、レーンチェンジ
void RunController::runLeftHalfAfter()
{
    if (!g_lineDetector.isCenterLine())
    {
        setColorLed(1, 1, 0);
        changePattern(LEFT_LANE_PREPARE);
        return;
    }

    if (stateMs_ >= 1000)
    {
        setColorLed(1, 1, 0);
        changePattern(TRACE_NORMAL);
        return;
    }

    calcBrakeMotorVal(BRAKE_TARGET_MOTOR);
    motor(leftBrake_, rightBrake_);
    handle(handleVal_);
}

// pattern 64: 左レーンチェンジ準備
void RunController::runLeftLanePrepare()
{
    handle(0);
    motor(LANE_STRAIGHT_MOTOR, LANE_STRAIGHT_MOTOR);

    setColorLed(1, 0, 0);
    changePattern(LEFT_LANE_TURN);
}

// pattern 65: 左レーンチェンジ：曲がる
void RunController::runLeftLaneTurn()
{
    handle(LANE_HANDLE);
    motor(LANE_MOTOR_IN, LANE_MOTOR_OUT);

    if (stateMs_ >= T_LANE_TURN_MS)
    {
        setColorLed(0, 0, 0);
        changePattern(LEFT_LANE_STRAIGHT);
    }
}

// pattern 66: 左レーンチェンジ：直進
void RunController::runLeftLaneStraight()
{
    handle(0);
    motor(LANE_STRAIGHT_MOTOR, LANE_STRAIGHT_MOTOR);

    if (stateMs_ >= T_LANE_STRAIGHT_MS)
    {
        setColorLed(0, 0, 1);
        changePattern(LEFT_LANE_COUNTER);
    }
}

// pattern 67: 左レーンチェンジ：戻し
void RunController::runLeftLaneCounter()
{
    handle(-LANE_COUNTER_HANDLE);
    motor(LANE_COUNTER_MOTOR_OUT, LANE_COUNTER_MOTOR_IN);

    if (stateMs_ >= T_LANE_COUNTER_MS)
    {
        setColorLed(0, 0, 1);
        changePattern(TRACE_NORMAL);
    }
}

// pattern 101: 走行終了（ログ保存中）
void RunController::runFinish()
{
    setColorLed(0, 1, 1);

    // 緩やかに停止：状態遷移直後の値を「直前のハンドル」として固定保持する
    // （update() の最初で stateMs_++ されるため、最初の呼び出しでは stateMs_ == 1）
    if (stateMs_ <= 1)
    {
        s_lastHandle101 = handleVal_;
    }
    handle(s_lastHandle101);

    // ボタン押下で再加速、それ以外は弱いブレーキ
    if (g_onboard.sw())
    {
        motor(Motor::MAX_POWER, Motor::MAX_POWER);
    }
    else
    {
        motor(0, 0);
    }

    finished_ = true;
}
