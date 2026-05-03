/*
 * RunController.h
 *
 *  メイン走行制御モジュール（初心者向けベース）
 *
 *  参考プロジェクト mcr_shiozawa_cclass_2.38m-s/main.cpp の intTimer() 内
 *  switch(pattern) 状態遷移を移植したもの。
 *
 *  オリジナルでは「エンコーダの累積パルス数」で距離制御を行っていたが、
 *  本実装では初心者が読みやすいように「経過時間 (ms)」に置き換えている。
 *  各状態に入るたびに stateMs_ が 0 クリアされ、1ms 割り込みごとに加算される。
 *
 *  使い方:
 *    1. g_runController.init()              // main 起動時に1回
 *    2. g_runController.update()            // OSTM0 1ms 割り込みから呼ぶ
 *    3. g_runController.isFinished() == true // 終了判定（main から監視）
 */

#ifndef DRIVERS_RUNCONTROLLER_H_
#define DRIVERS_RUNCONTROLLER_H_

#include "../core/IModule.h"

class RunController : public IModule
{
public:
    // 走行パターン番号（参考プロジェクトの pattern と同じ番号体系）
    enum Pattern
    {
        WAIT_SW              = 0,    // スイッチ入力待ち
        APPROACH_BAR         = 1,    // スタートバーに向かって前進
        WAIT_AT_BAR          = 2,    // スタートバー手前で停止
        ACCEL_AFTER_BAR      = 3,    // バー上昇後の加速
        CHECK_BAR_OPEN       = 4,    // バーが開いたか確認
        WAIT_BAR_OPEN        = 10,   // バー開放待ち
        TRACE_NORMAL         = 11,   // 通常トレース

        CROSSLINE_DETECTED   = 21,   // クロスライン検出時の処理開始
        CROSSLINE_SKIP       = 22,   // クロスラインを読み飛ばす
        CROSSLINE_AFTER      = 23,   // クロスライン後のトレース、クランク検出

        LEFT_CRANK_ENTRY     = 30,   // 左クランク開始
        LEFT_CRANK           = 31,   // 左クランク中

        RIGHT_CRANK_ENTRY    = 40,   // 右クランク開始
        RIGHT_CRANK          = 41,   // 右クランク中

        RIGHT_HALF_DETECTED  = 51,   // 右ハーフライン検出時
        RIGHT_HALF_SKIP      = 52,   // 右ハーフラインを読み飛ばす
        RIGHT_HALF_AFTER     = 53,   // 右ハーフライン後のトレース、レーンチェンジ
        RIGHT_LANE_PREPARE   = 54,   // 右レーンチェンジ準備
        RIGHT_LANE_TURN      = 55,   // 右レーンチェンジ：曲がる
        RIGHT_LANE_STRAIGHT  = 56,   // 右レーンチェンジ：直進
        RIGHT_LANE_COUNTER   = 57,   // 右レーンチェンジ：戻し

        LEFT_HALF_DETECTED   = 61,   // 左ハーフライン検出時
        LEFT_HALF_SKIP       = 62,   // 左ハーフラインを読み飛ばす
        LEFT_HALF_AFTER      = 63,   // 左ハーフライン後のトレース、レーンチェンジ
        LEFT_LANE_PREPARE    = 64,   // 左レーンチェンジ準備
        LEFT_LANE_TURN       = 65,   // 左レーンチェンジ：曲がる
        LEFT_LANE_STRAIGHT   = 66,   // 左レーンチェンジ：直進
        LEFT_LANE_COUNTER    = 67,   // 左レーンチェンジ：戻し

        FINISH               = 101   // 走行終了（ログ保存中）
    };

    // タイマー定数 (ms単位、エンコーダ距離値をそのまま流用)
    static const unsigned long T_LINE_SKIP_MS         = 100;   // クロス/ハーフ通過時間
    static const unsigned long T_CRANK_MS             = 440;   // クランク旋回時間
    static const unsigned long T_LANE_TURN_MS         = 340;   // レーンチェンジ：寄せ時間
    static const unsigned long T_LANE_STRAIGHT_MS     = 0;     // レーンチェンジ：直進時間
    static const unsigned long T_LANE_COUNTER_MS      = 250;   // レーンチェンジ：戻し時間
    static const unsigned long T_COURSE_TIMEOUT_MS    = 60000; // コース1周想定時間
    static const unsigned long T_BAR_WAIT_PRE_MS      = 500;   // pattern1 で前進開始までの待機
    static const unsigned long T_BAR_WAIT_POST_MS     = 1000;  // pattern2 で次に進む待機
    static const unsigned long T_AFTER_BAR_GO_MS      = 100;   // pattern3 でバー後加速する時間

