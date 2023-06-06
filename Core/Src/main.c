/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "ir.h"
#include "ws2812b.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
	if (ch == '\n')
		__io_putchar('\r');

	HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);

	return 1;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim2)
	{
		switch (HAL_TIM_GetActiveChannel(&htim2))
		{
		case HAL_TIM_ACTIVE_CHANNEL_1:
			ir_tim_interrupt();
			break;
		default:
			break;
		}
	}
}

// opoznienie likwidujace problem wielokrotnego kliku tego samego przycisku
#define NEXT_CLICK_DELAY	10

// struktura do przchowywania kolorow diody led (numer leda i kolory)
typedef struct
{
	uint32_t no;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	bool power;
} led_t;

// zmiana jasnosci koloru
#define BRIGHTNESS_LIMIT_UP		50
#define BRIGHTNESS_LIMIT_DOWN	0
#define BRIGHTNESS_STEP			2
void change_brightness(led_t* led, int color_number, bool dir)
{
	uint8_t* color;
	switch (color_number)
	{
	case 1:
		color = &led->r;
		break;
	case 2:
		color = &led->g;
		break;
	case 3:
		color = &led->b;
		break;
	default:
		break;
	}

	if (dir)
	{
		if (*color < BRIGHTNESS_LIMIT_UP)
			*color += BRIGHTNESS_STEP;
		else
			*color = BRIGHTNESS_LIMIT_UP;
	}
	else
	{
		if (*color > BRIGHTNESS_LIMIT_DOWN)
			*color -= BRIGHTNESS_STEP;
		else
			*color = BRIGHTNESS_LIMIT_DOWN;
	}
}

// wylaczanie ledow
void toggle_led(led_t* led)
{
	if (led->power) ws2812b_set_color(led->no, 0, 0, 0);
	else 			ws2812b_set_color(led->no, led->r, led->g, led->b);

	led->power = !led->power;
	ws2812b_update();
}

// inicjalizacja oraz reset ledow
void led_reset(led_t led[])
{
	for (int i=0; i<7; i++)
	{
		led[i].no = i;
		led[i].r = 2;
		led[i].g = 2;
		led[i].b = 2;
		led[i].power = true;
		ws2812b_set_color(led[i].no, led[i].r, led[i].g, led[i].b);
	}
	ws2812b_update();
}

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

// tryby automatyczne
int limit = 80;
int speed = 1;

void color_patern(int* i, bool* dir)
{
	if (*dir)
	{
		if (*i < limit) *i+=speed;
		else
		{
			*i = limit;
			*dir = false;
		}
	}
	else
	{
		if (*i > 0) *i-=speed;
		else
		{
			*i = 0;
			*dir = true;
		}
	}
}

// tryb automatyczny nr 1
void automatic_mode1(void)
{
	static bool dir = true;				// zmienne do stopniowej zmiany barwy ledow
	static int i = 0;					//

	ws2812b_set_color(0, gamma8[i], 			gamma8[i], 			gamma8[i]);
	ws2812b_set_color(1, gamma8[i], 			gamma8[i], 			gamma8[limit-i]);
	ws2812b_set_color(2, gamma8[i], 			gamma8[limit-i], 	gamma8[i]);
	ws2812b_set_color(3, gamma8[i], 			gamma8[limit-i], 	gamma8[limit-i]);
	ws2812b_set_color(4, gamma8[limit-i], 		gamma8[i], 			gamma8[i]);
	ws2812b_set_color(5, gamma8[limit-i], 		gamma8[i], 			gamma8[limit-i]);
	ws2812b_set_color(6, gamma8[limit-i], 		gamma8[limit-i], 	gamma8[i]);
	ws2812b_update();
	HAL_Delay(5);

	color_patern(&i, &dir);
}

//tryb automatyczny nr 2
void automatic_mode2(void)
{
	static bool dir_i=true, dir_j=true, dir_k=true;				// zmienne do stopniowej zmiany barwy ledow
	static int i=0, j=30, k=60;									//

	for (int m=0; m<7; m++)
		ws2812b_set_color(m, gamma8[i], gamma8[j], gamma8[k]);
	ws2812b_update();
	HAL_Delay(5);

	color_patern(&i, &dir_i);
	color_patern(&j, &dir_j);
	color_patern(&k, &dir_k);
}

//tryb automatyczny nr 3
void automatic_mode3(void)
{
	static int led_pos = 0;
	static bool dir = true;
	static uint8_t color_r = 0, color_g = 10, color_b = 20;

	ws2812b_set_color(led_pos, gamma8[color_r], gamma8[color_g], gamma8[color_b]);
	ws2812b_update();

	if (dir)
	{
		if (led_pos < 6) led_pos++;
		else
		{
			dir = !dir;
			color_r = rand()%limit;
			color_g = rand()%limit;
			color_b = rand()%limit;
		}
	}
	else
	{
		if (led_pos > 0) led_pos--;
		else
		{
			dir = !dir;
			color_r = rand()%limit;
			color_g = rand()%limit;
			color_b = rand()%limit;
		}
	}

	HAL_Delay(100);
}

