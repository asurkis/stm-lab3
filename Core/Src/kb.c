#include "main.h"
#include "pca9538.h"
#include "kb.h"
#include "sdk_uart.h"
#include "usart.h"

#define KBRD_RD_ADDR 0xE3
#define KBRD_WR_ADDR 0xE2

HAL_StatusTypeDef Set_Keyboard(void) {
	HAL_StatusTypeDef ret = HAL_OK;
	uint8_t buf;

	buf = 0x70;
	ret = PCA9538_Write_Register(KBRD_WR_ADDR, CONFIG, &buf);
	if (ret != HAL_OK) {
		UART_Transmit("Error write config\r\n");
		goto exit;
	}

	buf = 0;
	ret = PCA9538_Write_Register(KBRD_WR_ADDR, OUTPUT_PORT, &buf);
	if (ret != HAL_OK) {
		UART_Transmit("Error write output\r\n");
	}

	exit: return ret;
}

uint8_t Check_Row(uint8_t Nrow) {
	uint8_t Nkey = 0x00;
	HAL_StatusTypeDef ret = HAL_OK;
	uint8_t buf[4] = { 0, 0, 0, 0 };
	uint8_t kbd_in;

	ret = Set_Keyboard();
	if (ret != HAL_OK) {
		UART_Transmit("Error write config\r\n");
	}

	buf[0] = Nrow;

	ret = PCA9538_Write_Register(KBRD_WR_ADDR, OUTPUT_PORT, buf);
	if (ret != HAL_OK) {
		UART_Transmit("Error write output\r\n");
	}

	buf[0] = 0;
	ret = PCA9538_Read_Inputs(KBRD_RD_ADDR, buf);
	if (ret != HAL_OK) {
		UART_Transmit("Read error\r\n");
	}
	kbd_in = buf[0] & 0x70;
	Nkey = 0;
	Nkey |= (kbd_in & 0x10) ? 0 : 1;
	Nkey |= (kbd_in & 0x20) ? 0 : 2;
	Nkey |= (kbd_in & 0x40) ? 4 : 0;
	return Nkey;
}
