/*
 * system_init.c
 *
 *  FPU + MMU + L1 キャッシュ有効化
 *  mbed-os の system_RZ_A1H.c::SystemInit() を
 *  ベアメタル向けに移植。
 *
 *  呼び出しタイミング: main() の先頭（シリアル・GIC 初期化より前）
 *  注意: この関数内ではシリアル出力を行わないこと。
 *
 *  省略した mbed-os 処理:
 *    - RZ_A1_InitClock() / RZ_A1_InitBus(): ブートローダーが設定済み
 *    - IRQ_Initialize(): initGIC() で別途実施
 *    - L2C 有効化: 今回はスキップ (__L2C_PRESENT = 0)
 */

#include "system_init.h"
#include "rz_a1h_addr.h"
#include "core_ca.h"
#include "iodefine.h"  /* CPG.SYSCR3 アクセスに使用 */

void SystemInit(void)
{
    /* 1. FPU 有効化 (VFPv3, FPEXC.EN = 1)
     *    コンパイラが生成した VFP 命令を安全に実行するため */
    __FPU_Enable();

    /* 2. SRAM 書き込みアクセス有効化
     *    CPG.SYSCR3 = 0x0F: 全バンク書き込み許可 */
    CPG.SYSCR3 = 0x0F;

    /* 3. TLB をすべて無効化 */
    __set_TLBIALL(0);

    /* 4. 分岐予測配列をすべて無効化 */
    __set_BPIALL(0);
    __DSB();
    __ISB();

    /* 5. 命令キャッシュ無効化 & 分岐ターゲットキャッシュフラッシュ */
    __set_ICIALLU(0);
    __DSB();
    __ISB();

    /* 6. データキャッシュをすべて無効化 */
    L1C_InvalidateDCacheAll();

    /* 7. MMU 翻訳テーブル作成 (mmu_rzA1H.c) */
    MMU_CreateTranslationTable();

    /* 8. MMU 有効化 (SCTLR.M = 1, SCTLR.AFE = 1) */
    MMU_Enable();

    /* 9. L1 キャッシュ有効化 (I-cache + D-cache) */
    L1C_EnableCaches();

    /* 10. BTAC (分岐ターゲットアドレスキャッシュ) 有効化 */
    L1C_EnableBTAC();
}
