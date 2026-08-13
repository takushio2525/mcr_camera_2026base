/*
 * IModule.h
 *
 *  Embedded-Module-Architecture (EMA) 準拠の基底インターフェース。
 *
 *  3フェーズ実行モデル:
 *    Input  : updateInput(SystemData&)  ハードウェア → SystemData
 *    Logic  : (logic/RunLogic.cpp)      SystemData   → SystemData
 *    Output : updateOutput(SystemData&) SystemData   → ハードウェア
 *
 *  入力専用モジュール（センサ等）は updateInput のみを override する。
 *  出力専用モジュール（モーター・サーボ等）は updateOutput のみを override する。
 *  入出力両方のモジュール（Onboard 等）は両方を override する。
 *
 *  SystemData は前方宣言とすることで、各モジュールヘッダから
 *  SystemData.h をインクルードさせず、循環依存を回避する。
 */

#ifndef CORE_IMODULE_H_
#define CORE_IMODULE_H_

// 前方宣言（SystemData の実体は SystemData.h で定義）
struct SystemData;

class IModule
{
public:
    virtual ~IModule() {}

    // 初期化処理（成功 = true、失敗 = false）。
    // 失敗時は呼び出し側で enabled = false に落とす運用を推奨。
    //
    // 現状この運用は未実装: main() は全ての init() の戻り値を捨てており、
    // enabled に false を書く箇所も存在しない（下の enabled も読まれるだけ）。
    // そのため SDLogger::init() が SD 未挿入で false を返しても隔離されない。
    virtual bool init() = 0;

    // 入力フェーズ: ハードウェア → SystemData
    // 出力専用モジュールはオーバーライド不要（デフォルト空実装）。
    virtual void updateInput(SystemData& sys) { (void)sys; }

    // 出力フェーズ: SystemData → ハードウェア
    // 入力専用モジュールはオーバーライド不要（デフォルト空実装）。
    virtual void updateOutput(SystemData& sys) { (void)sys; }

    // 終了処理（バス停止、リソース解放等）。デフォルトは何もしない。
    virtual void deinit() {}

    // false の間は updateInput / updateOutput をスキップする
    // （呼び出し側の配列ループで判定する）。
    bool enabled = true;
};

#endif /* CORE_IMODULE_H_ */
