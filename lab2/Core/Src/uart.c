#include "uart.h"
#include "math.h"

extern  UART_HandleTypeDef huart2;

uint8_t rx_buff[MAX_PACKET_LENGTH]; // receive
uint8_t tx_buff[MAX_PACKET_LENGTH]; // transmit
uint8_t rx_length = 0;
uint8_t tx_length = 0;

uint32_t leds_pin_arr[] = {LED1_Pin, LED2_Pin, LED3_Pin, LED4_Pin, LED5_Pin, LED6_Pin, LED7_Pin, LED8_Pin};

struct header packet;

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
		for(size_t i = 0; i < NUM_OF_LEDS; i++) {
			HAL_GPIO_WritePin(GPIOC, leds_pin_arr[i], GPIO_PIN_RESET);
		}
		break;
	case Mask_leds_on:
		//uint8_t* value = packet.data;
		for(size_t i = 0; i < NUM_OF_LEDS; i++) {
			HAL_GPIO_WritePin(GPIOC, leds_pin_arr[i], packet.data[0]%2);
			packet.data[0] /= 2;
		}
		break;
	case Get_leds_on:
		break;
	}

	if(packet.request_id == Get_leds_on) {
		tx_length = 0;

		tx_buff[tx_length] = START_BIT;
		tx_length += 1;
		tx_buff[tx_length] = leds_cur_mask();
		tx_length += 1;
		tx_buff[tx_length] = STOP_BIT;
	}
	else {
		for(size_t i = 0; i < rx_length; i++) {
			tx_buff[i] = rx_buff[i];
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) { // срабатывает в момент окончания передачи данных
	if(huart == &huart2) {
		if(rx_buff[rx_length] == START_BIT) {
			rx_length += 1;
			HAL_UART_Receive_IT(&huart2, &rx_buff[rx_length], 2);
		}
		else if(rx_buff[rx_length] == STOP_BIT) {
			HAL_UART_Transmit_IT(&huart2, tx_buff, tx_length);

			nulify_buffs();
			HAL_UART_Receive_IT(&huart2, &rx_buff[rx_length], 1);
		}
		else if(rx_length == 1){
			packet.request_id = rx_buff[rx_length];
			packet.data_size = rx_buff[rx_length+1];

			rx_length += 2;

			if(packet.data_size == 0) {
				parse_packet(packet);
				HAL_UART_Receive_IT(&huart2, &rx_buff[rx_length], 1);
			} else {
				HAL_UART_Receive_IT(&huart2, &rx_buff[rx_length], packet.data_size);
			}
		}
		else {
			for(size_t i = 0; i < packet.data_size; i++) {
				packet.data[i] = rx_buff[rx_length+i];
			}

			parse_packet(packet);

			rx_length += packet.data_size;
			HAL_UART_Receive_IT(&huart2, &rx_buff[rx_length], 1);
		}
	}
}
