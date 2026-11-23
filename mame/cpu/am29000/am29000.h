// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/***************************************************************************

    am29000.h
    Interface file for the portable AMD Am29000 emulator.
    Written by Phil Bennett

***************************************************************************/

#ifndef __AM29000_H__
#define __AM29000_H__

#include "cpuintrf.h"


/***************************************************************************
    REGISTER ENUMERATION
***************************************************************************/

enum
{
	AM29000_PC = 1,
	AM29000_VAB,
	AM29000_OPS,
	AM29000_CPS,
	AM29000_CFG,
	AM29000_CHA,
	AM29000_CHD,
	AM29000_CHC,
	AM29000_RBP,
	AM29000_TMC,
	AM29000_TMR,
	AM29000_PC0,
	AM29000_PC1,
	AM29000_PC2,
	AM29000_MMU,
	AM29000_LRU,
	AM29000_IPC,
	AM29000_IPA,
	AM29000_IPB,
	AM29000_Q,
	AM29000_ALU,
	AM29000_BP,
	AM29000_FC,
	AM29000_CR,
	AM29000_FPE,
	AM29000_INTE,
	AM29000_FPS,
	AM29000_R1,
	AM29000_R64,
	AM29000_R255 = AM29000_R64 + 191
};


/***************************************************************************
    SPECIAL PURPOSE REGISTER INDICES
***************************************************************************/

enum
{   SPR_VAB  = 0,
	SPR_OPS  = 1,
	SPR_CPS  = 2,
	SPR_CFG  = 3,
	SPR_CHA  = 4,
	SPR_CHD  = 5,
	SPR_CHC  = 6,
	SPR_RBP  = 7,
	SPR_TMC  = 8,
	SPR_TMR  = 9,
	SPR_PC0  = 10,
	SPR_PC1  = 11,
	SPR_PC2  = 12,
	SPR_MMU  = 13,
	SPR_LRU  = 14,
	SPR_IPC  = 128,
	SPR_IPA  = 129,
	SPR_IPB  = 130,
	SPR_Q    = 131,
	SPR_ALU  = 132,
	SPR_BP   = 133,
	SPR_FC   = 134,
	SPR_CR   = 135,
	SPR_FPE  = 160,
	SPR_INTE = 161,
	SPR_FPS  = 162
};


/***************************************************************************
    EXCEPTION VECTORS
***************************************************************************/

enum
{
	EXCEPTION_ILLEGAL_OPCODE            = 0,
	EXCEPTION_UNALIGNED_ACCESS          = 1,
	EXCEPTION_OUT_OF_RANGE              = 2,
	EXCEPTION_COPRO_NOT_PRESENT         = 3,
	EXCEPTION_COPRO_EXCEPTION           = 4,
	EXCEPTION_PROTECTION_VIOLATION      = 5,
	EXCEPTION_INST_ACCESS_VIOLATION     = 6,
	EXCEPTION_DATA_ACCESS_VIOLATION     = 7,
	EXCEPTION_USER_INST_TLB_MISS        = 8,
	EXCEPTION_USER_DATA_TLB_MISS        = 9,
	EXCEPTION_SUPER_INST_TLB_MISS       = 10,
	EXCEPTION_SUPER_DATA_TLB_MISS       = 11,
	EXCEPTION_INST_TLB_PROT_VIOLATION   = 12,
	EXCEPTION_DATA_TLB_PROT_VIOLATION   = 13,
	EXCEPTION_TIMER                     = 14,
	EXCEPTION_TRACE                     = 15,
	EXCEPTION_INTR0                     = 16,
	EXCEPTION_INTR1                     = 17,
	EXCEPTION_INTR2                     = 18,
	EXCEPTION_INTR3                     = 19,
	EXCEPTION_TRAP0                     = 20,
	EXCEPTION_TRAP1                     = 21,
	EXCEPTION_FLOATING_POINT_EXCEPTION  = 22,
	EXCEPTION_MULTM                     = 30,
	EXCEPTION_MULTMU                    = 31,
	EXCEPTION_MULTIPLY                  = 32,
	EXCEPTION_DIVIDE                    = 33,
	EXCEPTION_MULTIPLU                  = 44       // TODO: FINISH ME
};


