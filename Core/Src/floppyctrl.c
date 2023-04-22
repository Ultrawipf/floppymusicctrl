/*
 * spictrl.c
 *
 *  Created on: Nov 20, 2022
 *      Author: Yannick
 */
#include <floppyctrl.h>
#include <string.h>
#include "helpers.h"
#include "fdd_data.h"

uint8_t stepState = 0;
double curfreq=0;
uint32_t masterclockfreq; // Default internal main clock

volatile uint8_t rxbuf[BUFFERSIZE] = {0};
volatile uint8_t txbuf[BUFFERSIZE] = {0};

spi_cmd nextcmd;
spi_cmd replycmd;

uint8_t dataptr = 0;
volatile uint8_t address = 0;
uint32_t stepnum = 80;
uint32_t spiErrors = 0;

uint8_t lastpacket[BUFFERSIZE] = {0};

volatile uint8_t track0_fired = 0;

uint8_t resetCountOnTrk0 = 0;

enum STATE {state_none,state_normal,state_execute_cmd,state_findaddress};
enum STATE state = state_normal;


uint16_t getTracks(){
	return stepnum;
}

uint16_t getTrack(){
	uint16_t track = DIRTIM.Instance->CNT;
	if(track > getTracks()){
		return getTracks()*2 - track;
	}
	return track;
}

void setTrack(uint16_t track){

	DIRTIM.Instance->CNT = track;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin){
//	if(GPIO_Pin == NSS_Pin){
//		setSSI(HAL_GPIO_ReadPin(NSS_GPIO_Port, NSS_Pin),&hspi1);
//	}
	if(GPIO_Pin == ADR_IN_Pin){
		if(state == state_findaddress)
			address++;
	}
}
// React to address id pin
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
//	if(GPIO_Pin == NSS_Pin){
//		setSSI(HAL_GPIO_ReadPin(NSS_GPIO_Port, NSS_Pin),&hspi1);
//	}
	if(GPIO_Pin == FDD_TRK0_Pin && resetCountOnTrk0){
		track0_fired  = 1;
		// Reset timer
		setTrack(0);
	}else if(GPIO_Pin == IDXPIN){
		handleIndex();
	}


}

void findAddress(){
	state = state_findaddress;
	setPulseFreq(0);
	address = 0; // reset
	uint8_t timeout = 0xff;
	uint8_t found_address = 0;
	HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
	HAL_Delay(10); // Initial wait to make sure all controllers are in this state
	// Pulse id pin once
	HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
	uint8_t lastaddr = address;
	while(timeout-- && !found_address){


		uint8_t timeout2 = 0xff;
		while(timeout2--){
			if(lastaddr != address){
				break;
			}
			HAL_Delay(2);
		}

		if(lastaddr == address && HAL_GPIO_ReadPin(ADR_IN_GPIO_Port, ADR_IN_Pin))
		{ // The one before us found its address. we pulse one more time
			found_address = 1;
			if(address == 0)
				address = 1;
//			HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_SET);
//			HAL_Delay(1);
//			HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
//			HAL_Delay(1);
			break;
		}else{
			timeout = 0xff; // Reset timeout
		}
		lastaddr = address;
	}

	for(uint8_t i = 0;i< address;i++){
		HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
		HAL_Delay(1);
		HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_SET);
		HAL_Delay(1);
	}

	if(found_address){
		address--; // We counted one too much in the initial test pulse

		// Pulse out our address on the enable pin
		setPulseFreq(0);
		for(uint8_t i = 0;i< address;i++){
			HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET); // Enable drive
			HAL_Delay(250);
			HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET); // Enable drive
			HAL_Delay(250);
		}
		//HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
	}
	// HAL_GPIO_WritePin(ADR_OUT_GPIO_Port, ADR_OUT_Pin, GPIO_PIN_RESET);
	state = state_normal;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){

}


void setSSI(uint8_t on,SPI_HandleTypeDef *hspi){
	if(on)
		hspi1.Instance->CR1 |= SPI_CR1_SSI;
	else
		hspi1.Instance->CR1 &= ~SPI_CR1_SSI;
}

void setEnabled(uint8_t enabled){
	HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET); // Enable drive
}

