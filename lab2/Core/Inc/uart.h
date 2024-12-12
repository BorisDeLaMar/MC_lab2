#ifndef INC_UART_H
#define INC_UART_H

#include "main.h"

#define MAX_PACKET_LENGTH 512
#define START_BIT 0x7A
#define STOP_BIT 0x79
#define NUM_OF_LEDS 8
#define NUM_ASCII_OFFSET 48

enum request {
	Ping = 0x01,
	All_leds_on = 0x02,
	All_leds_off = 0x03,
	Mask_leds_on = 0x04,
	Get_leds_on = 0x05
};

struct header {
	uint8_t request_id;
	uint8_t data_size;
	uint8_t data[512];
};

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

#endif /* INC_UART_H */
