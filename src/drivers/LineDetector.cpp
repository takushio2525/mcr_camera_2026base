/*
 * LineDetector.cpp
 *
 *  ライン検出モジュール実装
 *  参考: mcr_shiozawa_cclass_2.38m-s/main.cpp
 *        createDeviation() / createLineFlag() / sensor_inp() / shikiichi_henkan()
 */

#include "LineDetector.h"
#include "Camera.h"
#include <string.h>
#include <stdlib.h>

// グローバルインスタンス
LineDetector g_lineDetector;

// ====================================================================
// コンストラクタ
// ====================================================================
LineDetector::LineDetector()
    : crossLine_(false),
      leftLine_(false),
      rightLine_(false),
      centerLine_(false),
      sensorBin_(0),
      detectRow_(57),
      detectHeight_(3),
      pattern_(0)
{
    memset(allDeviation_, 0, sizeof(allDeviation_));
}

// ====================================================================
// init()
// ベアメタル環境ではグローバルコンストラクタが呼ばれない可能性への対策として
// メンバ変数を明示的に初期化する
// ====================================================================
bool LineDetector::init()
{
    memset(allDeviation_, 0, sizeof(allDeviation_));
    crossLine_    = false;
    leftLine_     = false;
    rightLine_    = false;
    centerLine_   = false;
    sensorBin_    = 0;
    detectRow_    = 57;
    detectHeight_ = 3;
    pattern_      = 0;
    return true;
}

// ====================================================================
// update()
// 毎フレーム呼ばれる周期処理: 偏差計算 → ライン検出 → センサ更新
// ====================================================================
void LineDetector::updateOutput(SystemData& sys)
{
    (void)sys;
    calcDeviation();
    calcLineFlags();
    updateSensorBin();
}

// ====================================================================
// getDeviation()
// 指定行の偏差を返す
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
// calcDeviation()
// 参考プロジェクトの createDeviation() (pattern==11, debug_mode==3 のパス)を移植
//
// 処理の流れ:
//   1. 全画素を取得し最大輝度を記録
//   2. 最大輝度 * 0.87 以上の画素を 255 に置き換える（準二値化）
//   3. 隣接ピクセル差分を計算
//   4. 差分 < -8 → 左エッジ（白→黒）、差分 > 8 → 右エッジ（黒→白）を記録
//   5. 検出なし行はデフォルト値（左=70, 右=90）を設定
//   6. 最下行は CENTER±10 に固定
//   7. 下から上へ走査し、一行下の中心に最も近いエッジ点を選択
//   8. 一行下との差が 5 超え → 外れ値として一行下の値を使用
//   9. allDeviation_[y] = CENTER - (左エッジ + 右エッジ) / 2
// ====================================================================
void LineDetector::calcDeviation()
{
    // 閾値パラメータ（pattern==11, debug_mode==3 の値）
    const float brightnessThreshold      = 0.87f; // 準二値化の輝度倍率
    const int   minasDifferenceThreshold = -8;    // 左エッジ（白→黒）検出閾値
    const int   plusDifferenceThreshold  = 8;     // 右エッジ（黒→白）検出閾値
    const int   differenceThresholdY     = 5;     // 一行下との外れ値検出閾値

    // スタックオーバーフロー防止のため大きな配列は static ローカルで確保
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
            // 差分 < -8 → 左エッジ（白→黒）
            if (difference[y][x] < minasDifferenceThreshold)
            {
                leftExceedingXPositions[y][leftExceedingXPositionsCount[y]] = x;
                leftExceedingXPositionsCount[y]++;
            }
            // 差分 > 8 → 右エッジ（黒→白）
            if (difference[y][x] > plusDifferenceThreshold)
            {
                rightExceedingXPositions[y][rightExceedingXPositionsCount[y]] = x;
                rightExceedingXPositionsCount[y]++;
            }
        }

        // ---- ステップ5: 検出なし行はデフォルト値を設定 ----
        if (leftExceedingXPositionsCount[y] == 0)
        {
            leftExceedingXPositions[y][0] = 70; // 中心付近のデフォルト値（左）
            leftExceedingXPositionsCount[y]++;
        }
        if (rightExceedingXPositionsCount[y] == 0)
        {
            rightExceedingXPositions[y][0] = 90; // 中心付近のデフォルト値（右）
            rightExceedingXPositionsCount[y]++;
        }
    }

    // ---- ステップ6: 最下行（HEIGHT-1）は CENTER±10 に固定 ----
    leftCenterCount[HEIGHT - 1]  = 0;
    rightCenterCount[HEIGHT - 1] = 0;
    leftExceedingXPositions[HEIGHT - 1][0]  = CENTER - 10;
    rightExceedingXPositions[HEIGHT - 1][0] = CENTER + 10;

    // ---- ステップ7 & 8: 下から上へ走査し、一行下の中心に最も近いエッジ点を選択 ----
    for (int y = HEIGHT - 2; y >= 0; y--)
    {
        // 左エッジ: 一行下の中心位置に最も近い点を選択
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

        // 左エッジ外れ値チェック: 一行下との差が閾値超えなら一行下の値を使用
        if (abs(leftExceedingXPositions[y][leftCenterCount[y]]
                - leftExceedingXPositions[y + 1][leftCenterCount[y + 1]])
            > differenceThresholdY
            && y < 100)
        {
            leftCenterCount[y] = leftCenterCount[y + 1];
            leftExceedingXPositions[y][leftCenterCount[y]] =
                leftExceedingXPositions[y + 1][leftCenterCount[y + 1]];
        }

        // 右エッジ: 一行下の中心位置に最も近い点を選択
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

        // 右エッジ外れ値チェック: 一行下との差が閾値超えなら一行下の値を使用
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

    // ---- ステップ9: 偏差を計算してグローバル変数に格納 ----
    // allDeviation_[HEIGHT-1] は最下行なので 0 固定
    allDeviation_[HEIGHT - 1] = 0;
    for (int y = HEIGHT - 2; y > 1; y--)
    {
        // 偏差 = 画像中心 - (左エッジ + 右エッジ) / 2
        allDeviation_[y] = CENTER
            - (leftExceedingXPositions[y][leftCenterCount[y]]
               + rightExceedingXPositions[y][rightCenterCount[y]])
            / 2;
    }
}

