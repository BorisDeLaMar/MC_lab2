#include <stdio.h>
#include "uart.h"
#include "math.h"

extern  UART_HandleTypeDef huart2;

uint8_t rx_buff[MAX_PACKET_LENGTH]; // receive
uint8_t tx_buff[MAX_PACKET_LENGTH]; // transmit
uint8_t rx_length = 0;
uint8_t tx_length = 0;

uint32_t leds_pin_arr[] = {LED1_Pin, LED2_Pin, LED3_Pin, LED4_Pin, LED5_Pin, LED6_Pin, LED7_Pin, LED8_Pin};

void nulify_buffs() {

	for(size_t i = 0; i < rx_length; i++) {
		rx_buff[i] = 0;
	}
	for(size_t i = 0; i < tx_length; i++) {
		tx_buff[i] = 0;
	}

	rx_length = 0;
	tx_length = 0;
}

// returns bin leds mask
uint8_t leds_cur_mask() {
	uint8_t bit_mask = 0;

	for(size_t i = 0; i < NUM_OF_LEDS; i++) {
		if(HAL_GPIO_ReadPin(GPIOC, leds_pin_arr[i]) == GPIO_PIN_SET)
			bit_mask += pow(2, i);
	}

	return bit_mask;
}

void parse_packet(struct header packet) {
	switch(packet.request_id) {
		case Ping:
			break;
		case All_leds_on:
			for(size_t i = 0; i < NUM_OF_LEDS; i++) {
				HAL_GPIO_WritePin(GPIOC, leds_pin_arr[i], GPIO_PIN_SET);
			}
			break;
		case All_leds_off:
		{
			for(size_t i = 0; i < NUM_OF_LEDS; i++) {
				HAL_GPIO_WritePin(GPIOC, leds_pin_arr[i], GPIO_PIN_RESET);
			}
			break;
		}
		case Mask_leds_on:
		{
			uint32_t length = packet.data_size;
			uint32_t value = 0; size_t counter = 0;

			while(length) {
				value += (packet.data[counter])*pow(10, length-1);

				counter++;
				length--;
			}
			for(size_t i = 0; i < NUM_OF_LEDS; i++) {
				HAL_GPIO_WritePin(GPIOC, leds_pin_arr[i], value%2);
				value /= 2;
			}
			break;
		}
		case Get_leds_on:
			break;
	}

	if(packet.request_id == Get_leds_on) {
		tx_length = 0;

		tx_buff[tx_length] = START_BIT;
		tx_length += 1;

		uint8_t mask = leds_cur_mask();
		for(size_t i = 0; i < 8; i++) {
				tx_buff[tx_length] = mask%2 + NUM_ASCII_OFFSET;
				tx_length += 1;

				mask /= 2;
		}

		tx_buff[tx_length] = STOP_BIT;
		tx_length += 1;
	}
	else {
		for(size_t i = 0; i <= rx_length+1; i++) {
			tx_buff[i] = rx_buff[i];
		}
		tx_length = rx_length+1;
	}
	tx_buff[tx_length] = '\n';
	tx_length++;
	tx_buff[tx_length] = '\r';
	tx_length++;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) { // срабатывает в момент окончания передачи данных
	struct header packet;

	if(huart == &huart2) {
		if(rx_buff[rx_length] == START_BIT) {
			rx_length += 1;

			packet.request_id = rx_buff[rx_length];
			packet.data_size = rx_buff[rx_length+1] - NUM_ASCII_OFFSET;

			rx_length += 2;

			if(packet.data_size == 0) {
				parse_packet(packet);
			} else {
				for(size_t i = 0; i < packet.data_size; i++) {
					packet.data[i] = rx_buff[rx_length+i] - NUM_ASCII_OFFSET;
				}

				rx_length += (packet.data_size);
				parse_packet(packet);
			}
			if(rx_buff[rx_length] == STOP_BIT) {
				HAL_UART_Transmit(&huart2, tx_buff, tx_length, 20);
			}
		}
	}

	nulify_buffs();
	HAL_UART_Receive_IT(&huart2, rx_buff, PACKET_LENGTH);
}
