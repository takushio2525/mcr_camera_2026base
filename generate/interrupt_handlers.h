                                                                          
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
                                                                           
/************************************************************************/
/*    File Version: V1.02                                               */
/*    Date Generated: 30/05/2019                                        */
/************************************************************************/

#ifndef INTERRUPT_HANDLERS_H
#define INTERRUPT_HANDLERS_H

/* Undefined Instruction */
void INT_Excep_UndefinedInst(void) __attribute__((interrupt, used));

/* SWI*/
void INT_Excep_SWI(void) __attribute__((interrupt, used));

/* PREFETCH_ABORT*/
void INT_Excep_PREFETCH_ABORT(void) __attribute__((interrupt, used));

/* DATA_ABORT*/
void INT_Excep_DATA_ABORT(void) __attribute__((interrupt, used));

/* Reserved*/
void INT_Excep_Reserved(void) __attribute__((used));

/* Reserved*/
void INT_Excep_Reserved(void) __attribute__((used));

/* IRQ*/
void INT_Excep_IRQ(void) __attribute__((interrupt, used));

/* FIQ*/
void INT_Excep_FIQ(void) __attribute__((interrupt, used));

/* GIC0*/
void INT_Excep_GIC0(void) __attribute__((used));

/* GIC1*/
void INT_Excep_GIC1(void) __attribute__((used));

/* GIC2*/
void INT_Excep_GIC2(void) __attribute__((used));

/* GIC3*/
void INT_Excep_GIC3(void) __attribute__((used));

/* GIC4*/
void INT_Excep_GIC4(void) __attribute__((used));

/* GIC5*/
void INT_Excep_GIC5(void) __attribute__((used));

/* GIC6*/
void INT_Excep_GIC6(void) __attribute__((used));

/* GIC7*/
void INT_Excep_GIC7(void) __attribute__((used));

/* GIC8*/
void INT_Excep_GIC8(void) __attribute__((used));

/* GIC9*/
void INT_Excep_GIC9(void) __attribute__((used));

/* GIC10*/
void INT_Excep_GIC10(void) __attribute__((used));

/* GIC11*/
void INT_Excep_GIC11(void) __attribute__((used));

/* GIC12*/
void INT_Excep_GIC12(void) __attribute__((used));

/* GIC13*/
void INT_Excep_GIC13(void) __attribute__((used));

/* GIC14*/
void INT_Excep_GIC14(void) __attribute__((used));

/* GIC15*/
void INT_Excep_GIC15(void) __attribute__((used));

/* CPU_PMUIRQ0*/
void INT_Excep_CPU_PMUIRQ0(void) __attribute__((used));

/* CPU_COMMRX0*/
void INT_Excep_CPU_COMMRX0(void) __attribute__((used));

/* CPU_COMMTX0*/
void INT_Excep_CPU_COMMTX0(void) __attribute__((used));

/* CPU_CTIIRQ0*/
void INT_Excep_CPU_CTIIRQ0(void) __attribute__((used));

/* vector 20 reserved */
/* vector 21 reserved */
/* vector 22 reserved */
/* vector 23 reserved */
/* vector 24 reserved */
/* vector 25 reserved */
/* vector 26 reserved */
/* vector 27 reserved */
/* vector 28 reserved */
/* vector 29 reserved */
/* vector 30 reserved */
/* vector 31 reserved */

/* IRQ0*/
void INT_Excep_IRQ0(void) __attribute__((used));

/* IRQ1*/
void INT_Excep_IRQ1(void) __attribute__((used));

/* IRQ2*/
void INT_Excep_IRQ2(void) __attribute__((used));

/* IRQ3*/
void INT_Excep_IRQ3(void) __attribute__((used));

/* IRQ4*/
void INT_Excep_IRQ4(void) __attribute__((used));

/* IRQ5*/
void INT_Excep_IRQ5(void) __attribute__((used));

/* IRQ6*/
void INT_Excep_IRQ6(void) __attribute__((used));

/* IRQ7*/
void INT_Excep_IRQ7(void) __attribute__((used));

/* PL310ERR*/
void INT_Excep_PL310ERR(void) __attribute__((used));

/* DMAINT0*/
void INT_Excep_DMAINT0(void) __attribute__((used));

/* DMAINT1*/
void INT_Excep_DMAINT1(void) __attribute__((used));

/* DMAINT2*/
void INT_Excep_DMAINT2(void) __attribute__((used));

/* DMAINT3*/
void INT_Excep_DMAINT3(void) __attribute__((used));

/* DMAINT4*/
void INT_Excep_DMAINT4(void) __attribute__((used));

/* DMAINT5*/
void INT_Excep_DMAINT5(void) __attribute__((used));

/* DMAINT6*/
void INT_Excep_DMAINT6(void) __attribute__((used));

/* DMAINT7*/
void INT_Excep_DMAINT7(void) __attribute__((used));

/* DMAINT8*/
void INT_Excep_DMAINT8(void) __attribute__((used));

/* DMAINT9*/
void INT_Excep_DMAINT9(void) __attribute__((used));

/* DMAINT10*/
void INT_Excep_DMAINT10(void) __attribute__((used));

/* DMAINT11*/
void INT_Excep_DMAINT11(void) __attribute__((used));

/* DMAINT12*/
void INT_Excep_DMAINT12(void) __attribute__((used));

/* DMAINT13*/
void INT_Excep_DMAINT13(void) __attribute__((used));

/* DMAINT14*/
void INT_Excep_DMAINT14(void) __attribute__((used));

/* DMAINT15*/
void INT_Excep_DMAINT15(void) __attribute__((used));

/* DMAERR*/
void INT_Excep_DMAERR(void) __attribute__((used));

/* vector 58 reserved */
/* vector 59 reserved */
/* vector 60 reserved */
/* vector 61 reserved */
/* vector 62 reserved */
/* vector 63 reserved */
/* vector 64 reserved */
/* vector 65 reserved */
/* vector 66 reserved */
/* vector 67 reserved */
/* vector 68 reserved */
/* vector 69 reserved */
/* vector 70 reserved */
/* vector 71 reserved */
/* vector 72 reserved */

/* USBI0*/
void INT_Excep_USBI0(void) __attribute__((used));

/* USBI1*/
void INT_Excep_USBI1(void) __attribute__((used));

