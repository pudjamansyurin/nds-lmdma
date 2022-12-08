/*
 * lm.h
 *
 *  Created on: Dec 8, 2022
 *      Author: pujak
 */

#ifndef NDS_LMDMA_LM_H_
#define NDS_LMDMA_LM_H_

/* Public types */
typedef enum {
	LM_ILM,
	LM_DLM,
} LM_TYPE;

/* Public function declaration */
int lm_initialize(LM_TYPE type, unsigned int base);
unsigned int lm_get_size(LM_TYPE type);

#endif /* NDS_LMDMA_LM_H_ */
