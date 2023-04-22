/*
 * fdd_data.h
 *
 *  Created on: Dec 11, 2022
 *      Author: Yannick
 */

#ifndef INC_FDD_DATA_H_
#define INC_FDD_DATA_H_
#include "main.h"

#define MFM_TRANSFERBUFFER 512UL // 2x buffer allocated
#define MFM_BUFFER 128UL // Half buffer per transfer
#define MFM_SECTORS_PER_TRACK 18
#define MFM_TIM_CNTPERUS 4 // 64/15+1
#define MFM_SHORT_TH 2*MFM_TIM_CNTPERUS+1
#define MFM_LONG_TH 3*MFM_TIM_CNTPERUS+1
// Pulses can be high about 1.5µs, 2.5µs or 3.5µs long
#define MFM_SECTOR_BEGIN 0xA1A1A1F8
#define MFM_SECTOR_BEGIN2 0xA1A1A1FB
#define MFM_INDEX_MARKER 0xff

#define READSTATUS_DONE 0x01
#define READSTATUS_TRACKDONE 0x02
#define READSTATUS_OVERRUN 0x04

#define DATATIM htim17
extern TIM_HandleTypeDef DATATIM; // FDD data timer
#define DATATIM_DMA hdma_tim17_ch1
extern DMA_HandleTypeDef DATATIM_DMA;

#define IDXPIN SWD_IDX_Pin

void handleData(uint8_t complete);
void handleIndex();
void enableDataMode(uint8_t enable);
void setMotor(uint8_t state);
void gotoTrack(uint32_t track);
void sendSectorSpi(uint16_t length);
void beginReadData();
void stopReadData();
void sendDataStatus();
void sendSpiData_done();
#endif /* INC_FDD_DATA_H_ */
