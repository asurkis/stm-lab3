/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "buzzer.h"
#include "sdk_uart.h"
#include "kb.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum MainState {
	MS_IDLE, MS_PLAY, MS_EDIT,
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FIFO_BUFFER_SIZE 32
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t event_buf[FIFO_BUFFER_SIZE];
size_t write_ptr = 0;
size_t read_ptr = 0;
int btn_state_prev = 0;
int btn_state = 0;
char print_buf[256];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void InitBuffer() {
	for (size_t i = 0; i < FIFO_BUFFER_SIZE; ++i)
		event_buf[i] = 0;
}

void AppendBuffer(uint8_t num) {
	event_buf[write_ptr] = num;
	write_ptr = (write_ptr + 1) % FIFO_BUFFER_SIZE;
}

int ReadBuffer() {
	if (read_ptr == write_ptr) {
		return -1;
	}
	uint8_t num = event_buf[read_ptr];
	read_ptr = (read_ptr + 1) % FIFO_BUFFER_SIZE;
	return num;
}

void KB_Test(void) {
	static uint8_t const rows[4] = { 0xF7, 0x7B, 0x3D, 0x1E };
	static int current_row = 0;
	static int row_result[4] = { 0, 0, 0, 0 };

	if (ks_state == 0) {
		if (row_result[current_row] != ks_result) {
			uint8_t keyNum = 0;
			if (ks_result & 1) {
				AppendBuffer(3 * current_row + 1);
			}
			if (ks_result & 2) {
				AppendBuffer(3 * current_row + 2);
			}
			if (ks_result & 4) {
				AppendBuffer(3 * current_row + 3);
			}
		}

		row_result[current_row] = ks_result;
		current_row = (current_row + 1) % 4;
		ks_current_row = rows[current_row];
		ks_continue();
	}
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == &hi2c1 && ks_state) {
		ks_continue();
	}
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
	if (hi2c == &hi2c1 && ks_state) {
		ks_continue();
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM6) {
		KB_Test();
	}
}

void UpdateButtonState() {
	static uint32_t last_change_tick = 0;
	static uint32_t last_tick = 0;
	static uint8_t state = 0;

	btn_state_prev = btn_state;

	uint32_t curr_tick = HAL_GetTick();
	if (curr_tick <= last_tick)
		return;
	last_tick = curr_tick;

	state += HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_15);

	if (curr_tick >= last_change_tick + 20) {
		last_change_tick = curr_tick;
		btn_state = state >= 10;
		state = 0;
	}
}