/* S0_VI_VSYNC0*/
void INT_Excep_S0_VI_VSYNC0(void) __attribute__((used));

/* S0_LO_VSYNC0*/
void INT_Excep_S0_LO_VSYNC0(void) __attribute__((used));

/* S0_VSYNCERR0*/
void INT_Excep_S0_VSYNCERR0(void) __attribute__((used));

/* GR3_VLINE0*/
void INT_Excep_GR3_VLINE0(void) __attribute__((used));

/* S0_VFIELD0*/
void INT_Excep_S0_VFIELD0(void) __attribute__((used));

/* IV1_VBUFERR0*/
void INT_Excep_IV1_VBUFERR0(void) __attribute__((used));

/* IV3_VBUFERR0*/
void INT_Excep_IV3_VBUFERR0(void) __attribute__((used));

/* IV5_VBUFERR0*/
void INT_Excep_IV5_VBUFERR0(void) __attribute__((used));

/* IV6_VBUFERR0*/
void INT_Excep_IV6_VBUFERR0(void) __attribute__((used));

/* S0_WLINE0*/
void INT_Excep_S0_WLINE0(void) __attribute__((used));

/* S1_VI_VSYNC0*/
void INT_Excep_S1_VI_VSYNC0(void) __attribute__((used));

/* S1_LO_VSYNC0*/
void INT_Excep_S1_LO_VSYNC0(void) __attribute__((used));

/* S1_VSYNCERR0*/
void INT_Excep_S1_VSYNCERR0(void) __attribute__((used));

/* S1_VFIELD0*/
void INT_Excep_S1_VFIELD0(void) __attribute__((used));

/* IV2_VBUFERR0*/
void INT_Excep_IV2_VBUFERR0(void) __attribute__((used));

/* IV4_VBUFERR0*/
void INT_Excep_IV4_VBUFERR0(void) __attribute__((used));

/* S1_WLINE0*/
void INT_Excep_S1_WLINE0(void) __attribute__((used));

/* OIR_VI_VSYNC0*/
void INT_Excep_OIR_VI_VSYNC0(void) __attribute__((used));

/* OIR_LO_VSYNC0*/
void INT_Excep_OIR_LO_VSYNC0(void) __attribute__((used));

/* OIR_VSYNCERR0*/
void INT_Excep_OIR_VSYNCERR0(void) __attribute__((used));

/* OIR_VFIELD0*/
void INT_Excep_OIR_VFIELD0(void) __attribute__((used));

/* IV7_VBUFERR0*/
void INT_Excep_IV7_VBUFERR0(void) __attribute__((used));

/* IV8_VBUFERR0*/
void INT_Excep_V8_VBUFERR0(void) __attribute__((used));

/* OIR_WLINE0*/
void INT_Excep_OIR_WLINE0(void) __attribute__((used));

/* S0_VI_VSYNC1*/
void INT_Excep_S0_VI_VSYNC1(void) __attribute__((used));

/* S0_LO_VSYNC1*/
void INT_Excep_S0_LO_VSYNC1(void) __attribute__((used));

/* S0_VSYNCERR1*/
void INT_Excep_S0_VSYNCERR1(void) __attribute__((used));

/* GR3_VLINE1*/
void INT_Excep_GR3_VLINE1(void) __attribute__((used));

/* S0_VFIELD1*/
void INT_Excep_S0_VFIELD1(void) __attribute__((used));

/* IV1_VBUFERR1*/
void INT_Excep_IV1_VBUFERR1(void) __attribute__((used));

/* IV3_VBUFERR1*/
void INT_Excep_IV3_VBUFERR1(void) __attribute__((used));

/* IV5_VBUFERR1*/
void INT_Excep_IV5_VBUFERR1(void) __attribute__((used));

/* IV6_VBUFERR1*/
void INT_Excep_IV6_VBUFERR1(void) __attribute__((used));

/* S0_WLINE1*/
void INT_Excep_S0_WLINE1(void) __attribute__((used));

/* S1_VI_VSYNC1*/
void INT_Excep_S1_VI_VSYNC1(void) __attribute__((used));

/* S1_LO_VSYNC1*/
void INT_Excep_S1_LO_VSYNC1(void) __attribute__((used));

/* S1_VSYNCERR1*/
void INT_Excep_S1_VSYNCERR1(void) __attribute__((used));

/* S1_VFIELD1*/
void INT_Excep_S1_VFIELD1(void) __attribute__((used));

/* IV2_VBUFERR1*/
void INT_Excep_IV2_VBUFERR1(void) __attribute__((used));

/* IV4_VBUFERR1*/
void INT_Excep_IV4_VBUFERR1(void) __attribute__((used));

/* S1_WLINE1*/
void INT_Excep_S1_WLINE1(void) __attribute__((used));

/* OIR_VI_VSYNC1*/
void INT_Excep_OIR_VI_VSYNC1(void) __attribute__((used));

/* OIR_LO_VSYNC1*/
void INT_Excep_OIR_LO_VSYNC1(void) __attribute__((used));

/* OIR_VLINE1*/
void INT_Excep_OIR_VLINE1(void) __attribute__((used));

/* OIR_VFIELD1*/
void INT_Excep_OIR_VFIELD1(void) __attribute__((used));

/* IV7_VBUFERR1*/
void INT_Excep_IV7_VBUFERR1(void) __attribute__((used));

/* IV8_VBUFERR1*/
void INT_Excep_IV8_VBUFERR1(void) __attribute__((used));

/* OIR_WLINE1*/
void INT_Excep_OIR_WLINE1(void) __attribute__((used));

/* IMRDI*/
void INT_Excep_IMRDI(void) __attribute__((used));

/* IMR2I0*/
void INT_Excep_IMR2I0(void) __attribute__((used));

/* IMR2I1*/
void INT_Excep_IMR2I1(void) __attribute__((used));

/* JEDI*/
void INT_Excep_JEDI(void) __attribute__((used));

/* JDTI*/
void INT_Excep_JDTI(void) __attribute__((used));

/* CMP0*/
void INT_Excep_CMP0(void) __attribute__((used));

/* CMP1*/
void INT_Excep_CMP1(void) __attribute__((used));

/* INT0*/
void INT_Excep_INT0(void) __attribute__((used));

/* INT1*/
void INT_Excep_INT1(void) __attribute__((used));

/* INT2*/
void INT_Excep_INT2(void) __attribute__((used));

