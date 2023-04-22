/*
 * spictrl.h
 *
 *  Created on: Nov 20, 2022
 *      Author: Yannick
 */

#ifndef INC_FLOPPYCTRL_H_
#define INC_FLOPPYCTRL_H_
#include "main.h"
#include "command_defs.h"

extern SPI_HandleTypeDef hspi1;

extern TIM_HandleTypeDef htim1; // Step timer
extern TIM_HandleTypeDef htim3; // Dir timer


#define STEPTIM htim1
#define DIRTIM htim3
#define PULSEWIDTH 2000 // In clock cycles
#define PACKETSIZE 6 // 6
#define BUFFERSIZE 7 // 6 + crc

// PA14/PA13 change pin
// PA14 boot0
uint16_t getTracks();
uint16_t getTrack();
void setEnabled(uint8_t enabled);
void setTrack(uint16_t track);

void initializeTimers();
void setDirSteps(uint16_t steps);
void setPulseFreq(float freq);
void initializeComms();
void setSSI(uint8_t on,SPI_HandleTypeDef *hspi);
void homeHeads();
void setSteps(uint32_t steps);
void update(); // Mainloop
void executeCmd(spi_cmd* cmd);
void executeCmd_IT(spi_cmd* cmd);
uint8_t getAddress();

void setExtClkMode(uint32_t extClkPeriod);

void findAddress();

void gpioStepPin();
void reinitTimerStepPin();

HAL_StatusTypeDef spiBeginReceive();
HAL_StatusTypeDef spiBeginTransmit(uint8_t *pData, uint16_t Size);


#endif /* INC_FLOPPYCTRL_H_ */
