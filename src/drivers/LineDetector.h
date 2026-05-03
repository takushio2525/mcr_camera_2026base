/*
 * LineDetector.h
 *
 *  ライン検出モジュール
 *  EMA 準拠版: SystemData / Config ベース
 *
 *  カメラ画像からコースの白線を検出し、偏差・各種ラインフラグ・
 *  8点センサ値を sys.line.* に格納する。
 *
 *  Input  : updateInput(sys) で
 *           1. sys.run.pattern を読んで pattern_ にセット
 *           2. calcDeviation()    → sys.line.deviation[]
 *           3. calcLineFlags()    → sys.line.{cross,left,right,center}Line
 *           4. updateSensorBin()  → sys.line.sensorBin
 *           5. sys.line.detectRow に直近検出行を反映
 *
 *  処理が重い (HEIGHT × WIDTH ループ) ため 1ms ISR 内では呼ばず、
 *  メインループで sys.cam.frameReady = true のときのみ実行する。
 */

#ifndef DRIVERS_LINEDETECTOR_H_
#define DRIVERS_LINEDETECTOR_H_

#include "../core/IModule.h"

// ====================================================================
// LineDetectorPatternParams — pattern 別の検出パラメータ
// crosslineWidth      : <0 → detectRow_ + 20 を実行時に算出
//                      ≥0 → 直接値
// crossCountThreshold : <0 → crosslineWidth + 値 (例 -1 で width-1)
//                      ≥0 → 直接値
// brightnessAbs       : >0 → 絶対閾値として使用
//                      =0 → maxBrightness * brightnessRatio を使用
// ====================================================================
struct LineDetectorPatternParams
{
    int   crosslineWidth;
    int   detectHeight;
    int   crossCountThreshold;
    float brightnessRatio;
    int   brightnessAbs;
    int   centerWidth;
};

// ====================================================================
// LineDetectorConfig — ライン検出全体の設定
// (実体は ProjectConfig.h の LINE_DETECTOR_CONFIG で定義)
// ====================================================================
struct LineDetectorConfig
{
    int   detectRowDefault;       // 57
    int   detectHeightDefault;    // 3

    // 偏差計算パラメータ
    float devBrightnessRatio;     // 0.87
    int   devMinasDiffTh;         // -8
    int   devPlusDiffTh;          // 8
    int   devOutlierThY;          // 5
    int   devLeftDefaultX;        // 70
    int   devRightDefaultX;       // 90
    int   devBottomCenterOffset;  // 10

    // パターン別パラメータ
    LineDetectorPatternParams patternNormal;
    LineDetectorPatternParams patternStart;
    LineDetectorPatternParams patternOther;

    // センターライン検出パラメータ
    int   centerRowNum;            // 47
    int   centerCountThreshold;    // 10
    int   centerBrightnessAbs;     // 170

    // 8点センサ生成パラメータ
    int   sensorBinRow;            // 60
    int   sensorBinThreshold;      // 180
    int   sensorBinDiff;           // 8
};

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

    // Config を注入してインスタンス生成
    explicit LineDetector(const LineDetectorConfig& cfg);

    // IModule
    bool init() override;
    void updateInput(SystemData& sys) override;

    // === 後方互換 getter (RunController 用、Step 10 で削除予定) ===
    int getDeviation(int row) const;
    bool isCrossLine() const  { return crossLine_; }
    bool isLeftLine() const   { return leftLine_; }
    bool isRightLine() const  { return rightLine_; }
    bool isCenterLine() const { return centerLine_; }
    unsigned char getSensorBin() const { return sensorBin_; }
    unsigned char sensorInput(unsigned char mask) const { return sensorBin_ & mask; }

    // === 検出行 (debug 用) ===
    int getDetectRow() const { return detectRow_; }

private:
    // --- 偏差計算 (参考プロジェクトの createDeviation 相当) ---
    void calcDeviation();

    // --- ライン検出 (参考プロジェクトの createLineFlag 相当) ---
    void calcLineFlags();

    // --- 8点センサ更新 ---
    void updateSensorBin();

    // 現 pattern に応じたパターン別パラメータを返す
    const LineDetectorPatternParams& selectPatternParams() const;

    LineDetectorConfig _config;

    // === 検出結果 ===
    signed int allDeviation_[HEIGHT];
    bool crossLine_;
    bool leftLine_;
    bool rightLine_;
    bool centerLine_;
    unsigned char sensorBin_;

    // === 検出パラメータ（updateInput 開始時に Config から復元）===
    int detectRow_;
    int detectHeight_;
    int pattern_;       // sys.run.pattern を毎周期コピー
};

extern LineDetector g_lineDetector;

#endif /* DRIVERS_LINEDETECTOR_H_ */