/* INT3*/
void INT_Excep_INT3(void) __attribute__((used));

/* OSTMI0*/
void INT_Excep_OSTMI0(void) __attribute__((used));

/* OSTMI1*/
void INT_Excep_OSTMI1(void) __attribute__((used));

/* CMI*/
void INT_Excep_CMI(void) __attribute__((used));

/* WTOUT*/
void INT_Excep_WTOUT(void) __attribute__((used));

/* ITI*/
void INT_Excep_ITI(void) __attribute__((used));

/* TGI0A*/
void INT_Excep_TGI0A(void) __attribute__((used));

/* TGI0B*/
void INT_Excep_TGI0B(void) __attribute__((used));

/* TGI0C*/
void INT_Excep_TGI0C(void) __attribute__((used));

/* TGI0D*/
void INT_Excep_TGI0D(void) __attribute__((used));

/* TGI0V*/
void INT_Excep_TGI0V(void) __attribute__((used));

/* TGI0E*/
void INT_Excep_TGI0E(void) __attribute__((used));

/* TGI0F*/
void INT_Excep_TGI0F(void) __attribute__((used));

/* TGI1A*/
void INT_Excep_TGI1A(void) __attribute__((used));

/* TGI1B*/
void INT_Excep_TGI1B(void) __attribute__((used));

/* TGI1V*/
void INT_Excep_TGI1V(void) __attribute__((used));

/* TGI1U*/
void INT_Excep_TGI1U(void) __attribute__((used));

/* TGI2A*/
void INT_Excep_TGI2A(void) __attribute__((used));

/* TGI2B*/
void INT_Excep_TGI2B(void) __attribute__((used));

/* TGI2V*/
void INT_Excep_TGI2V(void) __attribute__((used));

/* TGI2U*/
void INT_Excep_TGI2U(void) __attribute__((used));

/* TGI3A*/
void INT_Excep_TGI3A(void) __attribute__((used));

/* TGI3B*/
void INT_Excep_TGI3B(void) __attribute__((used));

/* TGI3C*/
void INT_Excep_TGI3C(void) __attribute__((used));

/* TGI3D*/
void INT_Excep_TGI3D(void) __attribute__((used));

/* TGI3V*/
void INT_Excep_TGI3V(void) __attribute__((used));

/* TGI4A*/
void INT_Excep_TGI4A(void) __attribute__((used));

/* TGI4B*/
void INT_Excep_TGI4B(void) __attribute__((used));

/* TGI4C*/
void INT_Excep_TGI4C(void) __attribute__((used));

/* TGI4D*/
void INT_Excep_TGI4D(void) __attribute__((used));

/* TGI4V*/
void INT_Excep_TGI4V(void) __attribute__((used));

/* CMI1*/
void INT_Excep_CMI1(void) __attribute__((used));

/* CMI2*/
void INT_Excep_CMI2(void) __attribute__((used));

/* SGDEI0*/
void INT_Excep_SGDEI0(void) __attribute__((used));

/* SGDEI1*/
void INT_Excep_SGDEI1(void) __attribute__((used));

/* SGDEI2*/
void INT_Excep_SGDEI2(void) __attribute__((used));

/* SGDEI3*/
void INT_Excep_SGDEI3(void) __attribute__((used));

/* ADI*/
void INT_Excep_ADI(void) __attribute__((used));

/* ADWAR*/
void INT_Excep_ADWAR(void) __attribute__((used));

/* SSII0*/
void INT_Excep_SSII0(void) __attribute__((used));

/* SSIRXI0*/
void INT_Excep_SSIRXI0(void) __attribute__((used));

/* SSITXI0*/
void INT_Excep_SSITXI0(void) __attribute__((used));

/* SSII1*/
void INT_Excep_SSII1(void) __attribute__((used));

/* SSIRXI1*/
void INT_Excep_SSIRXI1(void) __attribute__((used));

/* SSITXI1*/
void INT_Excep_SSITXI1(void) __attribute__((used));

/* SSII2*/
void INT_Excep_SSII2(void) __attribute__((used));

/* SSIRTI2*/
void INT_Excep_SSIRTI2(void) __attribute__((used));

/* SSII3*/
void INT_Excep_SSII3(void) __attribute__((used));

/* SSIRXI3*/
void INT_Excep_SSIRXI3(void) __attribute__((used));

/* SSITXI3*/
void INT_Excep_SSITXI3(void) __attribute__((used));

/* SSII4*/
void INT_Excep_SSII4(void) __attribute__((used));

/* SSIRTI4*/
void INT_Excep_SSIRTI4(void) __attribute__((used));

/* SSII5*/
void INT_Excep_SSII5(void) __attribute__((used));

/* SSIRXI5*/
void INT_Excep_SSIRXI5(void) __attribute__((used));

/* SSITXI5*/
void INT_Excep_SSITXI5(void) __attribute__((used));

/* SPDIFI*/
void INT_Excep_SPDIFI(void) __attribute__((used));

/* I2C_TEI0*/
void INT_Excep_I2C_TEI0(void) __attribute__((used));

/* RI0*/
void INT_Excep_RI0(void) __attribute__((used));

/* TI0*/
void INT_Excep_TI0(void) __attribute__((used));

/* SPI0*/
void INT_Excep_SPI0(void) __attribute__((used));

/* STI0*/
void INT_Excep_STI0(void) __attribute__((used));

/* NAKI0*/
void INT_Excep_NAKI0(void) __attribute__((used));

/* ALI0*/
void INT_Excep_ALI0(void) __attribute__((used));

/* TMOI0*/
void INT_Excep_TMOI0(void) __attribute__((used));

/* I2C_TEI1*/
void INT_Excep_I2C_TEI1(void) __attribute__((used));

/* RI1*/
void INT_Excep_RI1(void) __attribute__((used));

/* TI1*/
void INT_Excep_TI1(void) __attribute__((used));

/* SPI1*/
void INT_Excep_SPI1(void) __attribute__((used));

/* STI1*/
void INT_Excep_STI1(void) __attribute__((used));

/* NAKI1*/
void INT_Excep_NAKI1(void) __attribute__((used));

/* ALI1*/
void INT_Excep_ALI1(void) __attribute__((used));

/* TMOI1*/
void INT_Excep_TMOI1(void) __attribute__((used));