/***************************************************************************
    SPECIAL PURPOSE REGISTER FIELDS
***************************************************************************/

#define PRL_AM29000             (0 << 3)
#define PRL_AM29050             (1 << 3)
#define PRL_REV_A               (0)
#define PRL_REV_B               (1)
#define PRL_REV_C               (2)
#define PRL_REV_D               (3)

#define PROCESSOR_REL_FIELD     (PRL_AM29000 | PRL_REV_D)

#define VAB_SHIFT               (16)
#define VAB_MASK                (0xffff)

#define CPS_CA                  (1 << 15)
#define CPS_IP                  (1 << 14)
#define CPS_TE                  (1 << 13)
#define CPS_TP                  (1 << 12)
#define CPS_TU                  (1 << 11)
#define CPS_FZ                  (1 << 10)
#define CPS_LK                  (1 << 9)
#define CPS_RE                  (1 << 8)
#define CPS_WM                  (1 << 7)
#define CPS_PD                  (1 << 6)
#define CPS_PI                  (1 << 5)
#define CPS_SM                  (1 << 4)
#define CPS_IM_SHIFT            (2)
#define CPS_IM_MASK             (3)
#define CPS_DI                  (1 << 1)
#define CPS_DA                  (1 << 0)

#define CFG_PRL_MASK            (0xff)
#define CFG_PRL_SHIFT           (24)
#define CFG_DW                  (1 << 5)
#define CFG_VF                  (1 << 4)
#define CFG_RV                  (1 << 3)
#define CFG_BO                  (1 << 2)
#define CFG_CP                  (1 << 1)
#define CFG_CD                  (1 << 0)

#define CHC_CE_CNTL_MASK        (0xff)
#define CHC_CE_CNTL_SHIFT       (24)
#define CHC_CR_MASK             (0xff)
#define CHC_CR_SHIFT            (16)
#define CHC_LS                  (1 << 15)
#define CHC_ML                  (1 << 14)
#define CHC_ST                  (1 << 13)
#define CHC_LA                  (1 << 12)
#define CHC_TF                  (1 << 11)
#define CHC_TR_MASK             (0xff)
#define CHC_TR_SHIFT            (2)
#define CHC_NN                  (1 << 1)
#define CHC_CV                  (1 << 0)

#define RBP_MASK                (0xffff)

#define TCV_MASK                (0x00ffffff)
#define TCV_SHIFT               (0)

#define TMR_OV                  (1 << 26)
#define TMR_IN                  (1 << 25)
#define TMR_IE                  (1 << 24)
#define TMR_TRV_MASK            (0x00ffffff)
#define TMR_TRV_SHIFT           (0)

#define PC_MASK                 (0xfffffffc)

#define MMU_PS_MASK             (3)
#define MMU_PS_SHIFT            (8)
#define MMU_PID_MASK            (0xff)
#define MMU_PID_SHIFT           (0)

#define LRU_MASK                (0x3f)
#define LRU_SHIFT               (1)

#define ALU_DF_SHIFT            (11)
#define ALU_DF                  (1 << 11)
#define ALU_V_SHIFT             (10)
#define ALU_V                   (1 << 10)
#define ALU_N_SHIFT             (9)
#define ALU_N                   (1 << 9)
#define ALU_Z_SHIFT             (8)
#define ALU_Z                   (1 << 8)
#define ALU_C_SHIFT             (7)
#define ALU_C                   (1 << 7)
#define ALU_BP_MASK             (3)
#define ALU_BP_SHIFT            (5)
#define ALU_FC_MASK             (0x1f)
#define ALU_FC_SHIFT            (0)

#define IPX_MASK                (0xff)
#define IPX_SHIFT               (2)




/***************************************************************************
    INTERRUPT CONSTANTS
***************************************************************************/

#define AM29000_INTR0       0
#define AM29000_INTR1       1
#define AM29000_INTR2       2
#define AM29000_INTR3       3



void am29000_get_info(UINT32 state, union cpuinfo *info);

#endif /* __AM29000_H__ */