    // 走行パラメータ（参考プロジェクト pattern==11 のセット値そのまま）
    static const int CRANK_HANDLE                = 42;   // クランク旋回角度 [度]
    static const int CRANK_MOTOR_OUT             = 100;  // クランク外輪モーター
    static const int CRANK_MOTOR_IN              = 100;  // クランク内輪モーター
    static const int LANE_HANDLE                 = 23;   // レーンチェンジ寄せ角度 [度]
    static const int LANE_COUNTER_HANDLE         = 38;   // レーンチェンジ戻し角度 [度]
    static const int LANE_MOTOR_IN               = 37;   // レーンチェンジ内輪
    static const int LANE_MOTOR_OUT              = 55;   // レーンチェンジ外輪
    static const int LANE_STRAIGHT_MOTOR         = 100;  // レーンチェンジ直進
    static const int LANE_COUNTER_MOTOR_IN       = 20;   // レーンチェンジ戻し内輪
    static const int LANE_COUNTER_MOTOR_OUT      = 20;   // レーンチェンジ戻し外輪
    static const int BRAKE_TARGET_MOTOR          = 43;   // ブレーキ走行時の目標モーター

    static const int TRACE_ROW                   = 45;   // 通常トレース時の偏差取得行
    static const int APPROACH_ROW                = 95;   // スタート時の偏差取得行

    RunController();

    // 初期化（IModule）
    void init() override;

    // 1ms 周期処理（IModule）—— OSTM0 割り込みから呼ぶ
    // 内部で stateMs_ / totalMs_ をインクリメントし、現在の pattern に応じた処理を実行する
    void update() override;

    // 走行終了フラグ（main から参照してログ保存トリガに使う）
    bool isFinished() const { return finished_; }

    // 現在の走行パターン番号
    int getPattern() const { return (int)pattern_; }

    // 現在のハンドル設定値（デバッグ表示用）
    int getHandleVal() const { return handleVal_; }

    // デバッグ用: 走行を強制停止
    void forceStop();

private:
    Pattern        pattern_;       // 現在のパターン
    unsigned long  stateMs_;       // 現在のパターンに入ってからの経過時間 (ms)
    unsigned long  totalMs_;       // 走行スタートからの経過時間 (ms)
    unsigned long  cnt1Ms_;        // 補助タイマー（参考プロジェクトの cnt1 相当）
    bool           finished_;      // 走行終了フラグ
    int            handleVal_;     // 現在のハンドル指示値
    int            traceOffset_;   // ハンドル計算オフセット
    int            leftMotor_;     // 通常時の左モーター指示値
    int            rightMotor_;    // 通常時の右モーター指示値
    int            leftBrake_;     // ブレーキ時の左モーター指示値
    int            rightBrake_;    // ブレーキ時の右モーター指示値

    // パターン切替（stateMs_ を 0 クリア）
    void changePattern(Pattern p);

    // ハンドル制御（handleVal_ を計算）
    void calcHandle();

    // 通常モーター値計算（leftMotor_/rightMotor_ を計算）
    void calcMotorVal();

    // ブレーキモーター値計算（leftBrake_/rightBrake_ を計算）
    void calcBrakeMotorVal(int targetPower);

    // 出力ヘルパー（参考プロジェクトの handle()/motor() 相当）
    void handle(int angle);
    void motor(int left, int right);
    void setColorLed(int r, int g, int b);

    // 各パターンの処理本体（switch を読みやすく分割）
    void runWaitSw();
    void runApproachBar();
    void runWaitAtBar();
    void runAccelAfterBar();
    void runCheckBarOpen();
    void runWaitBarOpen();
    void runTraceNormal();
    void runCrosslineDetected();
    void runCrosslineSkip();
    void runCrosslineAfter();
    void runLeftCrankEntry();
    void runLeftCrank();
    void runRightCrankEntry();
    void runRightCrank();
    void runRightHalfDetected();
    void runRightHalfSkip();
    void runRightHalfAfter();
    void runRightLanePrepare();
    void runRightLaneTurn();
    void runRightLaneStraight();
    void runRightLaneCounter();
    void runLeftHalfDetected();
    void runLeftHalfSkip();
    void runLeftHalfAfter();
    void runLeftLanePrepare();
    void runLeftLaneTurn();
    void runLeftLaneStraight();
    void runLeftLaneCounter();
    void runFinish();
};

extern RunController g_runController;

#endif /* DRIVERS_RUNCONTROLLER_H_ */