/* TEI2*/
void INT_Excep_TEI2(void) __attribute__((used));

/* RI2*/
void INT_Excep_RI2(void) __attribute__((used));

/* TI2*/
void INT_Excep_TI2(void) __attribute__((used));

/* SPI2*/
void INT_Excep_SPI2(void) __attribute__((used));

/* STI2*/
void INT_Excep_STI2(void) __attribute__((used));

/* NAKI2*/
void INT_Excep_NAKI2(void) __attribute__((used));

/* ALI2*/
void INT_Excep_ALI2(void) __attribute__((used));

/* TMOI2*/
void INT_Excep_TMOI2(void) __attribute__((used));

/* TEI3*/
void INT_Excep_TEI3(void) __attribute__((used));

/* RI3*/
void INT_Excep_RI3(void) __attribute__((used));

/* TI3*/
void INT_Excep_TI3(void) __attribute__((used));

/* SPI3*/
void INT_Excep_SPI3(void) __attribute__((used));

/* STI3*/
void INT_Excep_STI3(void) __attribute__((used));

/* NAKI3*/
void INT_Excep_NAKI3(void) __attribute__((used));

/* ALI3*/
void INT_Excep_ALI3(void) __attribute__((used));

/* TMOI3*/
void INT_Excep_TMOI3(void) __attribute__((used));

/* BRI0*/
void INT_Excep_FIFO_BRI0(void) __attribute__((used));

/* ERI0*/
void INT_Excep_FIFO_ERI0(void) __attribute__((used));

/* RXI0*/
void INT_Excep_FIFO_RXI0(void) __attribute__((used));

/* TXI0*/
void INT_Excep_FIFO_TXI0(void) __attribute__((used));

/* BRI1*/
void INT_Excep_FIFO_BRI1(void) __attribute__((used));

/* ERI1*/
void INT_Excep_FIFO_ERI1(void) __attribute__((used));

/* RXI1*/
void INT_Excep_FIFO_RXI1(void) __attribute__((used));

/* TXI1*/
void INT_Excep_FIFO_TXI1(void) __attribute__((used));

/* BRI2*/
void INT_Excep_BRI2(void) __attribute__((used));

/* ERI2*/
void INT_Excep_ERI2(void) __attribute__((used));

/* RXI2*/
void INT_Excep_RXI2(void) __attribute__((used));

/* TXI2*/
void INT_Excep_TXI2(void) __attribute__((used));

/* BRI3*/
void INT_Excep_BRI3(void) __attribute__((used));

/* ERI3*/
void INT_Excep_ERI3(void) __attribute__((used));

/* RXI3*/
void INT_Excep_RXI3(void) __attribute__((used));

/* TXI3*/
void INT_Excep_TXI3(void) __attribute__((used));

/* BRI4*/
void INT_Excep_BRI4(void) __attribute__((used));

/* ERI4*/
void INT_Excep_ERI4(void) __attribute__((used));

/* RXI4*/
void INT_Excep_RXI4(void) __attribute__((used));

/* TXI4*/
void INT_Excep_TXI4(void) __attribute__((used));

/* BRI5*/
void INT_Excep_BRI5(void) __attribute__((used));

/* ERI5*/
void INT_Excep_ERI5(void) __attribute__((used));

/* RXI5*/
void INT_Excep_RXI5(void) __attribute__((used));

/* TXI5*/
void INT_Excep_TXI5(void) __attribute__((used));

/* BRI6*/
void INT_Excep_BRI6(void) __attribute__((used));

/* ERI6*/
void INT_Excep_ERI6(void) __attribute__((used));

/* RXI6*/
void INT_Excep_RXI6(void) __attribute__((used));

/* TXI6*/
void INT_Excep_TXI6(void) __attribute__((used));

/* BRI7*/
void INT_Excep_BRI7(void) __attribute__((used));

/* ERI7*/
void INT_Excep_ERI7(void) __attribute__((used));

/* RXI7*/
void INT_Excep_RXI7(void) __attribute__((used));

/* TXI7*/
void INT_Excep_TXI7(void) __attribute__((used));

/* GERI*/
void INT_Excep_GERI(void) __attribute__((used));

/* RFI*/
void INT_Excep_RFI(void) __attribute__((used));

/* CFRXI0*/
void INT_Excep_CFRXI0(void) __attribute__((used));

/* CERI0*/
void INT_Excep_CERI0(void) __attribute__((used));

/* CTXI0*/
void INT_Excep_CTXI0(void) __attribute__((used));

/* CFRXI1*/
void INT_Excep_CFRXI1(void) __attribute__((used));

/* CERI1*/
void INT_Excep_CERI1(void) __attribute__((used));

/* CTXI1*/
void INT_Excep_CTXI1(void) __attribute__((used));

/* CFRXI2*/
void INT_Excep_CFRXI2(void) __attribute__((used));

/* CERI2*/
void INT_Excep_CERI2(void) __attribute__((used));

/* CTXI2*/
void INT_Excep_CTXI2(void) __attribute__((used));

/* CFRXI3*/
void INT_Excep_CFRXI3(void) __attribute__((used));

/* CERI3*/
void INT_Excep_CERI3(void) __attribute__((used));

/* CTXI3*/
void INT_Excep_CTXI3(void) __attribute__((used));

/* CFRXI4*/
void INT_Excep_CFRXI4(void) __attribute__((used));

/* CERI4*/
void INT_Excep_CERI4(void) __attribute__((used));

/* CTXI4*/
void INT_Excep_CTXI4(void) __attribute__((used));

/* SPEI0*/
void INT_Excep_SPEI0(void) __attribute__((used));

/* SPRI0*/
void INT_Excep_SPRI0(void) __attribute__((used));

/* SPTI0*/
void INT_Excep_SPTI0(void) __attribute__((used));

/* SPEI1*/
void INT_Excep_SPEI1(void) __attribute__((used));

/* SPRI1*/
void INT_Excep_SPRI1(void) __attribute__((used));

/* SPTI1*/
void INT_Excep_SPTI1(void) __attribute__((used));

/* SPEI2*/
void INT_Excep_SPEI2(void) __attribute__((used));

/* SPRI2*/
void INT_Excep_SPRI2(void) __attribute__((used));

/* SPTI2*/
void INT_Excep_SPTI2(void) __attribute__((used));

/* SPEI3*/
void INT_Excep_SPEI3(void) __attribute__((used));