//tryb automatyczny nr 4
void automatic_mode4(void)
{
	static int led_pos = 0;
	static int positions[7] = {0}, positions_left = 7;
	static uint8_t color_r = 0, color_g = 10, color_b = 20;

	ws2812b_set_color(led_pos, gamma8[color_r], gamma8[color_g], gamma8[color_b]);
	ws2812b_update();

	while(1)
	{
		led_pos = rand()%7;
		if (positions[led_pos] == 0)
		{
			positions[led_pos] = 1;
			positions_left--;
			break;
		}
		else if (positions_left == 0)
		{
			memset(positions, 0, sizeof(positions));
			positions_left = 7;
			color_r = rand()%limit;
			color_g = rand()%limit;
			color_b = rand()%limit;
			break;
		}
	}

	HAL_Delay(100);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // inicjalizacja TIM2 jako timera mierzacego czas impulsow wyjsciowych z odbiornika IR zarowno
  // dodatnich i ujemnych
  ir_init();
  ws2812b_init();

  uint32_t last_time = HAL_GetTick();	// do odmierzania interwalu 1000ms w wysylaniu danych do konsoli po USART2
  uint32_t active_led_no = 0;			// numer wybranej diody (0...6)
  int active_color = 1;					// edytowany kolor (1-red, 2-green, 3-blue)
  int mode = 1;							// tryby pracy (1-manual pojedyncza dioda, 2-manual wszystkie diody, 3..5-automat)

  led_t led[7];
  led_reset(led);

  while (1)
  {
	  int value = ir_read();

	  switch (value)	// kod wspolny dla wszystkich trybow sterowania
	  {
	  case IR_CODE_7:
		  active_color = 1;
		  break;
	  case IR_CODE_8:
		  active_color = 2;
		  break;
	  case IR_CODE_9:
		  active_color = 3;
		  break;
	  case IR_CODE_ONOFF:
		  if (mode == 1)
			  toggle_led(&led[active_led_no]);
		  else
		  {
			  for (int i=0; i<7; i++)
			  {
				  toggle_led(&led[i]);
			  }
		  }
		  HAL_Delay(NEXT_CLICK_DELAY);
		  break;
  	  case IR_CODE_MENU:
  		  active_led_no = 0;
  		  active_color = 1;
  		  if (mode < 6) mode++;
  		  else mode = 1;
  		  led_reset(led);
  		  HAL_Delay(NEXT_CLICK_DELAY);
  		  break;
	  }

	  if (mode == 1)	// tryb sterowania pojedyncza dioda led
	  {
		  switch (value)
		  	  {
		  	  case IR_CODE_0:
		  		  active_led_no = 0;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_1:
		  		  active_led_no = 1;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_2:
		  		  active_led_no = 2;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_3:
		  		  active_led_no = 3;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_4:
		  		  active_led_no = 4;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_5:
		  		  active_led_no = 5;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_6:
		  		  active_led_no = 6;
		  		  active_color = 1;
		  		  break;
		  	  case IR_CODE_REWIND:
		  		  if (active_led_no > 0) active_led_no--;
		  		  else active_led_no = 6;
		  		  HAL_Delay(NEXT_CLICK_DELAY);
		  		  break;
		  	  case IR_CODE_FORWARD:
		  		  if (active_led_no < 6) active_led_no++;
				  else active_led_no = 0;
		  		  HAL_Delay(NEXT_CLICK_DELAY);
		  		  break;
		  	  case IR_CODE_PLUS:
		  		  change_brightness(&led[active_led_no], active_color, true);
		  		  ws2812b_set_color(led[active_led_no].no, led[active_led_no].r, led[active_led_no].g, led[active_led_no].b);
		  		  ws2812b_update();
		  		  break;
		  	  case IR_CODE_MINUS:
		  		  change_brightness(&led[active_led_no], active_color, false);
		  		  ws2812b_set_color(led[active_led_no].no, led[active_led_no].r, led[active_led_no].g, led[active_led_no].b);
		  		  ws2812b_update();
		  		  break;
		  	  }
	  }
	  else if (mode == 2)	// tryb sterowania wszystkimi diodami led
	  {
		  switch (value)
			  {
			  case IR_CODE_PLUS:
				  for (int i=0; i<7; i++)
				  {
					  change_brightness(&led[i], active_color, true);
					  ws2812b_set_color(led[i].no, led[i].r, led[i].g, led[i].b);
				  }
				  ws2812b_update();
				  break;
			  case IR_CODE_MINUS:
				  for (int i=0; i<7; i++)
				  {
					  change_brightness(&led[i], active_color, false);
					  ws2812b_set_color(led[i].no, led[i].r, led[i].g, led[i].b);
				  }
				  ws2812b_update();
				  break;
			  }
	  }
	  else if (mode == 3)				// tryb automatyczny nr 1
	  {
		  if (led[0].power) automatic_mode1();
	  }
	  else if (mode == 4)				// tryb automatyczny nr 2
	  {
		  if (led[0].power) automatic_mode2();
	  }
	  else if (mode == 5)				// tryb automatyczny nr 3
	  {
		  if (led[0].power) automatic_mode3();
	  }
	  else								// tryb automatyczny nr 4
	  {
		  if (led[0].power) automatic_mode4();
	  }

	  // pomoc przy debugowaniu - wysylanie na UART
	  if (HAL_GetTick() - last_time > 1000)
	  {
		  printf("Aktywna dioda: %lu, aktywny kolor: %d\n", active_led_no, active_color);
		  for (int i=0; i<7; i++)
		  {
			  printf("LED %d\tR: %d\tG: %d\tB: %d\t\n", i, led[i].r, led[i].g, led[i].b);
		  }
		  printf("\n");
		  last_time = HAL_GetTick();
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
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
