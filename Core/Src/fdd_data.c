/*
 * fdd_data.c
 *
 *  Created on: Dec 11, 2022
 *      Author: Yannick
 */


#include "fdd_data.h"
#include "floppyctrl.h"
#include "stdlib.h"
#include "string.h"
// TODO implement some reading function

enum FDD_DATA_STATE {fdddatastate_none,fdddatastate_active,fdddatastate_reading_data};
enum FDD_DATA_STATE fddDataState = fdddatastate_none;

uint16_t fddDataReadBuffer[MFM_BUFFER]; // Buffer for a sector. Can not store a full track in ram. Would be 9kiB
uint8_t fddDecodedSector[MFM_TRANSFERBUFFER*2 + 2]; // Half buffer used at a time + 2x crc
volatile uint8_t currentSector = 0;
volatile uint32_t currentSectorBit = 0;
volatile uint8_t requestedSector = 0;

volatile uint16_t lastIndexTime = 0;
volatile uint8_t indexHit = 0; // Check if the index was hit since last reset of this flag
volatile uint32_t indexPeriod = 0; // Rotation period in µs. Should be 300rpm = 200ms
volatile uint32_t curbit = 0;
volatile uint32_t indexcount = 0;
volatile uint32_t lastDataTime = 0;

uint32_t decodeBufferOffset = 0;
volatile uint8_t readStatus = 0; // Status flags for data reading
volatile uint8_t writingData = 0; // 1 during an ongoing SPI out transfer

// TODO is currently not called by the timer...
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim){

	if(htim == &DATATIM && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1){
		handleData(0);
	}
}

void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef* htim){

	if(htim == &DATATIM && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1){
		handleData(1);
	}
}


/**
 * Decode MFM data
 */
void handleData(uint8_t complete){
	if(fddDataState != fdddatastate_reading_data){
		return;
	}
	uint32_t dataOffset = (DATATIM_DMA.Instance->CNDTR - MFM_BUFFER/2) % MFM_BUFFER; // Start position of half buffer


	// Copies into larger buffer and processes differences
	for(uint32_t i = 0;i<MFM_BUFFER/2;i++){
		uint16_t* dataptr = (uint16_t*)(fddDataReadBuffer+((dataOffset + i) % MFM_BUFFER));
		uint16_t* prevdataptr = (uint16_t*)(fddDataReadBuffer+((dataOffset + i -1) % MFM_BUFFER));

		if(indexHit && lastIndexTime >= *prevdataptr && lastIndexTime <= *dataptr){
			indexHit = 0;
			fddDecodedSector[currentSectorBit+decodeBufferOffset] = MFM_INDEX_MARKER; //insert index marker
			currentSectorBit++;
		}

		if(indexcount >= 2){ // Done. Should be done if index was hit 2 times
			stopReadData();
			readStatus |= READSTATUS_TRACKDONE;
			// Fill rest of buffer and stop
			for(;i<MFM_BUFFER/2;i++){
				fddDecodedSector[currentSectorBit++ + decodeBufferOffset] = 0;
			}
			break;
		}if(currentSectorBit >= MFM_TRANSFERBUFFER){
			if(readStatus & READSTATUS_DONE){ // We are done and no transmission was finished
//				readStatus |= READSTATUS_OVERRUN;
//				stopReadData();
			}
			readStatus |= READSTATUS_DONE;

			break;
		}
		// If still in valid range calculate timing
		fddDecodedSector[currentSectorBit++ + decodeBufferOffset] = *dataptr - *prevdataptr; // Save differences
	}

	// TODO last active buffer section should be passed to SPI DMA now for streaming
}


/**
 * Rotation index reached
 */
void handleIndex(){
	currentSector = 0;
	currentSectorBit = 0;
	indexPeriod = (DATATIM.Instance->CNT - lastIndexTime) / (MFM_TIM_CNTPERUS);
	lastIndexTime = DATATIM.Instance->CNT;
	indexHit = 1;
	// After a few rotations data should be ready
	if(fddDataState == fdddatastate_reading_data){
		if(indexcount++ > 5){
			// Stop motor, ready
			stopReadData();
		}
	}
}