/* SPRI3*/
void INT_Excep_SPRI3(void) __attribute__((used));

/* SPTI3*/
void INT_Excep_SPTI3(void) __attribute__((used));

/* SPEI4*/
void INT_Excep_SPEI4(void) __attribute__((used));

/* SPRI4*/
void INT_Excep_SPRI4(void) __attribute__((used));

/* SPTI4*/
void INT_Excep_SPTI4(void) __attribute__((used));

/* IEBBTD*/
void INT_Excep_IEBBTD(void) __attribute__((used));

/* IEBBTERR*/
void INT_Excep_IEBBTERR(void) __attribute__((used));

/* IEBBTSTA*/
void INT_Excep_IEBBTSTA(void) __attribute__((used));

/* IEBBTV*/
void INT_Excep_IEBBTV(void) __attribute__((used));

/* ISY*/
void INT_Excep_ISY(void) __attribute__((used));

/* IERR*/
void INT_Excep_IERR(void) __attribute__((used));

/* ITARG*/
void INT_Excep_ITARG(void) __attribute__((used));

/* ISEC*/
void INT_Excep_ISEC(void) __attribute__((used));

/* IBUF*/
void INT_Excep_IBUF(void) __attribute__((used));

/* IREADY*/
void INT_Excep_IREADY(void) __attribute__((used));

/* STERB*/
void INT_Excep_STERB(void) __attribute__((used));

/* BTOERB*/
/*void INT_Excep_BTOERB(void) __attribute__((used));*/

/* FLTENDI*/
void INT_Excep_FLTENDI(void) __attribute__((used));

/* FLTREQ0I*/
void INT_Excep_FLTREQ0I(void) __attribute__((used));

/* FLTREQ1I*/
void INT_Excep_FLTREQ1I(void) __attribute__((used));

/* MMC0*/
void INT_Excep_MMC0(void) __attribute__((used));

/* MMC1*/
void INT_Excep_MMC1(void) __attribute__((used));

/* MMC2*/
void INT_Excep_MMC2(void) __attribute__((used));

/* SDHI0_3*/
void INT_Excep_SDHI0_3(void) __attribute__((used));

/* SDHI0_0*/
void INT_Excep_SDHI0_0(void) __attribute__((used));

/* SDHI0_1*/
void INT_Excep_SDHI0_1(void) __attribute__((used));

/* SDHI1_3*/
void INT_Excep_SDHI1_3(void) __attribute__((used));

/* SDHI1_0*/
void INT_Excep_SDHI1_0(void) __attribute__((used));

/* SDHI1_1*/
void INT_Excep_SDHI1_1(void) __attribute__((used));

/* ARM*/
void INT_Excep_ARM(void) __attribute__((used));

/* PRD*/
void INT_Excep_PRD(void) __attribute__((used));

/* CUP*/
void INT_Excep_CUP(void) __attribute__((used));

/* SCUAI0*/
void INT_Excep_SCUAI0(void) __attribute__((used));

/* SCUAI1*/
void INT_Excep_SCUAI1(void) __attribute__((used));

/* SCUFDI0*/
void INT_Excep_SCUFDI0(void) __attribute__((used));

/* SCUFDI1*/
void INT_Excep_SCUFDI1(void) __attribute__((used));

/* SCUFDI2*/
void INT_Excep_SCUFDI2(void) __attribute__((used));

/* SCUFDI3*/
void INT_Excep_SCUFDI3(void) __attribute__((used));

/* SCUFUI0*/
void INT_Excep_SCUFUI0(void) __attribute__((used));

/* SCUFUI1*/
void INT_Excep_SCUFUI1(void) __attribute__((used));

/* SCUFUI2*/
void INT_Excep_SCUFUI2(void) __attribute__((used));

/* SCUFUI3*/
void INT_Excep_SCUFUI3(void) __attribute__((used));

/* SCUDVI0*/
void INT_Excep_SCUDVI0(void) __attribute__((used));

/* SCUDVI1*/
void INT_Excep_SCUDVI1(void) __attribute__((used));

/* SCUDVI2*/
void INT_Excep_SCUDVI2(void) __attribute__((used));

/* SCUDVI3*/
void INT_Excep_SCUDVI3(void) __attribute__((used));

/* MLBCI*/
void INT_Excep_MLBCI(void) __attribute__((used));

/* MLBSI*/
void INT_Excep_MLBSI(void) __attribute__((used));

/* DRC0*/
void INT_Excep_DRC0(void) __attribute__((used));

/* DRC1*/
void INT_Excep_DRC1(void) __attribute__((used));

/* vector 329 reserved */
/* vector 330 reserved */

/* LINI0_INT_T*/
void INT_Excep_LINI0_INT_T(void) __attribute__((used));

/* LINI0_INT_R*/
void INT_Excep_LINI0_INT_R(void) __attribute__((used));

/* LINI0_INT_S*/
void INT_Excep_LINI0_INT_S(void) __attribute__((used));

/* LINI0_INT_M*/
void INT_Excep_LINI0_INT_M(void) __attribute__((used));

/* LINI1_INT_T*/
void INT_Excep_LINI1_INT_T(void) __attribute__((used));

/* LINI1_INT_R*/
void INT_Excep_LINI1_INT_R(void) __attribute__((used));

/* LINI1_INT_S*/
void INT_Excep_LINI1_INT_S(void) __attribute__((used));

/* LINI1_INT_M*/
void INT_Excep_LINI1_INT_M(void) __attribute__((used));

/* vector 339 reserved */
/* vector 340 reserved */
/* vector 341 reserved */
/* vector 342 reserved */
/* vector 343 reserved */
/* vector 344 reserved */
/* vector 345 reserved */
/* vector 346 reserved */

/* ERI0*/
void INT_Excep_ERI0(void) __attribute__((used));

/* RXI0*/
void INT_Excep_RXI0(void) __attribute__((used));

/* TXI0*/
void INT_Excep_TXI0(void) __attribute__((used));

/* TEI0*/
void INT_Excep_TEI0(void) __attribute__((used));

/* ERI1*/
void INT_Excep_ERI1(void) __attribute__((used));

/* RXI1*/
void INT_Excep_RXI1(void) __attribute__((used));

/* TXI1*/
void INT_Excep_TXI1(void) __attribute__((used));

