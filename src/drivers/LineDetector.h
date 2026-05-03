/*
 * LineDetector.h
 *
 *  ライン検出モジュール
 *  カメラ画像からコースの白線を検出し、偏差（コース中心からのズレ）を計算する
 *
 *  処理の流れ:
 *    1. calcDeviation(): 全画素の差分からエッジを検出し、各行の偏差を計算
 *    2. calcLineFlags(): 特定行の画素から クロスライン/左右ハーフラインを検出
 *    3. updateSensorBin(): 8点センサ値を更新
 */

#ifndef DRIVERS_LINEDETECTOR_H_
#define DRIVERS_LINEDETECTOR_H_

#include "../core/IModule.h"

// マスク値定義（8点センサ用）
#define MASK2_2 0x66
#define MASK2_0 0x60
#define MASK0_2 0x06
#define MASK3_3 0xe7
#define MASK0_3 0x07
#define MASK3_0 0xe0
#define MASK4_0 0xf0
#define MASK0_4 0x0f
#define MASK4_4 0xff

class LineDetector : public IModule
{
public:
    // 画像定数
    static const int WIDTH   = 160;
    static const int HEIGHT  = 120;
    static const int CENTER  = 80;

    LineDetector();

    // IModule
    void init() override;
    void update() override;  // calcDeviation + calcLineFlags + updateSensorBin を実行

    // === 偏差取得 ===
    // row: 行番号 (0-119)、その行での画像中心からのズレを返す
    // 正=ラインが左寄り、負=ラインが右寄り
    int getDeviation(int row) const;

    // === ライン検出結果 ===
    bool isCrossLine() const  { return crossLine_; }
    bool isLeftLine() const   { return leftLine_; }
    bool isRightLine() const  { return rightLine_; }
    bool isCenterLine() const { return centerLine_; }

    // === 8点センサ ===
    unsigned char getSensorBin() const { return sensorBin_; }
    unsigned char sensorInput(unsigned char mask) const { return sensorBin_ & mask; }

    // === 検出パラメータ設定（Controller から呼ばれる） ===
    void setDetectRow(int row)       { detectRow_ = row; }
    void setDetectHeight(int height) { detectHeight_ = height; }
    int getDetectRow() const         { return detectRow_; }
    int getDetectHeight() const      { return detectHeight_; }

    // 走行パターン番号を毎周期通知する（calcLineFlags 内部で使用）
    // 参考プロジェクトの createLineFlag() は pattern によって
    // crosslineWidth / brightnessThreshold / centerWidth を切り替えるため、
    // RunController 側から現在の pattern を渡す必要がある。
    void setPattern(int p)           { pattern_ = p; }
    int  getPattern() const          { return pattern_; }

private:
    // --- 偏差計算 (参考プロジェクトの createDeviation 相当) ---
    void calcDeviation();

    // --- ライン検出 (参考プロジェクトの createLineFlag 相当) ---
    void calcLineFlags();

    // --- 8点センサ更新 ---
    void updateSensorBin();

    // === 検出結果 ===
    signed int allDeviation_[HEIGHT]; // 各行の偏差
    bool crossLine_;    // クロスライン検出フラグ
    bool leftLine_;     // 左ハーフライン検出フラグ
    bool rightLine_;    // 右ハーフライン検出フラグ
    bool centerLine_;   // センターライン検出フラグ
    unsigned char sensorBin_; // 8点センサ値

    // === 検出パラメータ ===
    int detectRow_;     // ライン検出行（デフォルト: 57）
    int detectHeight_;  // 検出高さ（デフォルト: 3）
    int pattern_;       // 現在の走行パターン（RunController から通知）
};

extern LineDetector g_lineDetector;

#endif /* DRIVERS_LINEDETECTOR_H_ */
