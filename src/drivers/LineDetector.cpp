/*
 * LineDetector.cpp
 *
 *  ライン検出モジュール実装
 *  EMA 準拠版: SystemData / Config ベース
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        createDeviation() / createLineFlag() / sensor_inp() / shikiichi_henkan()
 *
 *  各種閾値・パラメータは ProjectConfig.h の LINE_DETECTOR_CONFIG で
 *  一括管理されている。
 */

#include "LineDetector.h"
#include "Camera.h"
#include "../core/SystemData.h"
#include <string.h>
#include <stdlib.h>

// グローバルインスタンスの実体定義は main 側に集約済み（EMA 準拠）。

// ====================================================================
// コンストラクタ (Config 注入)
// ====================================================================
LineDetector::LineDetector(const LineDetectorConfig& cfg)
    : _config(cfg),
      crossLine_(false),
      leftLine_(false),
      rightLine_(false),
      centerLine_(false),
      sensorBin_(0),
      detectRow_(cfg.detectRowDefault),
      detectHeight_(cfg.detectHeightDefault),
      pattern_(0)
{
    memset(allDeviation_, 0, sizeof(allDeviation_));
}

// ====================================================================
// init()
// ベアメタル環境ではグローバルコンストラクタが呼ばれない可能性への対策として
// メンバ変数を明示的に初期化する
//
// 注: この前提はコミット 40eca9b で変わった。現在は main() 冒頭の
//     runGlobalConstructors() が .init_array を手動反復するのでコンストラクタは
//     実行される。よって本関数はコンストラクタと同じ処理を重複実行している
//     （害はないが、消すなら runGlobalConstructors() が動くことが前提）。
// ====================================================================
bool LineDetector::init()
{
    memset(allDeviation_, 0, sizeof(allDeviation_));
    crossLine_    = false;
    leftLine_     = false;
    rightLine_    = false;
    centerLine_   = false;
    sensorBin_    = 0;
    detectRow_    = _config.detectRowDefault;
    detectHeight_ = _config.detectHeightDefault;
    pattern_      = 0;
    return true;
}

// ====================================================================
// updateInput()
// メインループから sys.cam.frameReady=true のときに呼ばれる。
// 内部で偏差・ラインフラグ・8点センサを計算し、結果を sys.line.* へ格納。
// ====================================================================
void LineDetector::updateInput(SystemData& sys)
{
    // 走行ロジック (sys.run.pattern) をライン検出パラメータ切替に反映
    pattern_ = sys.run.pattern;

    // 検出行・高さは現状 Config 既定値で固定。動的変更が必要なら sys.line.detectRow を入力にする手もある。
    detectRow_    = _config.detectRowDefault;
    detectHeight_ = _config.detectHeightDefault;

    calcDeviation();
    calcLineFlags();
    updateSensorBin();

    // SystemData へ反映
    for (int y = 0; y < HEIGHT; y++)
    {
        sys.line.deviation[y] = allDeviation_[y];
    }
    sys.line.crossLine  = crossLine_;
    sys.line.leftLine   = leftLine_;
    sys.line.rightLine  = rightLine_;
    sys.line.centerLine = centerLine_;
    sys.line.sensorBin  = sensorBin_;
    sys.line.detectRow  = detectRow_;
}

// ====================================================================
// getDeviation() — 後方互換 getter
// ====================================================================
int LineDetector::getDeviation(int row) const
{
    if (row < 0 || row >= HEIGHT)
    {
        return 0;
    }
    return allDeviation_[row];
}

// ====================================================================
// selectPatternParams() — 現 pattern_ から使うパラメータセットを選択
// ====================================================================
const LineDetectorPatternParams& LineDetector::selectPatternParams() const
{
    // 数値は RunLogic.h の RunPattern と対応する:
    //   11 = RP_TRACE_NORMAL / 1 = RP_APPROACH_BAR / 2 = RP_WAIT_AT_BAR
    // enum を include すると LineDetector が Logic レイヤーに依存するため、
    // ここでは数値のままにしてある（RunPattern を変えたら要追随）。
    if (pattern_ == 11) return _config.patternNormal;
    if (pattern_ == 1 || pattern_ == 2) return _config.patternStart;
    return _config.patternOther;
}

