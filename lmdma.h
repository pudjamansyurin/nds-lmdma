/*
 * lmdma.h
 *
 *  Created on: Dec 8, 2022
 *      Author: pujak
 */

#ifndef NDS_LMDMA_LMDMA_H_
#define NDS_LMDMA_LMDMA_H_

#include "nds32_defs.h"

/* NDS32_DMA_GCSW system register */
#define DMA_GCSW_FSM_ACMD			(0x0 << DMA_GCSW_offFSM)
#define DMA_GCSW_FSM_FSTART_TCNT	(0x1 << DMA_GCSW_offFSM)
#define DMA_GCSW_FSM_FSTART_ISADDR	(0x2 << DMA_GCSW_offFSM)
#define DMA_GCSW_FSM_FSTART_ESADDR	(0x3 << DMA_GCSW_offFSM)

#define DMA_GCSW_SCMD_NOCOND		(0x0 << DMA_GCSW_offSCMD)
#define DMA_GCSW_SCMD_ENQ			(0x1 << DMA_GCSW_offSCMD)
#define DMA_GCSW_SCMD_WAITDBS		(0x2 << DMA_GCSW_offSCMD)
#define DMA_GCSW_SCMD_ENQWAIT		(0x3 << DMA_GCSW_offSCMD)

#define DMA_GCSW_DMA_ENGINE_OFF		(0x0 << DMA_GCSW_offEN)
#define DMA_GCSW_DMA_ENGINE_ON		(0x1 << DMA_GCSW_offEN)

/* NDS32_DMA_SETUP system register */
#define DMA_SETUP_ILM				(0x0 << DMA_SETUP_offLM)
#define DMA_SETUP_DLM				(0x1 << DMA_SETUP_offLM)
#define DMA_SETUP_DDR_LM			(0x0 << DMA_SETUP_offTDIR)
#define DMA_SETUP_LM_DDR			(0x1 << DMA_SETUP_offTDIR)
#define DMA_SETUP_BYTE				(0x0 << DMA_SETUP_offTES)
#define DMA_SETUP_HALF_WORD			(0x1 << DMA_SETUP_offTES)
#define DMA_SETUP_WORD				(0x2 << DMA_SETUP_offTES)
#define DMA_SETUP_DOUBLE_WORD		(0x3 << DMA_SETUP_offTES)
#define DMA_SETUP_ESTR(X)			((X) << DMA_SETUP_offESTR)
#define DMA_SETUP_ESTR_1			(0x1 << DMA_SETUP_offESTR)
#define DMA_SETUP_ESTR_4			(0x1 << DMA_SETUP_offESTR)
#define DMA_SETUP_CIE_EN			(0x1 << DMA_SETUP_offCIE)
#define DMA_SETUP_SIE_EN			(0x1 << DMA_SETUP_offSIE)
#define DMA_SETUP_EIE_EN			(0x1 << DMA_SETUP_offEIE)
#define DMA_SETUP_UE_EN				(0x1 << DMA_SETUP_offUE)
#define DMA_SETUP_2DE_EN			(0x1 << DMA_SETUP_off2DE)
#define DMA_SETUP_NONCACHE			(0x2 << DMA_SETUP_offCOA)

/* NDS32_DMA_2DSET system register */
#define DMA_2DSET_WECNT(X)			((X) << DMA_2DSET_offWECNT)
#define DMA_2DSET_HTSTR(X)			((X) << DMA_2DSET_offHTSTR)

/* NDS32_DMA_2DSCTL system register */
#define DMA_2DSCTL_STWECNT(X)		((X) << DMA_2DSCTL_offSTWECNT)

/* Public types */
typedef void (*lmdma_callback_t)(unsigned int error);

/* Public function declarations */
int lmdma_initialize(lmdma_callback_t cb);
int lmdma_uninitialize(void);
int lmdma_abort(void);
int lmdma_get_ch_count(void);
int lmdma_ch_get_count(unsigned char ch);
int lmdma_ch_configure(unsigned int ch,
				unsigned int src_addr,
				unsigned int dst_addr,
				unsigned int size,
				unsigned int gcsw_ctl,
				unsigned int setup_ctl,
				unsigned int setup_2d_ctl,
				unsigned int startup_2d_ctl);

#endif /* NDS_LMDMA_LMDMA_H_ */
