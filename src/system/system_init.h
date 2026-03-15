/*
 * system_init.h
 *
 *  FPU + MMU + L1 キャッシュ有効化
 *  main() の先頭、他の初期化より前に呼ぶこと。
 *  この時点ではシリアルは未初期化なので内部でシリアル出力を行わない。
 */

#ifndef SYSTEM_SYSTEM_INIT_H_
#define SYSTEM_SYSTEM_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MMU 翻訳テーブル作成 (mmu_rzA1H.c で実装) */
void MMU_CreateTranslationTable(void);

/* FPU 有効化 → TLB/キャッシュ無効化 → MMU 設定 → MMU 有効化 → キャッシュ有効化 */
void SystemInit(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_SYSTEM_INIT_H_ */
