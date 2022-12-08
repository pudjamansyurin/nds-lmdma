# Andes Local Memory DMA
### Make sure your Platform and CPU implement LMDMA support (MSC_CFG.LMDMA)

#### **`main.c`**
```c
#include "nds-lmdma/lm.h"
#include "nds-lmdma/lmdma.h"

/* External variables */
extern unsigned int* p_data_src;
extern unsigned int* p_data_dst;
extern unsigned int data_count;

/* Private variables */
static volatile uint8_t u8_lmdma_busy;

/* Private function definitions */
static void lmdma_callback(unsigned int error)
{
	u8_lmdma_busy = 0;
}

/* Entry point */
int main(void) 
{
	uint32_t ilm_base, dlm_base;
	uint32_t ilm_size, dlm_size;
	uint32_t lmdma_ch_cnt;
	uint8_t lmdma_ch;
	int rc;

	lmdma_ch = 0;
	ilm_base = 0xA00000;
	dlm_base = 0xC00000;

	// Initialize Instruction Local Memory (ILM)
	rc = lm_initialize(LM_ILM, ilm_base);
	if (-1 != rc) {
		ilm_size = lm_get_size(LM_ILM);
	} else {
		ilm_size = 0;
	}

	// Initialize Data Local Memory (DLM)
	rc = lm_initialize(LM_DLM, dlm_base);
	if (-1 != rc) {
		dlm_size = lm_get_size(LM_DLM);
	} else {
		dlm_size = 0;
	}
	
	// Initialize Local Memory - DMA
	rc = lmdma_initialize(lmdma_callback);
	if (-1 != rc) {
		lmdma_ch_cnt = lmdma_get_ch_count();
	} else {
		lmdma_ch_cnt = 0;
	}

	// Send data using LMDMA
	u8_lmdma_busy = 1;

	rc = lmdma_ch_configure(lmdma_ch,
		(unsigned int) p_data_src,
		(unsigned int) p_data_dst,
		data_count,
		(DMA_GCSW_FSM_FSTART_ESADDR | DMA_GCSW_SCMD_ENQ | DMA_GCSW_DMA_ENGINE_ON),
		(DMA_SETUP_ILM | DMA_SETUP_LM_DDR | DMA_SETUP_BYTE | DMA_SETUP_ESTR(0) |
		DMA_SETUP_CIE_EN | DMA_SETUP_SIE_EN | DMA_SETUP_EIE_EN | DMA_SETUP_UE_EN | DMA_SETUP_NONCACHE),
		0,
		0);

#ifdef LMDMA_BLOCKING_EN
	/* blocking wait LMDMA */
	if (-1 != rc) {
		while(u8_lmdma_busy) {};
	} 
#endif

	while(1) {
		// Do other task in parallel
	}

	return 0;
}
```