void gotoTrack(uint32_t track){
	if(track >= getTracks()){
		track = getTracks()-1;
	}
	setEnabled(1);
	gpioStepPin();
	// Step to track
	if(track > getTrack()){
		setTrack(getTracks() + track);
	}

	for(int16_t t = 0 ; t < abs((int)getTrack() - (int)track); t++){
		HAL_GPIO_WritePin(FDD_STEP_GPIO_Port, FDD_STEP_Pin,GPIO_PIN_RESET);
		HAL_Delay(5);
		HAL_GPIO_WritePin(FDD_STEP_GPIO_Port, FDD_STEP_Pin,GPIO_PIN_SET);
		HAL_Delay(5);
	}
	setTrack(track);
	setEnabled(0);
}

void sendSectorSpi(uint16_t length){
	readStatus &= ~READSTATUS_DONE; // Reset status
	writingData = 1;
	currentSectorBit = 0; // Reset
	//uint32_t offset = decodeBufferOffset;
	uint32_t offset = (decodeBufferOffset + MFM_TRANSFERBUFFER) % (MFM_TRANSFERBUFFER*2); // The unused portion of the buffer
	spiBeginTransmit((uint8_t*)&fddDecodedSector+offset,length < MFM_TRANSFERBUFFER ? length : MFM_TRANSFERBUFFER);
}

void sendSpiData_done(){
	if(writingData){
		writingData = 0;
		decodeBufferOffset = (decodeBufferOffset + MFM_TRANSFERBUFFER) % (MFM_TRANSFERBUFFER*2); // The unused portion of the buffer
		//memset((uint8_t*)&fddDecodedSector+decodeBufferOffset,0,MFM_TRANSFERBUFFER);
	}
}

void sendDataStatus(){
	spi_cmd replycmd;
	replycmd.adr = getAddress();
	replycmd.cmd = CMD_DATASTATUSREPLY;
	replycmd.val1 = readStatus;
	replycmd.val2 = 0;
	replycmd.val2_16 = MFM_TRANSFERBUFFER;// available buffer size
	spiBeginTransmit((uint8_t*)&replycmd,sizeof(replycmd));
}

void beginReadData(){
	indexHit = 0;
	lastIndexTime = 0;
	indexcount = 0;
	currentSectorBit = 0;
	lastDataTime = 0;
	readStatus = 0; // Reset status
	setEnabled(1);
	setMotor(1);
	DATATIM.Instance->CNT = 0;
	HAL_TIM_IC_Start_DMA(&DATATIM, TIM_CHANNEL_1,fddDataReadBuffer,MFM_BUFFER);
	fddDataState = fdddatastate_reading_data;
}

void stopReadData(){
	indexcount = 0;
	fddDataState = fdddatastate_active;
	HAL_TIM_Base_Stop(&DATATIM);
	HAL_TIM_IC_Stop_DMA(&DATATIM, TIM_CHANNEL_1);
	//setMotor(0);
	setEnabled(0);
}

/**
 * Starts or stops the spindle motor
 */
void setMotor(uint8_t state){
	HAL_GPIO_WritePin(FDD_MOT_GPIO_Port, FDD_MOT_Pin, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void enableDataMode(uint8_t enable){
	if(enable && fddDataState == fdddatastate_none){
		homeHeads();
		// TODO Switch SWD pin to index mode
//		GPIO_InitTypeDef GPIO_InitStruct = {0};
//		GPIO_InitStruct.Pin = SWD_IDX_Pin;
//		GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
//		GPIO_InitStruct.Pull = GPIO_PULLUP;
//		HAL_GPIO_Init(SWD_IDX_GPIO_Port, &GPIO_InitStruct);

		HAL_TIM_Base_Start_IT(&DATATIM);
//		HAL_TIM_IC_Start_IT(&DATATIM, TIM_CHANNEL_1);
		fddDataState = fdddatastate_active;
		gpioStepPin();
	}
}
