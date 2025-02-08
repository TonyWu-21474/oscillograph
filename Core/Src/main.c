/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_BUFFER_SIZE 256
volatile uint32_t adcBuffer[ADC_BUFFER_SIZE]; // DMA数据缓冲区,每半缓冲区绘制一次图
#define SAMPLE_RATE 500    // 如需 1000Hz，请改为 1000

#if SAMPLE_RATE == 500
  #define TIM_PERIOD (2000 - 1)  // 2ms 周期（假设定时器时钟为 1MHz）
#elif SAMPLE_RATE == 1000
  #define TIM_PERIOD (1000 - 1)  // 1ms 周期
#else
  #error "采样率仅支持 500 或 1000Hz"
#endif
#define ADC_MAX_VALUE 4095
#define OLED_WIDTH    128
#define OLED_HEIGHT   64

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
void ProcessADCData(uint32_t* data, uint32_t length);
void DisplayHalfBuffer(uint32_t *halfBuffer, uint32_t length);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief DMA Half Transfer 完成回调函数.
  * 当 DMA 将数据传输至 adcBuffer 一半时调用
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* 处理缓冲区前半部分的数据 */
	DisplayHalfBuffer((uint32_t*)&adcBuffer[0],ADC_BUFFER_SIZE / 2);
  ProcessADCData((uint32_t*)&adcBuffer[0], ADC_BUFFER_SIZE / 2);
}

/**
  * @brief DMA Transfer Complete 完成回调函数.
  * 当 DMA 将数据传输完 adcBuffer 时调用
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* 处理缓冲区后半部分的数据 */
	DisplayHalfBuffer((uint32_t*)&adcBuffer[ADC_BUFFER_SIZE / 2],ADC_BUFFER_SIZE / 2);
  ProcessADCData((uint32_t*)&adcBuffer[ADC_BUFFER_SIZE / 2], ADC_BUFFER_SIZE / 2);
}
/**
  * @brief 处理采集的 ADC 数据
  * @param data: 处理数据的起始地址
  * @param length: 数据的长度
  */
void ProcessADCData(uint32_t* data, uint32_t length)
{
  uint32_t sum = 0;
  for (uint32_t i = 0; i < length; i++)
  {
    sum += data[i];
  }
  uint32_t avg = sum / length;
  // 后续处理：例如通过UART发送、存入全局变量等
}
void DisplayHalfBuffer(uint32_t *halfBuffer, uint32_t length)//传入任意长度和起始位置均可
{
    uint32_t x, y;
    uint32_t samplesToDisplay = (length < OLED_WIDTH) ? length : OLED_WIDTH;

    // 如有需要，首先清除显示（例如 OLED_Clear()）
    // OLED_Clear();

    for (x = 0; x < samplesToDisplay; x++)
    {
        /* 映射 ADC 数据到 y 坐标：
         * scaled_y 计算为 ADC 数据按比例缩放到 0 ～ (OLED_HEIGHT-1) 范围内，
         * 随后反转 y 坐标（OLED 顶部为0，底部为 OLED_HEIGHT-1）
         */
        y = (halfBuffer[x] * (OLED_HEIGHT - 1)) / ADC_MAX_VALUE;
        y = (OLED_HEIGHT - 1) - y;

        OLED_DrawPixel(x, y);
    }

    // 如有屏幕刷新函数，可在此调用（例如 OLED_UpdateScreen()）
    // OLED_UpdateScreen();
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM2)
  {
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
  }
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
	htim2.Init.Period = TIM_PERIOD;
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
	if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }//启动TIM2
	HAL_TIM_Base_Start_IT(&htim2);
	/* 启动 ADC 的 DMA 模式，在循环模式下自动将转换结果传送到 adcBuffer */
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcBuffer, ADC_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }
	//暂时考虑使用u8g2库完成绘图 1/28:u8g2有部分存在问题，暂时弃用 2/8:移除u8g2
	OLED_Init();
	OLED_ON();
	OLED_CLS();
	OLED_ShowStr(20,3,"hello world",1);//这一部分代码没有问题，另外：不要在注释里打//
	//OLED_DrawPixel(0,0);
//	for(int i =0;i<128;i++)
//	{
//		OLED_DrawPixel(i,i/2);
//		OLED_DrawPixel(i,63-i/2);
//	}
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
//		HAL_Delay(250);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 256;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
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