// ====================================================================
// calcDeviation()
// 参考プロジェクトの createDeviation() (pattern==11, debug_mode==3 のパス)を
// LINE_DETECTOR_CONFIG.dev* で駆動するよう移植したもの。
// ====================================================================
void LineDetector::calcDeviation()
{
    // 閾値パラメータ（Config から取得）
    const float brightnessThreshold      = _config.devBrightnessRatio;
    const int   minasDifferenceThreshold = _config.devMinasDiffTh;
    const int   plusDifferenceThreshold  = _config.devPlusDiffTh;
    const int   differenceThresholdY     = _config.devOutlierThY;
    const int   leftDefaultX             = _config.devLeftDefaultX;
    const int   rightDefaultX            = _config.devRightDefaultX;
    const int   bottomCenterOffset       = _config.devBottomCenterOffset;

    // スタックオーバーフロー防止のため大きな配列は static ローカルで確保
    //
    // 合計サイズ: [HEIGHT][WIDTH] の signed int 配列が 6 本で
    //   120 * 160 * 4 byte * 6 = 460,800 byte ≒ 450KB を BSS に静的確保する。
    // .cproject が -Wstack-usage=100 を指定している（1 関数のスタック使用量を
    // 100 byte 以下に抑える）ため、この規模を自動変数には置けない。
    //
    // 再入について:
    //   static なので calcDeviation() は **再入不可**。ISR から呼ぶと
    //   メインループ側の計算途中のバッファを壊す。LineDetector を
    //   s_inputModules[] に載せず、メインループから sys.cam.frameReady
    //   駆動でのみ呼んでいるのはこのため（LineDetector.h の注記も参照）。
    static signed int allImageData[HEIGHT][WIDTH];             // 取得した画素データ
    static signed int difference[HEIGHT][WIDTH];               // 隣接差分

    static signed int leftExceedingXPositions[HEIGHT][WIDTH];  // 左エッジ検出X座標（各行）
    static signed int rightExceedingXPositions[HEIGHT][WIDTH]; // 右エッジ検出X座標（各行）
    static signed int leftExceedingXPositionsCount[HEIGHT];    // 左エッジ検出個数
    static signed int rightExceedingXPositionsCount[HEIGHT];   // 右エッジ検出個数

    static signed int leftYDifference[HEIGHT][WIDTH];          // 左エッジと一行下中心との距離
    static signed int rightYDifference[HEIGHT][WIDTH];         // 右エッジと一行下中心との距離

    static signed int minLeftXDifference[HEIGHT];              // 左エッジの最小距離
    static signed int minRightXDifference[HEIGHT];             // 右エッジの最小距離

    static signed int rightCenterCount[HEIGHT]; // 右エッジの選択インデックス
    static signed int leftCenterCount[HEIGHT];  // 左エッジの選択インデックス

    signed int maxBrightness = 0; // 最大輝度

    // ---- 変数の初期化 ----
    for (int y = 0; y < HEIGHT; y++)
    {
        leftExceedingXPositionsCount[y]  = 0;
        rightExceedingXPositionsCount[y] = 0;
        // 160 は X 方向に取りうる距離の上限（= WIDTH）を番兵として入れている。
        // 以降のループで必ずこれ未満の値に更新される。WIDTH 定数と同値だが
        // 参考プロジェクトの記述のままリテラルになっている。
        minLeftXDifference[y]            = 160;
        minRightXDifference[y]           = 160;
        leftCenterCount[y]               = 0;
        rightCenterCount[y]              = 0;
    }

    // ---- ステップ1: 全画素を取得し最大輝度を記録 ----
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            allImageData[y][x] = g_camera.getPixel(x, y);
            if (allImageData[y][x] > maxBrightness)
            {
                maxBrightness = allImageData[y][x];
            }
        }
    }

    // ---- ステップ2: 最大輝度 * 閾値倍率以上の画素を 255 に置き換える（準二値化） ----
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            if (allImageData[y][x] > (signed int)(maxBrightness * brightnessThreshold))
            {
                allImageData[y][x] = 255;
            }
        }
    }

    // ---- ステップ3: 隣接ピクセル差分を計算（右端は 0）----
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            if (x < WIDTH - 1) // 右端以外
            {
                difference[y][x] = allImageData[y][x] - allImageData[y][x + 1];
            }
            else
            {
                difference[y][x] = 0;
            }
        }
    }

    // ---- ステップ4: 左右エッジ検出 ----
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            // 差分 < 閾値 → 左エッジ（白→黒）
            if (difference[y][x] < minasDifferenceThreshold)
            {
                leftExceedingXPositions[y][leftExceedingXPositionsCount[y]] = x;
                leftExceedingXPositionsCount[y]++;
            }
            // 差分 > 閾値 → 右エッジ（黒→白）
            if (difference[y][x] > plusDifferenceThreshold)
            {
                rightExceedingXPositions[y][rightExceedingXPositionsCount[y]] = x;
                rightExceedingXPositionsCount[y]++;
            }
        }

        // ---- ステップ5: 検出なし行はデフォルト値を設定 ----
        if (leftExceedingXPositionsCount[y] == 0)
        {
            leftExceedingXPositions[y][0] = leftDefaultX;
            leftExceedingXPositionsCount[y]++;
        }
        if (rightExceedingXPositionsCount[y] == 0)
        {
            rightExceedingXPositions[y][0] = rightDefaultX;
            rightExceedingXPositionsCount[y]++;
        }
    }

    // ---- ステップ6: 最下行は CENTER ± offset に固定 ----
    leftCenterCount[HEIGHT - 1]  = 0;
    rightCenterCount[HEIGHT - 1] = 0;
    leftExceedingXPositions[HEIGHT - 1][0]  = CENTER - bottomCenterOffset;
    rightExceedingXPositions[HEIGHT - 1][0] = CENTER + bottomCenterOffset;

    // ---- ステップ7 & 8: 下から上へ走査し、一行下の中心に最も近いエッジ点を選択 ----
    for (int y = HEIGHT - 2; y >= 0; y--)
    {
        // 左エッジ
        for (int count = 0; count < leftExceedingXPositionsCount[y]; count++)
        {
            leftYDifference[y][count] = abs(
                leftExceedingXPositions[y][count]
                - leftExceedingXPositions[y + 1][leftCenterCount[y + 1]]);
            if (leftYDifference[y][count] < minLeftXDifference[y])
            {
                minLeftXDifference[y] = leftYDifference[y][count];
                leftCenterCount[y]    = count;
            }
        }

        // 左エッジ外れ値チェック
        // 一行下で選ばれたエッジから differenceThresholdY (=5px) を超えて
        // 離れていたら外れ値とみなし、一行下の値をそのまま引き継ぐ。
        // 付帯条件 y < 100 により、画像下端側 20 行 (y=100〜118) では
        // この補正を行わない。**100 の根拠は追えない（根拠不明）**。
        // 参考プロジェクト由来の経験値と思われる。
        if (abs(leftExceedingXPositions[y][leftCenterCount[y]]
                - leftExceedingXPositions[y + 1][leftCenterCount[y + 1]])
            > differenceThresholdY
            && y < 100)
        {
            leftCenterCount[y] = leftCenterCount[y + 1];
            leftExceedingXPositions[y][leftCenterCount[y]] =
                leftExceedingXPositions[y + 1][leftCenterCount[y + 1]];
        }

        // 右エッジ
        for (int count = 0; count < rightExceedingXPositionsCount[y]; count++)
        {
            rightYDifference[y][count] = abs(
                rightExceedingXPositions[y][count]
                - rightExceedingXPositions[y + 1][rightCenterCount[y + 1]]);
            if (rightYDifference[y][count] < minRightXDifference[y])
            {
                minRightXDifference[y] = rightYDifference[y][count];
                rightCenterCount[y]    = count;
            }
        }

        // 右エッジ外れ値チェック（左エッジ側と同じ判定。y < 100 の根拠も同様に不明）
        if (abs(rightExceedingXPositions[y][rightCenterCount[y]]
                - rightExceedingXPositions[y + 1][rightCenterCount[y + 1]])
            > differenceThresholdY
            && y < 100)
        {
            rightCenterCount[y] = rightCenterCount[y + 1];
            rightExceedingXPositions[y][rightCenterCount[y]] =
                rightExceedingXPositions[y + 1][rightCenterCount[y + 1]];
        }
    }

    // ---- ステップ9: 偏差を計算 ----
    allDeviation_[HEIGHT - 1] = 0;
    for (int y = HEIGHT - 2; y > 1; y--)
    {
        allDeviation_[y] = CENTER
            - (leftExceedingXPositions[y][leftCenterCount[y]]
               + rightExceedingXPositions[y][rightCenterCount[y]])
            / 2;
    }
}