/* TEI1*/
void INT_Excep_TEI1(void) __attribute__((used));

/* vector 355 reserved */
/* vector 356 reserved */
/* vector 357 reserved */
/* vector 358 reserved */

/* ETHERI*/
void INT_Excep_ETHERI(void) __attribute__((used));

/* vector 360 reserved */
/* vector 361 reserved */
/* vector 362 reserved */
/* vector 363 reserved */

/* CEUI*/
void INT_Excep_CEUI(void) __attribute__((used));

/* INT_CSIH0TIR*/
void INT_Excep_INT_CSIH0TIR(void) __attribute__((used));

/* INT_CSIH0TIRE*/
void INT_Excep_INT_CSIH0TIRE(void) __attribute__((used));

/* INT_CSIH1TIC*/
void INT_Excep_INT_CSIH1TIC(void) __attribute__((used));

/* INT_CSIH1TIJC*/
void INT_Excep_INT_CSIH1TIJC(void) __attribute__((used));

/* ECCE10*/
void INT_Excep_ECCE10(void) __attribute__((used));

/* ECCE20*/
void INT_Excep_ECCE20(void) __attribute__((used));

/* ECCOVF0*/
void INT_Excep_ECCOVF0(void) __attribute__((used));

/* ECCE11*/
void INT_Excep_ECCE11(void) __attribute__((used));

/* ECCE21*/
void INT_Excep_ECCE21(void) __attribute__((used));

/* ECCOVF1*/
void INT_Excep_ECCOVF1(void) __attribute__((used));

/* ECCE12*/
void INT_Excep_ECCE12(void) __attribute__((used));

/* ECCE22*/
void INT_Excep_ECCE22(void) __attribute__((used));

/* ECCOVF2*/
void INT_Excep_ECCOVF2(void) __attribute__((used));

/* ECCE13*/
void INT_Excep_ECCE13(void) __attribute__((used));

/* ECCE23*/
void INT_Excep_ECCE23(void) __attribute__((used));

/* ECCOVF3*/
void INT_Excep_ECCOVF3(void) __attribute__((used));

/* H2XMLB_ERRINT*/
void INT_Excep_H2XMLB_ERRINT(void) __attribute__((used));

/* H2XIC1_ERRINT*/
void INT_Excep_H2XIC1_ERRINT(void) __attribute__((used));

/* X2HPERI1_ERRINT*/
void INT_Excep_X2HPERI1_ERRINT(void) __attribute__((used));

/* X2HPERI2_ERRINT*/
void INT_Excep_X2HPERI2_ERRINT(void) __attribute__((used));

/* X2HPERI34_ERRINT*/
void INT_Excep_X2HPERI34_ERRINT(void) __attribute__((used));

/* X2HPERI5_ERRINT*/
void INT_Excep_X2HPERI5_ERRINT(void) __attribute__((used));

/* X2HPERI67_ERRINT*/
void INT_Excep_X2HPERI67_ERRINT(void) __attribute__((used));

/* X2HDBGR_ERRINT*/
void INT_Excep_X2HDBGR_ERRINT(void) __attribute__((used));

/* X2HBSC_ERRINT */
void INT_Excep_X2HBSC_ERRINT(void) __attribute__((used));

/* X2HSPI1_ERRINT */
void INT_Excep_X2HSPI1_ERRINT(void) __attribute__((used));

/* X2HSPI2_ERRINT */
void INT_Excep_X2HSPI2_ERRINT(void) __attribute__((used));

/* PRRI*/
void INT_Excep_PRRI(void) __attribute__((used));

/* IFEI0*/
void INT_Excep_IFEI0(void) __attribute__((used));

/* OFFI0*/
void INT_Excep_OFFI0(void) __attribute__((used));

/* PFVEI0*/
void INT_Excep_PFVEI0(void) __attribute__((used));

/* IFEI1*/
void INT_Excep_IFEI1(void) __attribute__((used));

/* OFFI1*/
void INT_Excep_OFFI1(void) __attribute__((used));

/* PFVEI1*/
void INT_Excep_PFVEI1(void) __attribute__((used));

/* vector 396 reserved */
/* vector 397 reserved */
/* vector 398 reserved */
/* vector 399 reserved */
/* vector 400 reserved */
/* vector 401 reserved */
/* vector 402 reserved */
/* vector 403 reserved */
/* vector 404 reserved */
/* vector 405 reserved */
/* vector 406 reserved */
/* vector 407 reserved */
/* vector 408 reserved */
/* vector 409 reserved */
/* vector 410 reserved */
/* vector 411 reserved */
/* vector 412 reserved */
/* vector 413 reserved */
/* vector 414 reserved */
/* vector 415 reserved */

/* TINT0*/
void INT_Excep_TINT0(void) __attribute__((used));

/* TINT1*/
void INT_Excep_TINT1(void) __attribute__((used));

/* TINT2*/
void INT_Excep_TINT2(void) __attribute__((used));

/* TINT3*/
void INT_Excep_TINT3(void) __attribute__((used));

/* TINT4*/
void INT_Excep_TINT4(void) __attribute__((used));

/* TINT5*/
void INT_Excep_TINT5(void) __attribute__((used));

/* TINT6*/
void INT_Excep_TINT6(void) __attribute__((used));

/* TINT7*/
void INT_Excep_TINT7(void) __attribute__((used));

/* TINT8*/
void INT_Excep_TINT8(void) __attribute__((used));

/* TINT9*/
void INT_Excep_TINT9(void) __attribute__((used));

/* TINT10*/
void INT_Excep_TINT10(void) __attribute__((used));

/* TINT11*/
void INT_Excep_TINT11(void) __attribute__((used));

/* TINT12*/
void INT_Excep_TINT12(void) __attribute__((used));

/* TINT13*/
void INT_Excep_TINT13(void) __attribute__((used));

/* TINT14*/
void INT_Excep_TINT14(void) __attribute__((used));

/* TINT15*/
void INT_Excep_TINT15(void) __attribute__((used));

/* TINT16*/
void INT_Excep_TINT16(void) __attribute__((used));

/* TINT17*/
void INT_Excep_TINT17(void) __attribute__((used));

/* TINT18*/
void INT_Excep_TINT18(void) __attribute__((used));

/* TINT19*/
void INT_Excep_TINT19(void) __attribute__((used));

/* TINT20*/
void INT_Excep_TINT20(void) __attribute__((used));