uint32_t const megalovania_melody[] = { N_D3, N_D3, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_C3, N_C3, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_B2, N_B2, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_AS2, N_AS2, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_D3, N_D3, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_C3, N_C3, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_B2, N_B2, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_AS2, N_AS2, N_D4, N_A3, 0, N_GS3, N_G3,
N_F3, N_D3, N_F3, N_G3, N_D4, N_D4, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_C4, N_C4, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_B3, N_B3, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_AS3, N_AS3, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_D4, N_D4, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_C4, N_C4, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_B3, N_B3, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_AS3, N_AS3, N_D5, N_A4, 0, N_GS4, N_G4,
N_F4, N_D4, N_F4, N_G4, N_F4, N_F4, N_F4, N_F4, N_F4, N_D4, N_D4,
N_D4, N_F4, N_F4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_F4, N_G4, 0, N_F4,
N_F4, N_F4, N_G4, N_GS4, N_A4, N_C5, N_A4, N_D5, N_D5,
N_D5, N_A4, N_D5, N_C5, N_F4, N_F4, N_F4, N_F4, N_F4, N_D4, N_D4,
N_D4, N_F4, N_F4, N_F4, N_F4, N_D4, N_F4, N_E4, N_D4, N_C4, 0, N_G4,
N_E4, N_D4, N_D4, N_D4, N_D4, N_F3, N_G3, N_AS3, N_C4, N_D4, N_F4,
N_C5, 0, N_F4, N_D4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_GS4,
N_G4, N_F4, N_D4, N_F4, N_F4, N_F4, N_GS4, N_A4, N_C5, N_A4, N_GS4,
N_G4, N_F4, N_D4, N_E4, N_F4, N_G4, N_A4, N_C5, N_CS5, N_GS4, N_GS4,
N_G4, N_F4, N_G4, N_F3, N_G3, N_A3, N_F4, N_E4, N_D4, N_E4, N_F4,
N_G4, N_E4, N_A4, N_A4, N_G4, N_F4, N_DS4, N_CS4, N_DS4, 0, N_F4,
N_D4, N_F4, N_G4, N_GS4, N_G4, N_F4, N_D4, N_GS4, N_G4, N_F4, N_D4,
N_F4, N_F4, N_F4, N_GS4, N_A4, N_C5, N_A4, N_GS4, N_G4, N_F4, N_D4,
N_E4, N_F4, N_G4, N_A4, N_C5, N_CS5, N_GS4, N_GS4, N_G4, N_F4, N_G4,
N_F3, N_G3, N_A3, N_F4, N_E4, N_D4, N_E4, N_F4, N_G4, N_E4, N_A4,
N_A4, N_G4, N_F4, N_DS4, N_CS4, N_DS4, };
uint32_t const megalovania_delays[] = { 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16,
		16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16,
		16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8,
		8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32,
		8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8,
		6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16,
		16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16,
		16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8,
		16, 16, 16, 16, 16, 8, 6, 32, 8, 8, 8, 16, 16, 16, 16, 16, 8, 6, 32, 8,
		8, 8, 16, 16, 16, 8, 16, 8, 8, 8, 8, 4, 16, 8, 16, 8, 8, 8, 16, 16, 16,
		16, 16, 8, 8, 16, 8, 8, 8, 8, 8, 8, 8, 8, 16, 16, 16, 2, 8, 16, 8, 8, 8,
		8, 4, 16, 8, 16, 8, 8, 8, 8, 8, 16, 8, 16, 8, 8, 8, 8, 8, 8, 8, 16, 8,
		15, 8, 8, 2, 3, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8, 2,
		16, 8, 16, 8, 16, 16, 16, 16, 16, 16, 8, 8, 8, 8, 8, 8, 16, 16, 16, 2,
		8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 2, 8, 8, 8, 8, 2, 2, 3, 16, 16, 16, 16,
		16, 16, 16, 16, 16, 16, 16, 16, 8, 2, 16, 8, 16, 8, 16, 16, 16, 16, 16,
		16, 8, 8, 8, 8, 8, 8, 16, 16, 16, 2, 8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 2, 8,
		8, 8, 8, 2, 1 };
