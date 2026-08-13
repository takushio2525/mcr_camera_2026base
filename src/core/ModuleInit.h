/*
 * ModuleInit.h
 *
 *  IModule::init() の戻り値契約を実装する小さなヘルパ。
 *
 *  IModule.h は「init() が false を返したら呼び出し側で enabled = false に
 *  落とす」運用を前提に設計されているが、その判定を各呼び出し箇所に
 *  手書きすると必ず書き忘れが出る（実際、本ヘッダ導入前は main() が
 *  全ての init() の戻り値を捨てており、enabled に false を書く箇所が
 *  一つも存在しなかった）。契約を 1 箇所に閉じ込めるためのヘッダ。
 *
 *  ハードウェアに一切触れないので、ホスト側ユニットテスト
 *  (tests/test_module_contract.cpp) から直接検証できる。
 */

#ifndef CORE_MODULE_INIT_H_
#define CORE_MODULE_INIT_H_

#include "IModule.h"

// init() を呼び、その結果をそのまま enabled に反映する。
//
// 失敗 (false) の場合 enabled = false となり、以後そのモジュールは
// 3 フェーズ実行から外れる:
//   - ISR 配列 (mcr_camera_2026base.cpp の s_inputModules / s_outputModules)
//     のループが enabled を見てスキップする
//   - 配列に載っていない LineDetector / SDLogger は、メインループ側の
//     呼び出しが同じく enabled を見てスキップする
//
// 戻り値は init() の結果そのもの。呼び出し側はこれを見て
// シリアル出力等の障害報告を行う。
inline bool initModule(IModule& mod)
{
    const bool ok = mod.init();
    mod.enabled   = ok;
    return ok;
}

#endif /* CORE_MODULE_INIT_H_ */