// ====================================================================
// calcLineFlags()
// 参考プロジェクトの createLineFlag() を pattern 分岐込みで移植
//
// pattern による分岐（参考プロジェクトと同じ振る舞い）:
//   - pattern==11 (通常トレース)   : crosslineWidth=70 固定、threshold=max*0.69、crossCountThreshold=crosslineWidth-1
//   - pattern==1, 2 (スタート前)   : crosslineWidth=60、height=10、threshold=90、centerWidth=100、crossCountThreshold=59
//   - その他                       : crosslineWidth=detectRow_+20、threshold=max*0.68、crossCountThreshold=crosslineWidth-10
//
// 処理の流れ:
//   1. pattern に応じてパラメータを決定
//   2. detectRow_ から height 行ぶん画像を取得し、各列最大値を imageData に蓄積
//   3. 中心から左右それぞれ crosslineWidth/2 の範囲で閾値超えをカウント
//   4. カウントが crossCountThreshold/2 を超えたら左右フラグON
//   5. 左右両方ON → crossLine_
//   6. centerRowNum=47 行で輝度170超えのピクセルが centerWidth/2 範囲に
//      centerCountThreshold(=10) 以上あれば centerLine_
// ====================================================================
void LineDetector::calcLineFlags()
{
    // スタックオーバーフロー防止のため static ローカルで確保
    static int imageData[WIDTH];  // 検出領域内の各列の輝度最大値
    static int centerData[WIDTH]; // centerRowNum 行の画素データ

    // ---- パターン依存のパラメータ初期値 ----
    int rowNum         = detectRow_;
    int height         = detectHeight_;
    int crosslineWidth = rowNum + 20;

    // 通常トレース時はコース幅基準で 70 列固定（参考プロジェクト準拠）
    if (pattern_ == 11)
    {
        crosslineWidth = 70;
    }

    // スタートバー検知パターンは検出領域を厚くし、幅も狭く
    if (pattern_ == 1 || pattern_ == 2)
    {
        crosslineWidth = 60;
        height         = 10;
    }

    const int centerRowNum         = 47;            // センターライン検出行
    int       centerWidth          = centerRowNum - 2; // 既定 45
    const int centerCountThreshold = 10;            // センターライン判定カウント閾値

    int crossCountThreshold = crosslineWidth - 1; // 既定（pattern==11 はこの式）
    int maxBrightness       = 0;

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

    // ---- 明るさ閾値の決定（pattern により計算式が変わる） ----
    int brightnessThreshold = (int)(maxBrightness * 0.69f);

    if (pattern_ == 1 || pattern_ == 2)
    {
        // スタート前は固定閾値、センター検出も広く取り、クロス閾値は緩める
        brightnessThreshold = 90;
        centerWidth         = 100;
        crossCountThreshold = 59;
    }
    else if (pattern_ != 11)
    {
        // pattern==11 以外（クロス/ハーフ通過中・クランク中など）は
        // クロスライン誤検出を減らすため判定を厳しく
        crossCountThreshold = crosslineWidth - 10;
        brightnessThreshold = (int)(maxBrightness * 0.68f);
    }

    // ---- センターライン用データを取得 ----
    for (int x = 0; x < WIDTH; x++)
    {
        centerData[x] = g_camera.getPixel(x, centerRowNum);
    }

    // ---- 左ライン検出: 中心 - crosslineWidth/2 〜 中心まで走査 ----
    int leftCount = 0;
    for (int x = CENTER - crosslineWidth / 2; x < CENTER; x++)
    {
        if (x >= 0 && x < WIDTH && imageData[x] > brightnessThreshold)
        {
            leftCount++;
        }
    }
    leftLine_ = (leftCount > crossCountThreshold / 2);

    // ---- 右ライン検出: 中心 〜 中心 + crosslineWidth/2 まで走査 ----
    int rightCount = 0;
    for (int x = CENTER; x < CENTER + crosslineWidth / 2; x++)
    {
        if (x >= 0 && x < WIDTH && imageData[x] > brightnessThreshold)
        {
            rightCount++;
        }
    }
    rightLine_ = (rightCount > crossCountThreshold / 2);

    // ---- クロスライン判定: 左右両方ON ----
    crossLine_ = (leftLine_ && rightLine_);

    // ---- センターライン検出 ----
    int centerCount = 0;
    for (int x = WIDTH / 2 - centerWidth / 2; x < WIDTH / 2 + centerWidth / 2; x++)
    {
        if (x >= 0 && x < WIDTH && centerData[x] > 170)
        {
            centerCount++;
        }
    }
    centerLine_ = (centerCount > centerCountThreshold);
}

// ====================================================================
// updateSensorBin()
// Camera の thresholdConvert() を使って 8点センサ値を更新する
// 参考プロジェクトの sensor_bin = shikiichi_henkan(60, 180, 8) 相当
// ====================================================================
void LineDetector::updateSensorBin()
{
    sensorBin_ = g_camera.thresholdConvert(60, 180, 8);
}
