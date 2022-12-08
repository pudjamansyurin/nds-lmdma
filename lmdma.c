/*
 * lmdma.c
 *
 *  Created on: Dec 8, 2022
 *      Author: pujak
 */
#include "lmdma.h"
#include "ae210p.h"
#include <stddef.h>

/*****************************************************************************
 *  Custom stdlib functions                                                  *
 ****************************************************************************/
//#define printf(arg...)			do { char str[0x100]; sprintf(str, ##arg); uart_puts(str); } while (0)
//#define MEMSET(s, c, n)			__builtin_memset ((s), (c), (n))
//#define MEMCPY(des, src, n)		__builtin_memcpy((des), (src), (n))

/*****************************************************************************
 *  HAL Level : Interrupt                                                    *
 ****************************************************************************/
#define NDS32_HWINT_ID(hw)     		NDS32_INT_H##hw
#define NDS32_HWINT(hw)        		NDS32_HWINT_ID(hw)

// Number of LMDMA channels
#define LMDMA_NUMBER_OF_CHANNELS	((unsigned char) 2)
#define IS_LMDMA_V2()				(((__nds32__mfsr(NDS32_SR_DMA_CFG) >> 24) & 0xff) >= 0x2)
#define HAS_LMDMA()					(__nds32__mfsr(NDS32_SR_MSC_CFG) & MSC_CFG_mskLMDMA)

/* NDS32_DMA_ACT system register */
#define DMA_ACT_ACMD_NOP			0x0
#define DMA_ACT_ACMD_START			0x1
#define DMA_ACT_ACMD_STOP			0x2
#define DMA_ACT_ACMD_RESET			0x3

/* Private types */
typedef struct {
	unsigned int		Cnt;
} LMDMA_Channel_Info;

/* Private variable declarations */
static lmdma_callback_t g_lmdma_cb;
static LMDMA_Channel_Info g_channel_info[LMDMA_NUMBER_OF_CHANNELS];

/* Public function definitions */
int lmdma_initialize(lmdma_callback_t cb)
{
	if (!HAS_LMDMA()) {
		// printf("The system has no Local Memory DMA \n");
		return -1;
	}

	/* set LDMA priority to highest */
	__nds32__set_int_priority(NDS32_HWINT(IRQ_LDMA_VECTOR), 0);

	/* enable HW# (LDMA) */
	__nds32__enable_int(NDS32_HWINT(IRQ_LDMA_VECTOR));

	/* Enable global interrupt */
	__nds32__setgie_en();

	g_lmdma_cb = cb;

	return 0;
}

int lmdma_uninitialize(void)
{
	/* disable HW# (LDMA) */
	__nds32__disable_int(NDS32_HWINT(IRQ_LDMA_VECTOR));

	/* set LDMA priority to lowest */
	__nds32__set_int_priority(NDS32_HWINT(IRQ_LDMA_VECTOR), 3);

	return 0;
}

int lmdma_abort(void)
{
	/* Disable LMDMA engine */
	__nds32__mtsr((__nds32__mfsr(NDS32_SR_DMA_GCSW) & (~(DMA_GCSW_DMA_ENGINE_ON))), NDS32_SR_DMA_GCSW);

	return 0;
}

int lmdma_get_ch_count(void)
{
	int ch_count;

	ch_count = __nds32__mfsr(NDS32_SR_DMA_CFG) & DMA_CFG_mskNCHN;

	switch (ch_count) {
	case 0:
		// printf("There is 1 DMA channel \n");
		break;
	case 1:
		// printf("There are 2 DMA channel \n");
		break;
	default:
		// printf("Reserve \n");
		return -1;
	}

	return (ch_count+1);
}

int lmdma_ch_get_count(unsigned char ch)
{
	unsigned int mask;
	int xfer_cnt;

	/* Check if channel is valid */
	if (ch >= LMDMA_NUMBER_OF_CHANNELS) {
		return -1;
	}

	if (ch != ((__nds32__mfsr(NDS32_SR_DMA_GCSW) & DMA_GCSW_mskHCHAN) >> DMA_GCSW_offHCHAN)) {
		return -1;
	}

	mask = IS_LMDMA_V2() ? 0x7FFFFFU : 0x3FFFFU;
	xfer_cnt = __nds32__mfsr(NDS32_SR_DMA_TCNT) & mask;

	return (g_channel_info[ch].Cnt - xfer_cnt);
}

