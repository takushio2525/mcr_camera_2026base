/*
 * mmu_rzA1H.c
 *
 *  MMU 翻訳テーブル作成 (RZ/A1H / GR-PEACH)
 *  mbed-os の mmu_RZ_A1H.c を GCC リンカシンボル向けに移植。
 *
 *  主な変更点:
 *    Image$$TTB$$ZI$$Base      → __ttb_base   (リンカスクリプトで定義)
 *    Image$$RW_DATA_NC$$Base   → __nc_bss_start (同)
 *    RZ_A1H.h                  → rz_a1h_addr.h (ローカル)
 */

#include "rz_a1h_addr.h"  /* デバイスパラメータ + アドレス定数 */
#include "core_ca.h"       /* CMSIS Cortex-A MMU/キャッシュ制御 */

/* リンカスクリプトが定義するシンボル */
extern uint32_t __ttb_base;       /* TTB 先頭 (0x20000000, 16KB アライン) */
extern uint32_t __nc_bss_start;   /* NC BSS 先頭 (0x60020000) */
extern uint32_t __nc_bss_end;     /* NC BSS 末尾 */

/* セクションディスクリプタ変数 */
static uint32_t Sect_Normal;      /* WB/WA, 実行可, RW */
static uint32_t Sect_Normal_NC;   /* 非キャッシュ, RW */
static uint32_t Sect_Normal_Cod;  /* WB/WA, 実行可, RO (コード用) */
static uint32_t Sect_Normal_RO;   /* WB/WA, 実行不可, RO */
static uint32_t Sect_Normal_RW;   /* WB/WA, 実行不可, RW */
static uint32_t Sect_Device_RO;   /* デバイス, RO */
static uint32_t Sect_Device_RW;   /* デバイス, RW */

/* ページディスクリプタ変数 (ペリフェラル細粒度マップ用) */
static uint32_t Page_L1_4k  = 0x0;
static uint32_t Page_L1_64k = 0x0;
static uint32_t Page_4k_Device_RW;
static uint32_t Page_64k_Device_RW;

void MMU_CreateTranslationTable(void)
{
    mmu_region_attributes_Type region;

    /* ---- ディスクリプタ生成 ---- */
    section_normal     (Sect_Normal,     region);
    section_normal_cod (Sect_Normal_Cod, region);
    section_normal_ro  (Sect_Normal_RO,  region);
    section_normal     (Sect_Normal_RW,  region);
    section_device_ro  (Sect_Device_RO,  region);
    section_device_rw  (Sect_Device_RW,  region);
    section_normal_nc  (Sect_Normal_NC,  region);
    page64k_device_rw  (Page_L1_64k, Page_64k_Device_RW, region);
    page4k_device_rw   (Page_L1_4k,  Page_4k_Device_RW,  region);

    /* ---- フラットマップ設定 ---- */

    /* 4GB 全体をフォルト (未定義アクセスは即 Data Abort) */
    MMU_TTSection(&__ttb_base, 0, 4096, DESCRIPTOR_FAULT);

    /* SPI Flash I/O 0 (0x18000000, 64MB): 命令キャッシュ可, 実行可
     * XIP コードをキャッシュすることで命令フェッチを高速化 */
    MMU_TTSection(&__ttb_base, RZ_A1_SPI_IO0, 64, Sect_Normal_Cod);

    /* SPI Flash I/O 1 (0x1C000000, 64MB): 同上 */
    MMU_TTSection(&__ttb_base, RZ_A1_SPI_IO1, 64, Sect_Normal_Cod);

    /* On-Chip SRAM (0x20000000, 10MB): WB/WA キャッシュ, RW
     * TTB (0x20000000-0x20003FFF) もこの範囲に含まれるが問題なし */
    MMU_TTSection(&__ttb_base, RZ_A1_ONCHIP_SRAM_BASE, 10, Sect_Normal_RW);

    /* SPI マルチ I/O ベース (0x3FE00000, 1MB): デバイス属性 */
    MMU_TTSection(&__ttb_base, RZ_A1_SPI_MIO_BASE, 1, Sect_Device_RW);

    /* BSC ベース (0x3FF00000, 1MB): デバイス属性 */
    MMU_TTSection(&__ttb_base, RZ_A1_BSC_BASE, 1, Sect_Device_RW);

    /* On-Chip SRAM NC ミラー (0x60000000, 1MB): 非キャッシュ
     * フレームバッファ (0x60020000) を DMA 書き込み可能にする */
    MMU_TTSection(&__ttb_base, RZ_A1_ONCHIP_SRAM_NC_BASE, 1, Sect_Normal_NC);

    /* ペリフェラル BASE0 (0xE8000000, 3MB): デバイス属性 */
    MMU_TTSection(&__ttb_base, RZ_A1_PERIPH_BASE0, 3, Sect_Device_RW);

    /* ペリフェラル BASE1 (0xFCF00000, 49MB): デバイス属性 */
    MMU_TTSection(&__ttb_base, RZ_A1_PERIPH_BASE1, 49, Sect_Device_RW);

    /* ---- TTBR0 設定 ----
     * bits[1:0]=01 (IRGN[1]=0, IRGN[0]=1 = Inner WB/WA)
     * bits[4:3]=01 (RGN = Outer WB/WA)
     * = 0b01001 = 9 */
    __set_TTBR0((uint32_t)&__ttb_base | 9);
    __ISB();

    /* ---- DACR 設定: Domain 0 = Client (全アクセス許可) ---- */
    __set_DACR(1);
    __ISB();
}