void homeHeads()
{

	setPulseFreq(0); // Disable
	gpioStepPin(); // Get step pin as output
	setEnabled(1);

	// Drive forward some steps to get off the index
	setTrack(0);
	for(uint8_t i = 0; i<5;i++)
	{
		HAL_GPIO_WritePin(FDD_STEP_GPIO_Port, FDD_STEP_Pin,0);
		HAL_Delay(1);
		HAL_GPIO_WritePin(FDD_STEP_GPIO_Port, FDD_STEP_Pin,1);
		HAL_Delay(1);
	}
	track0_fired = 0;
	resetCountOnTrk0 = 1;
	setTrack(stepnum+1);
	for(uint8_t i = 0; i<stepnum*2;i++)
	{
		if(track0_fired){
			track0_fired = 0;
			break;
		}
		HAL_GPIO_TogglePin(FDD_STEP_GPIO_Port, FDD_STEP_Pin);
		HAL_Delay(1);
	}
	setTrack(0); // Reset direction timer
	setEnabled(0); // Disable drive
	reinitTimerStepPin();
	resetCountOnTrk0 = 0;
}

void setSteps(uint32_t steps){
	stepnum = steps;
	DIRTIM.Instance->CCR1 = steps;
	DIRTIM.Instance->ARR = (steps*2)-1;
}

uint8_t getAddress(){
	return address;
}

/**
 * Executes commands inside the rx interrupt (for replies)
 */
void executeCmd_IT(spi_cmd* cmd){
	state = state_normal; // reset state if command in here is found
	switch(cmd->cmd){
	case CMD_READ:
		{ // Read data
			replycmd.adr = address;
			replycmd.cmd = cmd->cmd;
			// TODO reply something

			spiBeginTransmit((uint8_t*)&replycmd,sizeof(replycmd));
		}
		break;

		case CMD_REPLY_ADR:
		{ // Read data
			replycmd.adr = address;
			replycmd.cmd = CMD_REPLY_ADR;
			replycmd.val1 = address;

			spiBeginTransmit((uint8_t*)&replycmd,sizeof(replycmd));
		}
		break;
		case CMD_REPLY_SPIERR:
		{ // Read data
			replycmd.adr = address;
			replycmd.cmd = CMD_REPLY_SPIERR;
			replycmd.val1_32 = spiErrors;

			spiBeginTransmit((uint8_t*)&replycmd,sizeof(replycmd));
		}
		break;
		default:
			state = state_execute_cmd; // Command not found. reset state to execute outsite interrupt
			break;
	}
}

/**
 * Executes commands outside of the rx interrupt
 */
void executeCmd(spi_cmd* cmd)
	{
	if(cmd->adr == 0xff && cmd->cmd == 0x0A){
		// Broadcast address detection
		findAddress();
	}

	switch(cmd->cmd){
	case CMD_SET_STEPS:
	{ // Read data
		setSteps(cmd->val1 | cmd->val2 << 8);
		resetCountOnTrk0 = cmd->val4 ? 1 : 0;
	}
	break;
	case CMD_PLAYFREQ:
	{ // Note on
		uint8_t buf[4] = {cmd->val1,cmd->val2,cmd->val3,cmd->val4};
		setPulseFreq(bytesToFloat(buf,0));

	}
	break;
	case CMD_PLAYFREQ_FIX:
	{ // Note on
		float f = cmd->val1_32;
		setPulseFreq(f/1000.0);

	}
	break;
	case CMD_RESET:
	{
		homeHeads();
	}
	break;
	case CMD_SETENABLE:
	{
		setEnabled(cmd->val1);
		setMotor(cmd->val2);
	}
	break;
	case CMD_DATASTATUS:
	{
		sendDataStatus();
	}
	break;
	case CMD_READSECTOR:
		sendSectorSpi(cmd->val1_16);
		break;
	case CMD_DATAMODE:
		enableDataMode(cmd->val1);
		setMotor(cmd->val3);
		if(cmd->val2)
			beginReadData();
		else
			stopReadData();
		break;
	case CMD_SET_EXTCLK:
	{
		setExtClkMode(cmd->val1_32);
		break;
	}

	default:

		break;
	}
}

void update(){

	if(state == state_execute_cmd){
		executeCmd(&nextcmd);
		state = state_normal;
	}


}

HAL_StatusTypeDef spiBeginReceive(){
	return HAL_SPI_Receive_DMA(&hspi1,(uint8_t*)&rxbuf,PACKETSIZE);
}