int lmdma_ch_configure(unsigned int ch,
				unsigned int src_addr,
				unsigned int dst_addr,
				unsigned int size,
				unsigned int gcsw_ctl,
				unsigned int setup_ctl,
				unsigned int setup_2d_ctl,
				unsigned int startup_2d_ctl) 
{
	unsigned int dma_gcsw;

	/* Save channel information */
	g_channel_info[ch].Cnt = size;

	/* Select Channel */
	__nds32__mtsr(ch, NDS32_SR_DMA_CHNSEL);

	/* Setup DMA Global Control and Status Word Register */
	dma_gcsw = __nds32__mfsr(NDS32_SR_DMA_GCSW);
	/* bit18 ~ 19, FSM : action command mode or fast-start mode with DMA_TCNT/DMA_ISADDR/DMA_ESADDR as trigger register */
	/* bit20 ~ 21, SCMD : NOCOND or EnQ or WAITDBS or ENQWAIT */
	/* bit 31, EN : DMA engine is enabled */
	dma_gcsw &= ~(DMA_GCSW_mskFSM | DMA_GCSW_mskSCMD | DMA_GCSW_mskEN);
	dma_gcsw |= (gcsw_ctl & (DMA_GCSW_mskFSM | DMA_GCSW_mskSCMD | DMA_GCSW_mskEN));
	__nds32__mtsr(dma_gcsw, NDS32_SR_DMA_GCSW);

	/* Setup DMA Setup Register */
	__nds32__mtsr(setup_ctl, NDS32_SR_DMA_SETUP);

	if (setup_ctl & DMA_SETUP_msk2DE) {
		/* Setup 2D DMA Setup Register */
		__nds32__mtsr(setup_2d_ctl, NDS32_SR_DMA_2DSET);

		/* Setup DMA 2D Startup Control Register */
		__nds32__mtsr(startup_2d_ctl, NDS32_SR_DMA_2DSCTL);
	}

	/* setup $dma_rcnt reg instead of directing to fill $dma_tcnt reg */
	/* The DMA Refill Element Count Register refills the DMA Transfer Count Register (DMA_TCNT)
	   with its content when all of the following conditions are true.
	   1. The channel is activated by a "Start" command.
	   2. The DMA_TCNT register is equal to zero. */
	__nds32__mtsr_dsb(size, NDS32_SR_DMA_RCNT);

	switch (gcsw_ctl & DMA_GCSW_mskFSM) {
	case DMA_GCSW_FSM_ACMD:
		if ((setup_ctl & DMA_SETUP_mskTDIR) == DMA_SETUP_LM_DDR) {	/* LM => DDR */
			/* internal start address offset (LM) */
			__nds32__mtsr(src_addr, NDS32_SR_DMA_ISADDR);

			/* External start address DDR */
			__nds32__mtsr_dsb(dst_addr, NDS32_SR_DMA_ESADDR);
		} else {	/* DDR => LM */
			/* internal start address offset (LM) */
			__nds32__mtsr(dst_addr, NDS32_SR_DMA_ISADDR);

			/* External start address DDR */
			__nds32__mtsr_dsb(src_addr, NDS32_SR_DMA_ESADDR);
		}

		/* Start the DMA transfer */
		__nds32__mtsr_dsb(DMA_ACT_ACMD_START, NDS32_SR_DMA_ACT);
		break;

	case DMA_GCSW_FSM_FSTART_TCNT:
		if ((setup_ctl & DMA_SETUP_mskTDIR) == DMA_SETUP_LM_DDR) {	/* LM => DDR */
			/* internal start address offset (LM) */
			__nds32__mtsr(src_addr, NDS32_SR_DMA_ISADDR);

			/* External start address DDR */
			__nds32__mtsr_dsb(dst_addr, NDS32_SR_DMA_ESADDR);
		} else {	/* DDR => LM */
			/* internal start address offset (LM) */
			__nds32__mtsr(dst_addr, NDS32_SR_DMA_ISADDR);

			/* External start address DDR */
			__nds32__mtsr_dsb(src_addr, NDS32_SR_DMA_ESADDR);
		}

		/* Start the DMA transfer */
		/* TCNT is trigger register as fast-start mode */
		/* Writing any value to the trigger register is equivalent to performing the following operations:
		   1. Update the value of the trigger register with the new value.
		   2. Write a "Start" command into the DMA_ACT register. The DMA_GCSW.SCMD field
		      will be used as the sub-action command for the "Start" command. */
		__nds32__mtsr_dsb(size, NDS32_SR_DMA_TCNT);
		break;

	case DMA_GCSW_FSM_FSTART_ISADDR:
		if ((setup_ctl & DMA_SETUP_mskTDIR) == DMA_SETUP_LM_DDR) {	/* LM => DDR */
			/* External start address DDR */
			__nds32__mtsr_dsb(dst_addr, NDS32_SR_DMA_ESADDR);

			/* Start the DMA transfer */
			/* Internal start address offset (LM)is trigger register as fast-start mode */
			/* Writing any value to the trigger register is equivalent to performing the following operations:
			   1. Update the value of the trigger register with the new value.
			   2. Write a "Start" command into the DMA_ACT register. The DMA_GCSW.SCMD field
			      will be used as the sub-action command for the "Start" command. */
			__nds32__mtsr(src_addr, NDS32_SR_DMA_ISADDR);
		} else {	/* DDR => LM */
			/* External start address DDR */
			__nds32__mtsr_dsb(src_addr, NDS32_SR_DMA_ESADDR);

			/* Start the DMA transfer */
			/* Internal start address offset (LM)is trigger register as fast-start mode */
			/* Writing any value to the trigger register is equivalent to performing the following operations:
			   1. Update the value of the trigger register with the new value.
			   2. Write a "Start" command into the DMA_ACT register. The DMA_GCSW.SCMD field
			      will be used as the sub-action command for the "Start" command. */
			__nds32__mtsr(dst_addr, NDS32_SR_DMA_ISADDR);
		}
		break;

	case DMA_GCSW_FSM_FSTART_ESADDR:
		if ((setup_ctl & DMA_SETUP_mskTDIR) == DMA_SETUP_LM_DDR) {	/* LM => DDR */
			/* Internal start address offset (LM) */
			__nds32__mtsr(src_addr, NDS32_SR_DMA_ISADDR);

			/* Start the DMA transfer */
			/* External start address DDR is trigger register as fast-start mode */
			/* Writing any value to the trigger register is equivalent to performing the following operations:
			   1. Update the value of the trigger register with the new value.
			   2. Write a "Start" command into the DMA_ACT register. The DMA_GCSW.SCMD field
			      will be used as the sub-action command for the "Start" command. */
			__nds32__mtsr_dsb(dst_addr, NDS32_SR_DMA_ESADDR);
		} else {	/* DDR => LM */
			/* Internal start address offset (LM) */
			__nds32__mtsr(dst_addr, NDS32_SR_DMA_ISADDR);

			/* Start the DMA transfer */
			/* External start address DDR is trigger register as fast-start mode */
			/* Writing any value to the trigger register is equivalent to performing the following operations:
			   1. Update the value of the trigger register with the new value.
			   2. Write a "Start" command into the DMA_ACT register. The DMA_GCSW.SCMD field
			      will be used as the sub-action command for the "Start" command. */
			__nds32__mtsr_dsb(src_addr, NDS32_SR_DMA_ESADDR);
		}
		break;
	}

	return 0;
}

