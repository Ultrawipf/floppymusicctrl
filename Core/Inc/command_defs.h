/*
 * command_defs.h
 *
 *  Created on: 25.11.2022
 *      Author: Yannick
 */

#ifndef INC_COMMAND_DEFS_H_
#define INC_COMMAND_DEFS_H_

typedef struct{
	uint8_t adr;
	uint8_t cmd;
	union{
		struct{
			uint8_t val1;
			uint8_t val2;
			uint8_t val3;
			uint8_t val4;
		};
		struct{
			uint16_t val1_16;
			uint16_t val2_16;
		};
		struct{
			uint32_t val1_32;
		};

	};

} __attribute__((packed)) spi_cmd;


#define ADR_BROADCAST 0xff
#define CMDMASK_READ 0x80

#define CMD_READ 0x7f
#define CMD_REPLY_ADR 0xDA
#define CMD_REPLY_SPIERR 0xDE

#define CMD_PLAYFREQ 0x01
#define CMD_PLAYFREQ_FIX 0x02
#define CMD_SETENABLE 0x03
#define CMD_RESET 0x10
#define CMD_SET_STEPS 0x11
#define CMD_SET_EXTCLK 0x12 // 0 to enable internal clock. anything else sets new reference step clock frequency in Hz

#define CMD_DATAMODE 0x20
#define CMD_DATASEEK 0x21 // Seek to track, head and sector
#define CMD_READSECTOR 0xD5 // Replies a full sector 512B
#define CMD_DATASTATUS 0xD1 // Current buffer and read status. could be 2B long instead of 6
#define CMD_DATASTATUSREPLY 0x51

#endif /* INC_COMMAND_DEFS_H_ */
