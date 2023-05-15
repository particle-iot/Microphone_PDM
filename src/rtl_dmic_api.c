#ifdef rtl872x

#include "rl6548.h" // RTL standard peripheral audio driver

#include "rtl_dmic_api.h"

// These should be in rtl8721d_pinmux_defines.h but aren't getting picked up
#define _PB_1		(0x21)	//0x484 = DMIC_CLK - A0
#define _PB_2		(0x22)	//0x488 = DMIC_DATA - A1

#define SP_FULL_BUF_SIZE		128

typedef struct {
	GDMA_InitTypeDef       	SpRxGdmaInitStruct;              //Pointer to GDMA_InitTypeDef	
}SP_GDMA_STRUCT, *pSP_GDMA_STRUCT;


typedef struct {
	u8 rx_gdma_own;
	u32 rx_addr;
	u32 rx_length;
	
}RX_BLOCK, *pRX_BLOCK;

typedef struct {
	RX_BLOCK rx_block[SP_DMA_PAGE_NUM];
	RX_BLOCK rx_full_block;
	u8 rx_gdma_cnt;
	u8 rx_usr_cnt;
	u8 rx_full_flag;
	
}SP_RX_INFO, *pSP_RX_INFO;



static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_RX_INFO sp_rx_info;


//The size of this buffer should be multiples of 32 and its head address should align to 32 
//to prevent problems that may occur when CPU and DMA access this area simultaneously. 
SRAM_NOCACHE_DATA_SECTION static u8 sp_rx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM]__attribute__((aligned(32)));
static u8 sp_full_buf[SP_FULL_BUF_SIZE]__attribute__((aligned(32)));


// Can't figure out where this is defined
#define APP_DMIC_IN		((u32)0x00000002)

u8 *sp_get_ready_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	if (prx_block->rx_gdma_own)
		return NULL;
	else{
		return (u8*)prx_block->rx_addr;
	}
}

void sp_read_rx_page(u8 *dst, u32 length)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_usr_cnt]);
	
	if (dst) {
		memcpy(dst, (void const*)prx_block->rx_addr, length);
	}
	prx_block->rx_gdma_own = 1;
	sp_rx_info.rx_usr_cnt++;
	if (sp_rx_info.rx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_rx_info.rx_usr_cnt = 0;
	}
}

void sp_release_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (sp_rx_info.rx_full_flag){
	}
	else{
		prx_block->rx_gdma_own = 0;
		sp_rx_info.rx_gdma_cnt++;
		if (sp_rx_info.rx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_rx_info.rx_gdma_cnt = 0;
		}
	}
}

u8 *sp_get_free_rx_page(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);
	
	if (prx_block->rx_gdma_own){
		sp_rx_info.rx_full_flag = 0;
		return (u8*)prx_block->rx_addr;
	}
	else{
		sp_rx_info.rx_full_flag = 1;
		return (u8*)sp_rx_info.rx_full_block.rx_addr;	//for audio buffer full case
	}
}

u32 sp_get_free_rx_length(void)
{
	pRX_BLOCK prx_block = &(sp_rx_info.rx_block[sp_rx_info.rx_gdma_cnt]);

	if (sp_rx_info.rx_full_flag){
		return sp_rx_info.rx_full_block.rx_length;
	}
	else{
		return prx_block->rx_length;
	}
}


void sp_rx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 rx_addr;
	u32 rx_length;
	
	GDMA_InitStruct = &(gs->SpRxGdmaInitStruct);
	DCache_Invalidate(GDMA_InitStruct->GDMA_DstAddr, GDMA_InitStruct->GDMA_BlockSize<<2);
	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);

	sp_release_rx_page();
	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	GDMA_SetDstAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_length>>2);	
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
	//AUDIO_SP_RXGDMA_Restart(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, rx_addr, rx_length);
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	//PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k

	RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);	

	//set clock divider to gen clock sample_rate*256 from 98.304M.
	switch(psp_obj->sample_rate){
		case SR_48K:
			div = 8;
			break;
		case SR_96K:
			div = 4;
			break;
		case SR_32K:
			div = 12;
			break;
		case SR_16K:
			div = 24;
			break;
		case SR_8K:
			div = 48;
			break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
	PAD_CMD(_PB_1, DISABLE);
	PAD_CMD(_PB_2, DISABLE);
	Pinmux_Config(_PB_1, PINMUX_FUNCTION_DMIC); // DMIC_CLK - A0
	Pinmux_Config(_PB_2, PINMUX_FUNCTION_DMIC);	// DMIC_DATA - A1

}

static void sp_init_rx_variables(void)
{
	int i;

	sp_rx_info.rx_full_block.rx_addr = (u32)sp_full_buf;
	sp_rx_info.rx_full_block.rx_length = (u32)SP_FULL_BUF_SIZE;
	
	sp_rx_info.rx_gdma_cnt = 0;
	sp_rx_info.rx_usr_cnt = 0;
	sp_rx_info.rx_full_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_rx_info.rx_block[i].rx_gdma_own = 1;
		sp_rx_info.rx_block[i].rx_addr = (u32)sp_rx_buf+i*SP_DMA_PAGE_SIZE;
		sp_rx_info.rx_block[i].rx_length = SP_DMA_PAGE_SIZE;
	}
}


//
// External API
//

void dmic_setup(int sampleRate, bool stereoMode) {
    SP_OBJ sp_obj;

	switch(sampleRate) {
		case 8000:
			sp_obj.sample_rate = SR_8K;
			break;

		case 32000:
			sp_obj.sample_rate = SR_32K;
			break;

		case 16000:
		default:
			sp_obj.sample_rate = SR_16K;
			break;
	}

	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo = stereoMode ? CH_STEREO : CH_MONO;
	sp_obj.direction = APP_DMIC_IN;
	SP_OBJ *psp_obj = &sp_obj;

	int i = 0;
	u32 rx_addr;
	u32 rx_length;
	
	DBG_8195A("Audio dmic demo begin......\n");

	sp_init_hal(psp_obj);
	
	sp_init_rx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_RdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_RxStart(AUDIO_SPORT_DEV, ENABLE);	

	rx_addr = (u32)sp_get_free_rx_page();
	rx_length = sp_get_free_rx_length();
	AUDIO_SP_RXGDMA_Init(0, &SPGdmaStruct.SpRxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_rx_complete, (u8*)rx_addr, rx_length);	

    // Particle.connect();
}


void dmic_flush() {
	while(sp_get_ready_rx_page() != NULL) {
        sp_read_rx_page(NULL, 0);
    }  
}

unsigned char *dmic_ready() {
	return sp_get_ready_rx_page();
}

void dmic_read(unsigned char *buf, size_t len) {
	sp_read_rx_page(buf, len);
}



#endif
