/*
 * SystemData.h
 *
 *  全モジュールの入出力データを集約するシステム共有データ構造。
 *  Embedded-Module-Architecture (EMA) の SystemData に相当する。
 *
 *  EMA の核心ルール:
 *    各モジュールの {Module}Data 構造体は **対応するモジュールヘッダ側で**
 *    宣言し、本ファイルではそれらを include して struct SystemData に
 *    集約するだけ。Data の所有権は常にモジュール側にある。
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

// 各モジュールヘッダを include することで {Module}Data 型を取得
// (各モジュールが自身の Data 構造体を所有)
//
// Camera だけは Camera.h ではなく CameraData.h を include する。
// Camera.h は VDC5 ドライバ (video/DisplayBace.h) を引き込み、その先で
// 実機のレジスタ定義を要求するため、ここで include すると SystemData.h を
// 使う全モジュールが VDC5 に依存してしまう（詳細は CameraData.h 冒頭）。
// Camera クラス自体が要るファイル（main / Camera.cpp / LineDetector.cpp）は
// いずれも Camera.h を直接 include している。
#include "../drivers/CameraData.h"      // CameraConfig / CameraData
#include "../drivers/LineDetector.h"    // LineDetectorData
#include "../drivers/Motor.h"           // MotorData
#include "../drivers/Servo.h"           // ServoData
#include "../drivers/Onboard.h"         // OnboardData
#include "../drivers/Encoder.h"         // EncoderData
#include "../drivers/Serial.h"          // SerialData
#include "../drivers/SDLogger.h"        // SDLoggerData
#include "../logic/RunLogic.h"          // RunData (Logic レイヤーの状態データ)

// ====================================================================
// SystemData — 全モジュールの Data を集約
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