/* TINT21*/
void INT_Excep_TINT21(void) __attribute__((used));

/* TINT22*/
void INT_Excep_TINT22(void) __attribute__((used));

/* TINT23*/
void INT_Excep_TINT23(void) __attribute__((used));

/* TINT24*/
void INT_Excep_TINT24(void) __attribute__((used));

/* TINT25*/
void INT_Excep_TINT25(void) __attribute__((used));

/* TINT26*/
void INT_Excep_TINT26(void) __attribute__((used));

/* TINT27*/
void INT_Excep_TINT27(void) __attribute__((used));

/* TINT28*/
void INT_Excep_TINT28(void) __attribute__((used));

/* TINT29*/
void INT_Excep_TINT29(void) __attribute__((used));

/* TINT30*/
void INT_Excep_TINT30(void) __attribute__((used));

/* TINT31*/
void INT_Excep_TINT31(void) __attribute__((used));

/* TINT32*/
void INT_Excep_TINT32(void) __attribute__((used));

/* TINT33*/
void INT_Excep_TINT33(void) __attribute__((used));

/* TINT34*/
void INT_Excep_TINT34(void) __attribute__((used));

/* TINT35*/
void INT_Excep_TINT35(void) __attribute__((used));

/* TINT36*/
void INT_Excep_TINT36(void) __attribute__((used));

/* TINT37*/
void INT_Excep_TINT37(void) __attribute__((used));

/* TINT38*/
void INT_Excep_TINT38(void) __attribute__((used));

/* TINT39*/
void INT_Excep_TINT39(void) __attribute__((used));

/* TINT40*/
void INT_Excep_TINT40(void) __attribute__((used));

/* TINT41*/
void INT_Excep_TINT41(void) __attribute__((used));

/* TINT42*/
void INT_Excep_TINT42(void) __attribute__((used));

/* TINT43*/
void INT_Excep_TINT43(void) __attribute__((used));

/* TINT44*/
void INT_Excep_TINT44(void) __attribute__((used));

/* TINT45*/
void INT_Excep_TINT45(void) __attribute__((used));

/* TINT46*/
void INT_Excep_TINT46(void) __attribute__((used));

/* TINT47*/
void INT_Excep_TINT47(void) __attribute__((used));

/* TINT48*/
void INT_Excep_TINT48(void) __attribute__((used));

/* TINT49*/
void INT_Excep_TINT49(void) __attribute__((used));

/* TINT50*/
void INT_Excep_TINT50(void) __attribute__((used));

/* TINT51*/
void INT_Excep_TINT51(void) __attribute__((used));

/* TINT52*/
void INT_Excep_TINT52(void) __attribute__((used));

/* TINT53*/
void INT_Excep_TINT53(void) __attribute__((used));

/* TINT54*/
void INT_Excep_TINT54(void) __attribute__((used));

/* TINT55*/
void INT_Excep_TINT55(void) __attribute__((used));

/* TINT56*/
void INT_Excep_TINT56(void) __attribute__((used));

/* TINT57*/
void INT_Excep_TINT57(void) __attribute__((used));

/* TINT58*/
void INT_Excep_TINT58(void) __attribute__((used));

/* TINT59*/
void INT_Excep_TINT59(void) __attribute__((used));

/* TINT60*/
void INT_Excep_TINT60(void) __attribute__((used));

/* TINT61*/
void INT_Excep_TINT61(void) __attribute__((used));

/* TINT62*/
void INT_Excep_TINT62(void) __attribute__((used));

/* TINT63*/
void INT_Excep_TINT63(void) __attribute__((used));

/* TINT64*/
void INT_Excep_TINT64(void) __attribute__((used));

/* TINT65*/
void INT_Excep_TINT65(void) __attribute__((used));

/* TINT66*/
void INT_Excep_TINT66(void) __attribute__((used));

/* TINT67*/
void INT_Excep_TINT67(void) __attribute__((used));

/* TINT68*/
void INT_Excep_TINT68(void) __attribute__((used));

/* TINT69*/
void INT_Excep_TINT69(void) __attribute__((used));

/* TINT70*/
void INT_Excep_TINT70(void) __attribute__((used));

/* TINT71*/
void INT_Excep_TINT71(void) __attribute__((used));

/* TINT72*/
void INT_Excep_TINT72(void) __attribute__((used));

/* TINT73*/
void INT_Excep_TINT73(void) __attribute__((used));

/* TINT74*/
void INT_Excep_TINT74(void) __attribute__((used));

/* TINT75*/
void INT_Excep_TINT75(void) __attribute__((used));

/* TINT76*/
void INT_Excep_TINT76(void) __attribute__((used));

/* TINT77*/
void INT_Excep_TINT77(void) __attribute__((used));

/* TINT78*/
void INT_Excep_TINT78(void) __attribute__((used));

/* TINT79*/
void INT_Excep_TINT79(void) __attribute__((used));

/* TINT80*/
void INT_Excep_TINT80(void) __attribute__((used));

/* TINT81*/
void INT_Excep_TINT81(void) __attribute__((used));

/* TINT82*/
void INT_Excep_TINT82(void) __attribute__((used));

/* TINT83*/
void INT_Excep_TINT83(void) __attribute__((used));

/* TINT84*/
void INT_Excep_TINT84(void) __attribute__((used));

/* TINT85*/
void INT_Excep_TINT85(void) __attribute__((used));

/* TINT86*/
void INT_Excep_TINT86(void) __attribute__((used));

/* TINT87*/
void INT_Excep_TINT87(void) __attribute__((used));

/* TINT88*/
void INT_Excep_TINT88(void) __attribute__((used));

/* TINT89*/
void INT_Excep_TINT89(void) __attribute__((used));

/* TINT90*/
void INT_Excep_TINT90(void) __attribute__((used));

/* TINT91*/
void INT_Excep_TINT91(void) __attribute__((used));

/* TINT92*/
void INT_Excep_TINT92(void) __attribute__((used));

/* TINT93*/
void INT_Excep_TINT93(void) __attribute__((used));

/* TINT94*/
void INT_Excep_TINT94(void) __attribute__((used));

/* TINT95*/
void INT_Excep_TINT95(void) __attribute__((used));

/* TINT96*/
void INT_Excep_TINT96(void) __attribute__((used));

