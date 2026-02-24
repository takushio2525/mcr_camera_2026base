/************************************************************************/
/*    File Version: V1.01                                               */
/*    Date Generated: 23/04/2024                                        */
/************************************************************************/


@==================================================================
@ Entry point for the Reset handler
@==================================================================
/* This program is allocated to section ".fvectors" */
    .section .fvectors, "ax"
    .arm

    .extern  _PowerON_Reset
	.extern  INT_Excep_UndefinedInst
	.extern  INT_Excep_SWI
	.extern  INT_Excep_PREFETCH_ABORT
	.extern  INT_Excep_DATA_ABORT
	.extern  INT_Excep_IRQ
	.extern  INT_Excep_FIQ
	.extern  INT_Excep_Reserved

    .global Start
    .global  vector_table

Start:

vector_table:
    LDR pc, =_PowerON_Reset            @ +0x0000_0000 : Reset exception
    LDR pc, =INT_Excep_UndefinedInst   @ +0x0000_0004 : Exception(Undefined Instruction)
    LDR pc, =INT_Excep_SWI             @ +0x0000_0008 : Exception(SWI)
    LDR pc, =INT_Excep_PREFETCH_ABORT  @ +0x0000_000c : Exception(Prefetch abort)
    LDR pc, =INT_Excep_DATA_ABORT      @ +0x0000_0010 : Exception(Data abort)
    LDR pc, =INT_Excep_Reserved        @ +0x0000_0014 : Reserved
    LDR pc, =INT_Excep_IRQ             @ +0x0000_0018 : Exception(IRQ)
    LDR pc, =INT_Excep_FIQ             @ +0x0000_001c : Exception(FIQ)

Literals:
    .LTORG

    .END
