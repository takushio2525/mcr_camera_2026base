#ifndef PINMAP_H
#define PINMAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Dummy definisions for PinName
typedef enum {
  P8_11 = 0,
  P10_0,
  P1_12,
  P1_13,
  P1_1,
  P1_0,
  P1_9,
  P2_15,
  P4_7,
  P5_7,
  P1_8,
  P2_14,
  P4_6,
  P5_6,
  P1_7,
  P2_13,
  P4_5,
  P5_5,
  P1_6,
  P4_4,
  P5_4,
  P10_5,
  P2_10,
  P10_14,
  P2_9,
  P10_13,
  P10_12,
  P2_7,
  P2_6,
  P2_5,
  P2_4,
  P2_3,
  P2_2,
  P2_1,
  P2_0,
  P10_1,
  P10_2,
  P10_3,
  P10_4,
  P10_7,
  P10_6,
  P10_8,
  P10_9,
  P10_10,
  P10_11,
  P4_0,
  P10_15,
  P3_15,
  P11_0,
  P3_14,
  P11_1,
  P3_13,
  P11_2,
  P3_12,
  P11_3,
  P3_11,
  P11_4,
  P3_10,
  P11_5,
  P3_9,
  P11_6,
  P3_8,
  P11_7,
  P11_10,
  P11_11,
  P11_12,
  P3_2,
  P11_13,
  P11_14,
  P11_15,
  P5_3,
  P5_2,
  P5_1,
  P5_0,
  NC = (int)0xFFFFFFFF
} PinName;

typedef struct {
  PinName pin;
  int peripheral;
  int function;
} PinMap;

static inline void pinmap_peripheral(PinName pin, const PinMap *map) {}
static inline void pinmap_pinout(PinName pin, const PinMap *map) {}

// Dummy definisions for interrupts
typedef uint32_t IRQn_Type;

// Mbed OS Specific IRQn definitions used in gr_peach_vdc5.c
enum {
  S0_VI_VSYNC0_IRQn = 0,
  S0_LO_VSYNC0_IRQn,
  S0_VSYNCERR0_IRQn,
  GR3_VLINE0_IRQn,
  S0_VFIELD0_IRQn,
  IV1_VBUFERR0_IRQn,
  IV3_VBUFERR0_IRQn,
  IV5_VBUFERR0_IRQn,
  IV6_VBUFERR0_IRQn,
  S0_WLINE0_IRQn,
  S1_VI_VSYNC0_IRQn,
  S1_LO_VSYNC0_IRQn,
  S1_VSYNCERR0_IRQn,
  S1_VFIELD0_IRQn,
  IV2_VBUFERR0_IRQn,
  IV4_VBUFERR0_IRQn,
  S1_WLINE0_IRQn,
  OIR_VI_VSYNC0_IRQn,
  OIR_LO_VSYNC0_IRQn,
  OIR_VSYNCERR0_IRQn,
  OIR_VFIELD0_IRQn,
  IV7_VBUFERR0_IRQn,
  IV8_VBUFERR0_IRQn
};

typedef void (*IRQHandler)(uint32_t);

// GR-PEACH (RZ/A1H) GIC Registers
#include "iodefine.h"

// 実際の割り込みベクタの外部参照
extern void (*g_irq_handlers[256])(uint32_t);

// mbedのIRQn(0〜)と実際のGIC IRQ IDの対応表
// generate/vects.c の S0_VI_VSYNC0 等のオフセット(0x012C / 4 = 75)に準拠
static const uint32_t gic_irq_map[] = {
    75, // S0_VI_VSYNC0_IRQn
    76, // S0_LO_VSYNC0_IRQn
    77, // S0_VSYNCERR0_IRQn
    78, // GR3_VLINE0_IRQn
    79, // S0_VFIELD0_IRQn
    80, // IV1_VBUFERR0_IRQn
    81, // IV3_VBUFERR0_IRQn
    82, // IV5_VBUFERR0_IRQn
    83, // IV6_VBUFERR0_IRQn
    84, // S0_WLINE0_IRQn
    85, // S1_VI_VSYNC0_IRQn
    86, // S1_LO_VSYNC0_IRQn
    87, // S1_VSYNCERR0_IRQn
    88, // S1_VFIELD0_IRQn
    89, // IV2_VBUFERR0_IRQn
    90, // IV4_VBUFERR0_IRQn
    91, // S1_WLINE0_IRQn
    92, // OIR_VI_VSYNC0_IRQn
    93, // OIR_LO_VSYNC0_IRQn
    94, // OIR_VSYNCERR0_IRQn
    95, // OIR_VFIELD0_IRQn
    96, // IV7_VBUFERR0_IRQn
    97  // IV8_VBUFERR0_IRQn
};

static inline void InterruptHandlerRegister(IRQn_Type irq,
                                            void (*handler)(uint32_t)) {
  if (irq < (sizeof(gic_irq_map) / sizeof(gic_irq_map[0]))) {
    uint32_t gic_id = gic_irq_map[irq];
    if (gic_id < 256) {
      g_irq_handlers[gic_id] = handler;
    }
  }
}

static inline void GIC_SetPriority(IRQn_Type irq, uint32_t priority) {
  if (irq < (sizeof(gic_irq_map) / sizeof(gic_irq_map[0]))) {
    uint32_t gic_id = gic_irq_map[irq];
    uint32_t reg_idx = gic_id / 4;
    uint32_t bit_pos = (gic_id % 4) * 8;
    volatile uint32_t *icdipr = (volatile uint32_t *)&INTC.ICDIPR0 + reg_idx;
    uint32_t val = *icdipr;
    val &= ~(0xFF << bit_pos);
    val |= (priority << (bit_pos + 3)); // RZ/A1H GIC priority is top 5 bits
    *icdipr = val;
  }
}

static inline void GIC_EnableIRQ(IRQn_Type irq) {
  if (irq < (sizeof(gic_irq_map) / sizeof(gic_irq_map[0]))) {
    uint32_t gic_id = gic_irq_map[irq];
    uint32_t reg_idx = gic_id / 32;
    uint32_t bit_pos = gic_id % 32;

    // エッジ/レベル設定 (ICDICFR) - VDC5割り込みはレベルトリガが基本(0b00)
    volatile uint32_t *icdicfr =
        (volatile uint32_t *)&INTC.ICDICFR0 + (gic_id / 16);
    uint32_t icf_bit = (gic_id % 16) * 2;
    *icdicfr &= ~(3 << icf_bit);

    // ターゲットCPU設定 (ICDIPTR) - CPU0
    volatile uint32_t *icdiptr =
        (volatile uint32_t *)&INTC.ICDIPTR0 + (gic_id / 4);
    uint32_t pt_bit = (gic_id % 4) * 8;
    *icdiptr &= ~(0xFF << pt_bit);
    *icdiptr |= (0x01 << pt_bit);

    // 割り込みイネーブル (ICDISER)
    volatile uint32_t *icdiser = (volatile uint32_t *)&INTC.ICDISER0 + reg_idx;
    *icdiser |= (1 << bit_pos);
  }
}

static inline void GIC_DisableIRQ(IRQn_Type irq) {
  if (irq < (sizeof(gic_irq_map) / sizeof(gic_irq_map[0]))) {
    uint32_t gic_id = gic_irq_map[irq];
    uint32_t reg_idx = gic_id / 32;
    uint32_t bit_pos = gic_id % 32;
    volatile uint32_t *icdicer = (volatile uint32_t *)&INTC.ICDICER0 + reg_idx;
    *icdicer |= (1 << bit_pos);
  }
}

#ifdef __cplusplus
}
#endif

#endif // PINMAP_H
