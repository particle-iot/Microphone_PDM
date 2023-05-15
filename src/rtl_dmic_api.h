#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define SP_DMA_PAGE_SIZE	512   // 2 ~ 4096
#define SP_DMA_PAGE_NUM    	4

typedef struct {
	unsigned int sample_rate;
	unsigned int word_len;
	unsigned int mono_stereo;
	unsigned int direction;	
}SP_OBJ, *pSP_OBJ;

void dmic_setup(int sampleRate, bool stereoMode);

void dmic_flush();
unsigned char *dmic_ready();
void dmic_read(unsigned char *buf, size_t len);


#ifdef __cplusplus
}
#endif