uint32_t const zelda_melody[] = { N_AS4, 0, 0, N_AS4, N_AS4, N_AS4, N_AS4,
		N_AS4, 0,
		N_GS4, N_AS4, 0, 0, N_AS4, N_AS4, N_AS4, N_AS4, N_AS4, 0, N_GS4,
		N_AS4, 0, 0, N_AS4, N_AS4, N_AS4, N_AS4, N_AS4, N_F3, N_F3, N_F3,
		N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_AS4, N_F3, N_F3, 0,
		N_AS4, N_AS4, N_C5, N_D5, N_DS5, N_F5, 0, N_F5, N_F5, N_FS5, N_GS5,
		N_AS5, 0, N_AS5, N_AS5, N_AS5, N_GS5, N_FS5, N_GS5, 0, N_FS5, N_F5,
		N_F5, N_DS5, N_DS5, N_F5, N_FS5, N_F5, N_DS5, N_CS5, N_CS5, N_DS5,
		N_F5, N_DS5, N_CS5, N_C5, N_C5, N_D5, N_E5, N_G5, N_F5, N_F3, N_F3,
		N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_AS4, N_F3, N_F3, 0,
		N_AS4, N_AS4, N_C5, N_D5, N_DS5, N_F5, 0, N_F5, N_F5, N_FS5,
		N_GS5, N_AS5, 0, N_CS6, N_C6, N_A5, 0, N_F5, N_FS5, 0, N_AS5, N_A5,
		N_F5, 0, N_F5, N_FS5, 0, N_AS5, N_A5, N_F5, 0, N_D5, N_DS5, 0,
		N_FS5, N_F5, N_CS5, 0, N_AS4, N_C5, N_C5, N_D5, N_E5, 0, N_G5, N_F5,
		N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_AS4,
		N_F3, N_F3, 0, N_AS4, N_AS4, N_C5, N_D5, N_DS5, N_F5, 0, N_F5, N_F5,
		N_FS5, N_GS5, N_AS5, 0, N_AS5, N_AS5, N_AS5, N_GS5, N_FS5, N_GS5, 0,
		N_FS5, N_F5, N_F5, N_DS5, N_DS5, N_F5, N_FS5, N_F5, N_DS5, N_CS5,
		N_CS5, N_DS5, N_F5, N_DS5, N_CS5, N_C5, N_C5, N_D5, N_E5, N_G5,
		N_F5, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3,
		N_AS4, N_F3, N_F3, 0, N_AS4, N_AS4, N_C5, N_D5, N_DS5, N_F5, 0,
		N_F5, N_F5, N_FS5, N_GS5, N_AS5, 0, N_CS6, N_C6, N_A5, 0, N_F5,
		N_FS5, 0, N_AS5, N_A5, N_F5, 0, N_F5, N_FS5, 0, N_AS5, N_A5, N_F5, 0,
		N_D5, N_DS5, 0, N_FS5, N_F5, N_CS5, 0, N_AS4, N_C5, N_C5, N_D5,
		N_E5, 0, N_G5, N_F5, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3, N_F3,
		N_F3, N_F3 };
uint32_t const zelda_delays[] = { 2, 8, 8, 8, 8, 8, 8, 6, 16, 16, 4, 8, 8, 8, 8,
		8, 8, 6, 16, 16, 4, 8, 8, 8, 8, 8, 8, 8, 16, 16, 8, 16, 16, 8, 16, 16,
		8, 8, 4, 4, 6, 16, 16, 16, 16, 16, 16, 2, 8, 8, 8, 8, 8, 2, 8, 8, 8, 8,
		8, 8, 6, 16, 16, 2, 4, 8, 16, 16, 2, 8, 8, 8, 16, 16, 2, 8, 8, 8, 16,
		16, 2, 4, 8, 16, 16, 8, 16, 16, 8, 16, 16, 8, 8, 4, 4, 6, 16, 16, 16,
		16, 16, 16, 2, 8, 8, 8, 8, 8, 2, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4,
		2, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 8, 16, 16, 4, 4, 4, 8, 16, 16,
		8, 16, 16, 8, 16, 16, 8, 8, 4, 4, 6, 16, 16, 16, 16, 16, 16, 2, 8, 8, 8,
		8, 8, 2, 8, 8, 8, 8, 8, 8, 6, 16, 16, 2, 4, 8, 16, 16, 2, 8, 8, 8, 16,
		16, 2, 8, 8, 8, 16, 16, 2, 4, 8, 16, 16, 8, 16, 16, 8, 16, 16, 8, 8, 4,
		4, 6, 16, 16, 16, 16, 16, 16, 2, 8, 8, 8, 8, 8, 2, 4, 4, 4, 4, 4, 4, 2,
		4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 8, 16, 16,
		4, 4, 4, 8, 16, 16, 8, 16, 16, 8, 16, 16, 8, 8 };

#define USER_MELODY_LENGTH 128
uint32_t user_melody[USER_MELODY_LENGTH];
uint32_t user_delays[USER_MELODY_LENGTH];

uint32_t const *const melodies[] = { megalovania_melody, zelda_melody, NULL,
		NULL, user_melody };
uint32_t const *const delays[] = { megalovania_delays, zelda_delays, NULL, NULL,
		user_delays };
