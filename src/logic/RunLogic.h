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

struct SystemData;

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