HAL_StatusTypeDef spiBeginTransmit(uint8_t *pData, uint16_t Size){
	return HAL_SPI_Transmit_DMA(&hspi1,pData,Size); // Size + CRC
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi){
	// Got an error. ignore and retry receiving
	spiErrors++;
	initializeComms();
	// Toggle led
	HAL_GPIO_TogglePin(FDD_HEAD_GPIO_Port, FDD_HEAD_Pin);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
	if(hspi == &hspi1){
		spiBeginReceive();
		sendSpiData_done();
	}
}

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi){

	if(hspi != &hspi1){
		return;
	}
	uint8_t transmitted = 0;
	//setSSI(0,&hspi1);
	if((rxbuf[0] == address || rxbuf[0] == 0xff) && state == state_normal){
		state = state_execute_cmd;
		if(rxbuf[1] & CMDMASK_READ){
			// Read data
			transmitted = 1;
		/// Wait for transmit in mainloop.
		}
	}
	if(state == state_execute_cmd){
		memcpy(&nextcmd,rxbuf,sizeof(nextcmd)); // Buffer command. Maybe disable interrupts during copy
		executeCmd_IT(&nextcmd);
	}

	if(!transmitted)
		if(spiBeginReceive() != HAL_OK){

		}
	//setSSI(1,&hspi1);
}

// Reconfigures step pin as output gpio
void gpioStepPin(){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = FDD_STEP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(FDD_STEP_GPIO_Port, &GPIO_InitStruct);
}

void reinitTimerStepPin(){
	HAL_GPIO_DeInit(FDD_STEP_GPIO_Port, FDD_STEP_Pin);
	MX_TIM1_Init();
}

// Period in
void setPulseFreq(float freq){

	if(freq == 0){
		//STEPTIM.Instance->CR1 = 0;
		HAL_TIM_PWM_Stop(&STEPTIM, TIM_CHANNEL_1);
		stepState = 0;
		HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
	}else{
		HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
		// Set frequency and scalers
		//uint32_t period_ns = 1e9 / freq;
		uint32_t arr_raw = (masterclockfreq/freq);
		uint32_t prescaler = arr_raw / 0xffff;
		uint32_t arr = arr_raw / (prescaler + 1);

		STEPTIM.Instance->PSC = prescaler;
		STEPTIM.Instance->ARR = arr;
		uint32_t width = (PULSEWIDTH / (prescaler + 1) ) + 1;
		STEPTIM.Instance->CCR1 = width < arr ? width : (arr / 2) + 1;


		if(!stepState){
			stepState = 1;
			HAL_TIM_PWM_Start(&STEPTIM, TIM_CHANNEL_1);
		}
	}

}

/**
 * Switches between external ETR2 and internal HCLK clock source on steptimer
 */
void setExtClkMode(uint32_t extClkPeriod){
	if(extClkPeriod){
		// pin config
//		HAL_TIM_Base_DeInit(&STEPTIM);
//		HAL_TIM_Base_Init(&STEPTIM);
		// Clock config

		TIM_ClockConfigTypeDef sClockSourceConfig = {0};
		sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
		sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_NONINVERTED;
		sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
		sClockSourceConfig.ClockFilter = 0;
		HAL_TIM_ConfigClockSource(&STEPTIM, &sClockSourceConfig);
//		HAL_TIM_PWM_Init(&STEPTIM);

		// Enable ext trigger on timer STEPTIM

		GPIO_InitTypeDef GPIO_InitStruct = {0};
		HAL_GPIO_DeInit(EXTCLK_GPIO_Port, EXTCLK_Pin);
		GPIO_InitStruct.Pin = EXTCLK_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
		HAL_GPIO_Init(EXTCLK_GPIO_Port, &GPIO_InitStruct);

		masterclockfreq = extClkPeriod;
		setPulseFreq(0);
	}else{
		// Reset to internal clock
		masterclockfreq = HAL_RCC_GetHCLKFreq();
		TIM_ClockConfigTypeDef sClockSourceConfig = {0};
		sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	    HAL_TIM_ConfigClockSource(&STEPTIM, &sClockSourceConfig);
	}

}

void initializeTimers(){
	masterclockfreq = HAL_RCC_GetHCLKFreq();
	setSteps(stepnum);
	// Start PWM output on timers
	setPulseFreq(0);
	//HAL_TIM_PWM_Start(&STEPTIM, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&DIRTIM, TIM_CHANNEL_1);

}


void initializeComms(){
	// Start SPI
	// setSSI(1,&hspi1);
//	memset(txbuf, 0, sizeof(txbuf)); // Clear tx buffer
//	HAL_SPIEx_FlushRxFifo(&hspi1);
//	HAL_SPI_DMAStop(&hspi1);
	if(spiBeginReceive() != HAL_OK){

	}
	//setSSI(1,&hspi1);

}