size_t lengths[] = { sizeof(megalovania_melody) / sizeof(megalovania_melody[0]),
		sizeof(zelda_melody) / sizeof(zelda_melody[0]), 0, 0,
		USER_MELODY_LENGTH };
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */
	for (size_t i = 0; i < USER_MELODY_LENGTH; ++i) {
		user_melody[i] = 0;
		user_delays[i] = 4;
	}
	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */
	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART6_UART_Init();
	MX_I2C1_Init();
	MX_TIM6_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */

	Buzzer_Init();
	HAL_TIM_Base_Start_IT(&htim6);

	int is_test_mode = 1;
	enum MainState state = MS_IDLE;
	int curr_melody;
	size_t melody_pos;
	uint32_t melody_ts;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	for (;;) {
		UpdateButtonState();
		if (btn_state && !btn_state_prev) {
			is_test_mode ^= 1;
			snprintf(print_buf, sizeof(print_buf), "%s\r\n", is_test_mode ? "Testing mode!" : "Music mode!");
			UART_Transmit((uint8_t*) print_buf);
		}
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, is_test_mode);
		if (is_test_mode) {
			Buzzer_Set_Volume(BUZZER_VOLUME_MUTE);
			int key = ReadBuffer();
			if (key >= 0) {
				snprintf(print_buf, sizeof(print_buf), "Keycode = %d\r\n", key);
				UART_Transmit((uint8_t*) print_buf);
			}
		} else {
			int key = ReadBuffer();
			if (key >= 0) {
				snprintf(print_buf, sizeof(print_buf), "Music mode!\r\n");
				UART_Transmit((uint8_t*) print_buf);
			}

			switch (key) {
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
				melody_pos = 0;
				melody_ts = HAL_GetTick();
				curr_melody = key - 1;
				state = MS_PLAY;
				break;
			case 6:
				melody_pos = 0;
				curr_melody = 4;
				lengths[curr_melody] = 0;
				state = MS_EDIT;
				break;
			case 7:
				state = MS_IDLE;
				break;
			case 8:
				for (size_t i = 0; i < lengths[curr_melody]; ++i) {
					snprintf(print_buf, sizeof(print_buf), "%d ",
							melodies[curr_melody][i]);
					UART_Transmit((uint8_t*) print_buf);
				}
			case 9:
				user_melody[lengths[curr_melody]++] = N_D3;
				break;
			case 10:
				user_melody[lengths[curr_melody]++] = N_D4;
				break;
			case 11:
				user_melody[lengths[curr_melody]++] = N_A3;
				break;
			case 12:
				user_melody[lengths[curr_melody]++] = N_GS3;
				break;
			}

			if (key == 9 || key == 10 | key == 11 || key == 12) {
				snprintf(print_buf, sizeof(print_buf), "New note: %d\r\n", user_melody[lengths[curr_melody] - 1]);
				UART_Transmit((uint8_t*) print_buf);
			}

			uint32_t note, delay;
			uint32_t tick = HAL_GetTick();
			switch (state) {
			case MS_IDLE:
			case MS_EDIT:
				Buzzer_Set_Volume(BUZZER_VOLUME_MUTE);
				break;
			case MS_PLAY:
				if (melody_pos < 0 || melody_pos >= lengths[curr_melody]) {
					state = MS_IDLE;
					break;
				}
				note = melodies[curr_melody][melody_pos];
				delay = delays[curr_melody][melody_pos];
				if (tick > melody_ts + 1920 / delay + 10) {
					melody_ts += 1920 / delay;
					++melody_pos;
				} else if (tick > melody_ts + 1920 / delay) {
					Buzzer_Set_Volume(BUZZER_VOLUME_MUTE);
				} else {
					if (note) {
						Buzzer_Set_Freq(note);
						Buzzer_Set_Volume(BUZZER_VOLUME_MAX);
					} else {
						Buzzer_Set_Volume(BUZZER_VOLUME_MUTE);
					}
				}
				break;
			}
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