// ====================================================================
// calcLineFlags()
// LINE_DETECTOR_CONFIG.patternNormal/Start/Other を pattern_ に応じて選択し、
// パターン別パラメータ駆動でクロス/ハーフ/センターラインを判定する。
//
// パターン別パラメータ仕様 (LineDetectorPatternParams):
//   crosslineWidth      : <0 → detectRow_ + 20 (元コードの patternOther 既定式)
//                        ≥0 → 直接値
//   detectHeight        : 検出領域の高さ
//   crossCountThreshold : <0 → crosslineWidth + 値 (例 -1 で width-1, -10 で width-10)
//                        ≥0 → 直接値
//   brightnessAbs       : >0 → 絶対閾値として使用 (brightnessRatio は無視)
//                        =0 → maxBrightness * brightnessRatio を使用
//   brightnessRatio     : maxBrightness に乗じる倍率
//   centerWidth         : センターライン検出幅
// ====================================================================
void LineDetector::calcLineFlags()
{
    // スタックオーバーフロー防止のため static ローカルで確保
    static int imageData[WIDTH];  // 検出領域内の各列の輝度最大値
    static int centerData[WIDTH]; // centerRowNum 行の画素データ

    const LineDetectorPatternParams& params = selectPatternParams();

    // crosslineWidth 解釈: <0 → detectRow_ + 20 (固定オフセット)
    // 単位は画素。detectRow_ が下（値が大きい方）にあるほど、画面上でライン
    // 幅が広く写るのでクロス判定幅も広げる、という意図の近似式。
    // 既定値では 57 + 20 = 77 px になる。
    // **オフセット 20 の根拠は追えない（根拠不明）**。参考プロジェクト由来。
    int crosslineWidth = (params.crosslineWidth < 0) ? (detectRow_ + 20)
                                                     : params.crosslineWidth;

    int height = params.detectHeight;
    int rowNum = detectRow_;

    int centerWidth          = params.centerWidth;
    const int centerRowNum         = _config.centerRowNum;
    const int centerCountThreshold = _config.centerCountThreshold;

    int maxBrightness = 0;

    // ---- imageData を 0 クリア ----
    for (int x = 0; x < WIDTH; x++)
    {
        imageData[x] = 0;
    }

    // ---- 検出領域の画像を取得し、各列の最大値を imageData に蓄積 ----
    for (int y = rowNum; y < rowNum + height; y++)
    {
        if (y < 0 || y >= HEIGHT) continue;
        for (int x = 0; x < WIDTH; x++)
        {
            int val = g_camera.getPixel(x, y);
            if (val > imageData[x])
            {
                imageData[x] = val;
            }
            if (val > maxBrightness)
            {
                maxBrightness = val;
            }
        }
    }

    // ---- 明るさ閾値の決定: brightnessAbs > 0 ならそれ、それ以外は max * ratio ----
    int brightnessThreshold = (params.brightnessAbs > 0)
        ? params.brightnessAbs
        : (int)(maxBrightness * params.brightnessRatio);

    // ---- crossCountThreshold 解釈: <0 → crosslineWidth + 値 ----
    int crossCountThreshold = (params.crossCountThreshold < 0)
        ? (crosslineWidth + params.crossCountThreshold)
        : params.crossCountThreshold;

    // ---- センターライン用データを取得 ----
    for (int x = 0; x < WIDTH; x++)
    {
        centerData[x] = g_camera.getPixel(x, centerRowNum);
    }

    // ---- 左ライン検出 ----
    int leftCount = 0;
    for (int x = CENTER - crosslineWidth / 2; x < CENTER; x++)
    {
        if (x >= 0 && x < WIDTH && imageData[x] > brightnessThreshold)
        {
            leftCount++;
        }
    }
    leftLine_ = (leftCount > crossCountThreshold / 2);

    // ---- 右ライン検出 ----
    int rightCount = 0;
    for (int x = CENTER; x < CENTER + crosslineWidth / 2; x++)
    {
        if (x >= 0 && x < WIDTH && imageData[x] > brightnessThreshold)
        {
            rightCount++;
        }
    }
    rightLine_ = (rightCount > crossCountThreshold / 2);

    // ---- クロスライン判定 ----
    crossLine_ = (leftLine_ && rightLine_);

    // ---- センターライン検出 ----
    int centerCount = 0;
    for (int x = WIDTH / 2 - centerWidth / 2; x < WIDTH / 2 + centerWidth / 2; x++)
    {
        if (x >= 0 && x < WIDTH && centerData[x] > _config.centerBrightnessAbs)
        {
            centerCount++;
        }
    }
    centerLine_ = (centerCount > centerCountThreshold);
}

// ====================================================================
// updateSensorBin()
// Camera::thresholdConvert() を _config.sensorBin* で駆動して 8点センサ更新。
// 参考プロジェクトの sensor_bin = shikiichi_henkan(60, 180, 8) 相当。
// ====================================================================
void LineDetector::updateSensorBin()
{
    sensorBin_ = g_camera.thresholdConvert(_config.sensorBinRow,
                                           _config.sensorBinThreshold,
                                           _config.sensorBinDiff);
}