/* Interrupt handlers */
void ldma_irq_handler(void)
{
	unsigned int error;

	if (0x4 != (0x7 & __nds32__mfsr(NDS32_SR_DMA_HSTATUS))) {
		// printf("LMDMA head ch not complete!\n");
		error = 1;

	} else {
		error = 0;

		/* Although writing any value into this register will dequeue the DMA Channel Queue, we
		   recommend you to use the register ($r0) as the source register. Any of the following commands
		   will dequeue the DMA Channel Queue regardless of the value in the register.
		   mtsr $r0, $DMA_HSTATUS
		   mtusr $r0, $DMA_HSTATUS */
		__nds32__mtsr_dsb(0x0, NDS32_SR_DMA_HSTATUS);

		/* Without extra reset action command to idle state, un-writable if fast-start mode */
		if ((__nds32__mfsr(NDS32_SR_DMA_GCSW) & DMA_GCSW_mskFSM) == DMA_GCSW_FSM_ACMD) {
			__nds32__mtsr_dsb((DMA_ACT_ACMD_RESET | (DMA_GCSW_SCMD_NOCOND << 2)), NDS32_SR_DMA_ACT);
		}

		/* turn off DMA, reset tail & head ch to 0 */
		__nds32__mtsr_dsb(0x0, NDS32_SR_DMA_GCSW);
	}

	/* trigger user callback */
	if (NULL != g_lmdma_cb)
	{
		g_lmdma_cb(error);
	}
}
