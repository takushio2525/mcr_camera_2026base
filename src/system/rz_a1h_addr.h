/*
 * rz_a1h_addr.h
 *
 *  RZ/A1H デバイス設定マクロ + メモリマップ定数
 *  mmu_rzA1H.c および system_init.c から参照される。
 *  mbed-os の RZ_A1H.h から必要な定義のみを抽出・簡略化。
 */

#ifndef SYSTEM_RZ_A1H_ADDR_H_
#define SYSTEM_RZ_A1H_ADDR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- core_ca.h 向けデバイスパラメータ ---- */
#define __CA_REV        0x0000U   /* Cortex-A9 r0p0 */
#define __FPU_PRESENT   1U        /* VFPv3-D16 搭載 */
#define __GIC_PRESENT   1U
#define __TIM_PRESENT   1U
#define __L2C_PRESENT   0U        /* L2C は今回有効化しない */

/* ---- RZ/A1H メモリマップ ---- */
#define RZ_A1_SPI_IO0              (0x18000000UL)  /* SPI Flash I/O 0 (64MB) */
#define RZ_A1_SPI_IO1              (0x1C000000UL)  /* SPI Flash I/O 1 (64MB) */
#define RZ_A1_ONCHIP_SRAM_BASE     (0x20000000UL)  /* On-Chip SRAM (10MB) */
#define RZ_A1_ONCHIP_SRAM_NC_BASE  (0x60000000UL)  /* On-Chip SRAM NC ミラー */
#define RZ_A1_SPI_MIO_BASE         (0x3FE00000UL)  /* SPI マルチ I/O ベース */
#define RZ_A1_BSC_BASE             (0x3FF00000UL)  /* BSC ベース */
#define RZ_A1_PERIPH_BASE0         (0xE8000000UL)  /* ペリフェラル BASE0 */
#define RZ_A1_PERIPH_BASE1         (0xFCF00000UL)  /* ペリフェラル BASE1 */

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_RZ_A1H_ADDR_H_ */
