/*
 * lm.c
 *
 *  Created on: Dec 8, 2022
 *      Author: pujak
 */
#include "lm.h"
#include "ae210p.h"
#include "nds32_defs.h"

/* Public function definitions */
int lm_initialize(LM_TYPE type, unsigned int base)
{
	unsigned int cm_cfg;
	unsigned int lm_has;
	unsigned int lm_bsav;

	/* Check LM existence */
	if (LM_ILM == type) {
		cm_cfg = __nds32__mfsr(NDS32_SR_ICM_CFG);
		lm_has = cm_cfg & ICM_CFG_mskILMB;
	} else {
		cm_cfg = __nds32__mfsr(NDS32_SR_DCM_CFG);
		lm_has = cm_cfg & DCM_CFG_mskDLMB;
	}

	if (0 == lm_has) {
		//printf("The system has no LM \n");
		return -1;
	}

	/* Check LM base register alignment version */
	if (LM_ILM == type) {
		lm_bsav = (cm_cfg & ICM_CFG_mskBSAV) >> ICM_CFG_offBSAV;
	} else {
		lm_bsav = (cm_cfg & DCM_CFG_mskBSAV) >> DCM_CFG_offBSAV;
	}

	switch (lm_bsav) {
	case 0:
		// printf("LM base address is 1MB-aligned, Abort! \n");
		return -1;
	case 1:
		// printf("LM base address is aligned to local memory size that has to be power of 2! \n");
		break;
	default:
		// printf("Reserved, Abort! \n");
		return -1;
	}

	/* Initialize the LM */
	if (LM_ILM == type) {
		if(!(__nds32__mfsr(NDS32_SR_ILMB) & ILMB_mskIEN)) {
			/* Set ILM base and enable it, The base physical address of ILM. It has to be
			   aligned to multiple of ILM size. */
			__nds32__mtsr((base | ILMB_mskIEN), NDS32_SR_ILMB);
		}
	} else {
		if(!(__nds32__mfsr(NDS32_SR_DLMB) & DLMB_mskDEN)) {
			/* Set DLM base and enable it, The base physical address of DLM. It has to be
			   aligned to multiple of DLM size. */
			__nds32__mtsr((base | DLMB_mskDEN), NDS32_SR_DLMB);
		}
	}

	/* if cache available, disable it */
	if (cm_cfg & DCM_CFG_mskDSZ) {
		__nds32__mtsr(__nds32__mfsr(NDS32_SR_CACHE_CTL) & ~(CACHE_CTL_mskIC_EN | CACHE_CTL_mskDC_EN), NDS32_SR_CACHE_CTL);
	}

	return 0;
}

unsigned int lm_get_size(LM_TYPE type)
{
	unsigned int size;

	if (LM_ILM == type) {
		size = (ILMB_mskILMSZ & __nds32__mfsr(NDS32_SR_ILMB)) >> ILMB_offILMSZ;
	} else {
		size = (DLMB_mskDLMSZ & __nds32__mfsr(NDS32_SR_DLMB)) >> DLMB_offDLMSZ;
	}

	if (12 < size) {
		size = 0;
	} else {
		if (10 < size)
			size = 1 << (10 + size);	// 2048KB ~ 4096KB
		else if (8 < size)
			size = 1 << ( 1 + size);	// 1KB ~ 2KB
		else
			size = 1 << (12 + size);	// 4KB ~ 1024KB
	}

	return (size);
}