/* TINT97*/
void INT_Excep_TINT97(void) __attribute__((used));

/* TINT98*/
void INT_Excep_TINT98(void) __attribute__((used));

/* TINT99*/
void INT_Excep_TINT99(void) __attribute__((used));

/* TINT100*/
void INT_Excep_TINT100(void) __attribute__((used));

/* TINT101*/
void INT_Excep_TINT101(void) __attribute__((used));

/* TINT102*/
void INT_Excep_TINT102(void) __attribute__((used));

/* TINT103*/
void INT_Excep_TINT103(void) __attribute__((used));

/* TINT104*/
void INT_Excep_TINT104(void) __attribute__((used));

/* TINT105*/
void INT_Excep_TINT105(void) __attribute__((used));

/* TINT106*/
void INT_Excep_TINT106(void) __attribute__((used));

/* TINT107*/
void INT_Excep_TINT107(void) __attribute__((used));

/* TINT108*/
void INT_Excep_TINT108(void) __attribute__((used));

/* TINT109*/
void INT_Excep_TINT109(void) __attribute__((used));

/* TINT110*/
void INT_Excep_TINT110(void) __attribute__((used));

/* TINT111*/
void INT_Excep_TINT111(void) __attribute__((used));

/* TINT112*/
void INT_Excep_TINT112(void) __attribute__((used));

/* TINT113*/
void INT_Excep_TINT113(void) __attribute__((used));

/* TINT114*/
void INT_Excep_TINT114(void) __attribute__((used));

/* TINT115*/
void INT_Excep_TINT115(void) __attribute__((used));

/* TINT116*/
void INT_Excep_TINT116(void) __attribute__((used));

/* TINT117*/
void INT_Excep_TINT117(void) __attribute__((used));

/* TINT118*/
void INT_Excep_TINT118(void) __attribute__((used));

/* TINT119*/
void INT_Excep_TINT119(void) __attribute__((used));

/* TINT120*/
void INT_Excep_TINT120(void) __attribute__((used));

/* TINT121*/
void INT_Excep_TINT121(void) __attribute__((used));

/* TINT122*/
void INT_Excep_TINT122(void) __attribute__((used));

/* TINT123*/
void INT_Excep_TINT123(void) __attribute__((used));

/* TINT124*/
void INT_Excep_TINT124(void) __attribute__((used));

/* TINT125*/
void INT_Excep_TINT125(void) __attribute__((used));

/* TINT126*/
void INT_Excep_TINT126(void) __attribute__((used));

/* TINT127*/
void INT_Excep_TINT127(void) __attribute__((used));

/* TINT128*/
void INT_Excep_TINT128(void) __attribute__((used));

/* TINT129*/
void INT_Excep_TINT129(void) __attribute__((used));

/* TINT130*/
void INT_Excep_TINT130(void) __attribute__((used));

/* TINT131*/
void INT_Excep_TINT131(void) __attribute__((used));

/* TINT132*/
void INT_Excep_TINT132(void) __attribute__((used));

/* TINT133*/
void INT_Excep_TINT133(void) __attribute__((used));

/* TINT134*/
void INT_Excep_TINT134(void) __attribute__((used));

/* TINT135*/
void INT_Excep_TINT135(void) __attribute__((used));

/* TINT136*/
void INT_Excep_TINT136(void) __attribute__((used));

/* TINT137*/
void INT_Excep_TINT137(void) __attribute__((used));

/* TINT138*/
void INT_Excep_TINT138(void) __attribute__((used));

/* TINT139*/
void INT_Excep_TINT139(void) __attribute__((used));

/* TINT140*/
void INT_Excep_TINT140(void) __attribute__((used));

/* TINT141*/
void INT_Excep_TINT141(void) __attribute__((used));

/* TINT142*/
void INT_Excep_TINT142(void) __attribute__((used));

/* TINT143*/
void INT_Excep_TINT143(void) __attribute__((used));

/* TINT144*/
void INT_Excep_TINT144(void) __attribute__((used));

/* TINT145*/
void INT_Excep_TINT145(void) __attribute__((used));

/* TINT146*/
void INT_Excep_TINT146(void) __attribute__((used));

/* TINT147*/
void INT_Excep_TINT147(void) __attribute__((used));

/* TINT148*/
void INT_Excep_TINT148(void) __attribute__((used));

/* TINT149*/
void INT_Excep_TINT149(void) __attribute__((used));

/* TINT150*/
void INT_Excep_TINT150(void) __attribute__((used));

/* TINT151*/
void INT_Excep_TINT151(void) __attribute__((used));

/* TINT152*/
void INT_Excep_TINT152(void) __attribute__((used));

/* TINT153*/
void INT_Excep_TINT153(void) __attribute__((used));

/* TINT154*/
void INT_Excep_TINT154(void) __attribute__((used));

/* TINT155*/
void INT_Excep_TINT155(void) __attribute__((used));

/* TINT156*/
void INT_Excep_TINT156(void) __attribute__((used));

/* TINT157*/
void INT_Excep_TINT157(void) __attribute__((used));

/* TINT158*/
void INT_Excep_TINT158(void) __attribute__((used));

/* TINT159*/
void INT_Excep_TINT159(void) __attribute__((used));

/* TINT160*/
void INT_Excep_TINT160(void) __attribute__((used));

/* TINT161*/
void INT_Excep_TINT161(void) __attribute__((used));

/* TINT162*/
void INT_Excep_TINT162(void) __attribute__((used));

/* TINT163*/
void INT_Excep_TINT163(void) __attribute__((used));

/* TINT164*/
void INT_Excep_TINT164(void) __attribute__((used));

/* TINT165*/
void INT_Excep_TINT165(void) __attribute__((used));

/* TINT166*/
void INT_Excep_TINT166(void) __attribute__((used));

/* TINT167*/
void INT_Excep_TINT167(void) __attribute__((used));

/* TINT168*/
void INT_Excep_TINT168(void) __attribute__((used));

/* TINT169*/
void INT_Excep_TINT169(void) __attribute__((used));

/* TINT170*/
void INT_Excep_TINT170(void) __attribute__((used));

/*;<<VECTOR DATA START (POWER ON RESET)>>*/
/*;Power On Reset PC*/
extern void _PowerON_Reset(void) __attribute__((used));                                                                                                                
/*;<<VECTOR DATA END (_POWER ON RESET)>>*/

#endif