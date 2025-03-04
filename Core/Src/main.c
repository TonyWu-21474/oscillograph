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
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UPDATE_RATE 10000UL  // 采样率 10kHz
// 全局变量：相位累加器和正弦波频率（单位 Hz）
volatile double phase = 0.0;
volatile int counter = 0;
volatile double sineFrequency = 1000.0;  // 正弦波频率，初始为 1 kHz
const double timeStep = 1.0 / UPDATE_RATE;  // 每个采样点的时间间隔（秒）
#define ADC_BUFFER_SIZE 128
#define SINE_TABLE_SIZE 128
volatile uint16_t adcBuffer[ADC_BUFFER_SIZE]; // DMA数据缓冲区,每半缓冲区绘制一次图
volatile uint8_t table[SINE_TABLE_SIZE]={0};//正弦表
volatile uint16_t adcValue = 0;
volatile int flag_halfbuf = 0;
volatile int flag_fullbuf = 0;	//半满/全满标志位
#define SAMPLE_RATE 1000    // 如需 1000Hz，请改为 1000

#if SAMPLE_RATE == 500		//别太高
  #define TIM_PERIOD (2000 - 1)  // 2ms 周期（假设定时器时钟为 1MHz）现在配置为10MHz
#elif SAMPLE_RATE == 1000
  #define TIM_PERIOD (1000 - 1)  // 1ms 周期
#else
  #error "采样率仅支持 500 或 1000Hz"
#endif
#define ADC_MAX_VALUE 4095
#define OLED_WIDTH    128
#define OLED_HEIGHT   64
#define M_PI 3.14159
#define GRID_TOP       8
#define GRID_BOTTOM    63
#define GRID_CELL_SIZE 8   //绘制网格
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
float ProcessADCData(volatile uint16_t* data, uint32_t length);
void DisplayHalfBuffer(volatile uint16_t *halfBuffer, uint32_t length);
float CalculateFrequency(volatile uint16_t *adcBuffer, size_t buf_size, float mean);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief DMA Half Transfer 完成回调函数.
  * 当 DMA 将数据传输至 adcBuffer 一半时调用
  */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	flag_halfbuf = 1;
}

/**
  * @brief DMA Transfer Complete 完成回调函数.
  * 当 DMA 将数据传输完 adcBuffer 时调用
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	flag_fullbuf = 1;
}
/**
  * @brief 处理采集的 ADC 数据
  * @param data: 处理数据的起始地址
  * @param length: 数据的长度
  */
float ProcessADCData(volatile uint16_t* data, uint32_t length)
{
  uint32_t sum = 0;
  for (uint32_t i = 0; i < length; i++)
  {
    sum += data[i];
//		OLED_ShowNum(0,0,data[i],4,2);
////		HAL_Delay(100);
//		OLED_ShowStr(0,0,"    ",1);
  }
  float avg = sum / length;
//	OLED_ShowStr(0,0,"     ",1);
//	OLED_ShowStr(55,0,"            ",1);
	//
  // 后续处理：例如通过UART发送、存入全局变量等
	return avg;
}

void DisplayHalfBuffer(volatile uint16_t *halfBuffer, uint32_t length)//传入任意长度和起始位置均可
{
//		uint8_t x, y;
//    uint32_t samplesPerPixel;

//    // 根据输入长度决定每列对应的数据点个数
//    if (length < OLED_WIDTH)
//    {
//        samplesPerPixel = 1;
//    }
//    else
//    {
//        // 当长度为 OLED_WIDTH 的倍数时，每列显示 length/OLED_WIDTH 个数据
//        samplesPerPixel = length / OLED_WIDTH;
//    }

//    // 如有需要，首先清除显示区域和 OLED 缓冲区
//    ClearRectangleStr();
////		OLED_CLS();
//    OLED_ClearBuffer();

//    
//    // =====================================================
//    // 抽样显示数据
//    // =====================================================
//    for (x = 0; x < OLED_WIDTH; x++)
//    {
//        // 计算本列样本在缓冲区中的索引
//        uint32_t index = x * samplesPerPixel;
//        if (index >= length)
//        {
//            index = length - 1;
//        }

//        // 将 ADC 数据映射到 y 坐标：
//        // 1. 首先将 ADC 数据按比例缩放到 0 ~ (OLED_HEIGHT - 1 - 8) 范围内（预留顶部 8 行）
//        // 2. 然后 y 坐标翻转：OLED 顶部为 8 像素，底部为 OLED_HEIGHT-1
//        y = (halfBuffer[index] * (OLED_HEIGHT - 1 - 8)) / ADC_MAX_VALUE;
//        //y = (OLED_HEIGHT - 1) - y;
//        
//        // 绘制该采样对应的点
//        OLED_DrawWave(x, y); //使用这个方法不用反转坐标
//    }
		uint8_t x, y;

    // 清除显示区域和 OLED 缓冲区
    ClearRectangleStr();
    OLED_ClearBuffer();

    if (length == 0)
    {
        return; // 处理长度为0的情况，避免除以零
    }

    // 遍历每一列
    for (x = 0; x < OLED_WIDTH; x++)
    {
        // 计算当前列对应的数据点索引（线性插值）
        uint32_t index = ( (uint32_t)x * (length - 1) ) / (OLED_WIDTH - 1);

        // 确保索引不越界
        if (index >= length)
        {
            index = length - 1;
        }

        // 将 ADC 值映射到 Y 轴坐标（预留顶部8像素）
        y = (halfBuffer[index] * (OLED_HEIGHT - 1 - 8)) / ADC_MAX_VALUE;

        // 绘制波形点
        OLED_DrawWave(x, y);
    }
		// =====================================================
    // 绘制正方形网格
    // =====================================================

    // 绘制一条水平网格线（例如在 y = 35 像素处）
    for (x = 0; x < OLED_WIDTH; x++)
    {
        OLED_DrawPixel(x, 35);
    }

    // 绘制其他水平网格线，每 GRID_CELL_SIZE 个像素绘制 1 条
    for (y = GRID_TOP; y <= GRID_BOTTOM; y += GRID_CELL_SIZE)
    {
        for (x = 0; x < OLED_WIDTH; x++)
        {
            if (x % 8 == 0)
            {
                OLED_DrawPixel(x, y);
            }
        }
    }

    // 绘制垂直网格线，每 GRID_CELL_SIZE 个像素绘制
    for (x = 0; x < OLED_WIDTH; x += GRID_CELL_SIZE)
    {
        // 此处例如只对 x 为 32 的倍数的列绘制完整的垂直线
        if (x % 32 == 0)
        {
            for (y = GRID_TOP; y <= GRID_BOTTOM; y++)
            {
                OLED_DrawPixel(x, y);
            }
        }
    }

		// 如有屏幕刷新函数，可在此调用（例如 OLED_UpdateScreen() 或 OLED_UpdatePage()）
    // 例如：OLED_UpdatePage();

    // 以下为测试用：切换指示灯状态
    //HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);  // 参数根据实际情况填写
}
void Write_DAC(uint8_t value) {
    // 假定使用 GPIOB 的低8位（即 PB0 ~ PB7）这一部分需要重写
    // 先清除低8位，再写入 value
    //GPIOB->ODR = (GPIOB->ODR & 0xFF00) | value;
	// 输出位 0 到 GPIOA, PA1
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, (value & (1 << 0)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 1 到 GPIOA, PA2
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, (value & (1 << 1)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 2 到 GPIOA, PA3
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, (value & (1 << 2)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 3 到 GPIOA, PA4
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, (value & (1 << 3)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 4 到 GPIOA, PA5
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, (value & (1 << 4)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 5 到 GPIOA, PA6
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, (value & (1 << 5)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 6 到 GPIOA, PA7
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, (value & (1 << 6)) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // 输出位 7 到 GPIOB, PB0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (value & (1 << 7)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM2)
  {	
		
  }
	else if (htim->Instance == TIM3) {
//        //清除中断标志位
//        
//				//HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
//        // 计算当前正弦值
//        double sineValue = sin(phase);  // 范围为 -1 到 +1
//        // 映射到 0～255
//        uint8_t dacValue = (uint8_t)(((sineValue + 1.0) / 2.0) * 255.0);

//        // 输出到 DAC（GPIO）
//        Write_DAC(dacValue);

//        // 更新相位：每个采样点相位增量 = 2π * sineFrequency * timeStep
//        phase += 2.0 * M_PI * sineFrequency * timeStep;
//        if (phase >= 2.0 * M_PI) {
//            phase -= 2.0 * M_PI;
//        }
//			}
					// 根据当前 phase 计算查找表的索引
        // phase范围：[0,2π), 对应查找表索引：[0, SINE_TABLE_SIZE-1]
        uint16_t tableIndex = (uint16_t)(phase * SINE_TABLE_SIZE / (2.0 * M_PI));

        // 保险检查：确保索引在合理范围内
        if (tableIndex >= SINE_TABLE_SIZE)
        {
            tableIndex = 0;
        }

        // 从查找表中读取预计算的正弦值（已映射至 0-255）
        uint8_t dacValue = table[tableIndex];

        // 输出到 DAC
        Write_DAC(dacValue);

        // 更新相位：每个采样周期内增加的相位增量为 2π * sineFrequency * timeStep
        phase += 2.0 * M_PI * sineFrequency * timeStep;
        if (phase >= 2.0 * M_PI)
        {
            phase -= 2.0 * M_PI;
        }
			}
}
/**
 * @brief 绘制正弦波形的测试函数
 *
 * 该函数在 OLED 显示屏上绘制一个完整周期的正弦波形：
 * - 振幅取 OLED_HEIGHT/4
 * - 波形中心线位于屏幕中间
 * - 水平方向上正弦波运动一个完整周期
 */
void DrawSineWave(void)
{
    int x, y;
    double amplitude = (double)OLED_HEIGHT / 2.0;  // 振幅
    double center = (double)OLED_HEIGHT / 2.0;       // 中心线

    for (x = 0; x < OLED_WIDTH; x++)
    {
        // angle 计算：使得 x 遍历时正弦波运动一个完整周期
        double angle = ((double)x / OLED_WIDTH) * 8 * M_PI;
        // 根据正弦公式计算 y 坐标，并加上中心偏移
        y = (int)(center + amplitude * sin(angle));
        // 绘制像素
        OLED_DrawWave(x, y);
				//HAL_Delay(100);
    }
}


float CalculateFrequency(volatile uint16_t *temp_buf, size_t buf_size, float mean)
{
    uint32_t firstTimestamp = 0;      // 第一次过零时的时间戳
    uint32_t previousTimestamp = 0;   // 上一次过零的时间戳
    uint32_t cumulativeInterval = 0;  // 累计相邻两个过零之间的时间间隔(ms)
    uint16_t crossingCount = 0;       // 过零点计数
    // 遍历 ADC 数据缓冲区，寻找过零点（从低于均值跃升到大于等于均值）
    for (size_t i = 1; i < buf_size; i++)
    {
        if ((temp_buf[i - 1] < mean) && (temp_buf[i] >= mean ))
        {
            // 当检测到过零点时，马上通过 HAL 库获取当前内核时间戳（单位 ms）
            uint32_t currentTimestamp = i;

            if (crossingCount == 0)
            {
                // 第一次检测到过零点，记录时间戳
                firstTimestamp = currentTimestamp;
            }
            else
            {
                // 累计相邻两个过零之间的时间差
                cumulativeInterval += (currentTimestamp - previousTimestamp);
            }

            // 更新上一个过零点的时间戳，并计数
            previousTimestamp = currentTimestamp;
            crossingCount++;
        }
    }

    float frequency = 0.0f;

    // 至少需要 2 个过零点来计算周期（即至少一个完整周期的采样间隔）
    if (crossingCount > 1)
    {
        // 计算平均周期，注意累计的时间间隔个数为 crossingCount - 1
        uint32_t averagePeriod_ms = cumulativeInterval / (crossingCount - 1);
        // 将周期转换为秒
        float period_sec = averagePeriod_ms / 1000.0f;
        // 计算频率 (Hz)
        frequency = 1.0f / period_sec;
    }
    else
    {
        // 如果过零点不足，无法计算正确频率，可返回 0 或用于错误处理
        frequency = 0.0f;
    }

    return frequency;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	float mean = 0;										// 用于频率计算
	float freq = 0;
	uint16_t temp_buf[ADC_BUFFER_SIZE] = {0};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	//htim2.Init.Period = TIM_PERIOD;
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C2_Init();
  MX_TIM3_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
	HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_ALL);
	int count = 0;
	for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // 计算当前角度：将 0～2π 均分 64 个点
        double angle = 2.0 * M_PI * i / SINE_TABLE_SIZE;
        // 计算正弦值，范围：-1 ~ 1
        double sineValue = sin(angle);
        // 映射到 0～255，请注意对浮点值进行四舍五入处理
        double mappedValue = (sineValue + 1.0) / 2.0 * 255.0;
        table[i] = (uint8_t)(mappedValue + 0.5);
    }
	htim2.Init.Period = TIM_PERIOD;
	HAL_TIM_Base_Init(&htim2);//配置TIM2参数 
	uint32_t period = (100000000 / UPDATE_RATE) - 1;
  htim3.Init.Period = period;
	sineFrequency = 100;
	HAL_TIM_Base_Init(&htim3);//配置TIM3周期
	if (HAL_TIM_Base_Start(&htim2) != HAL_OK)
  {
    Error_Handler();
  }//启动TIM2
	if (HAL_TIM_Base_Start(&htim3) != HAL_OK)
  {
    Error_Handler();
  }//启动TIM3
	
//	//暂时考虑使用u8g2库完成绘图 1/28:u8g2有部分存在问题，暂时弃用 2/8:移除u8g2
	OLED_Init();
	OLED_ON();
	OLED_CLS();
	OLED_ShowStr(20,3,"Pocket Instrument",1);//这一部分代码没有问题，另外：不要在注释里打//
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_RESET);
	HAL_Delay(1000);
	OLED_CLS();
	DrawSineWave();
	HAL_Delay(1000);
	OLED_CLS();//自检
//	for(int i = 0;i<128;i++)
//	{
//		for(int j = 0;j<64;j++)
//		{
//			OLED_DrawPixel(i,j);
//		}
//	}
//	HAL_Delay(1000);
//	ClearRectangleStr();
//	HAL_Delay(1000);
//	OLED_CLS();
//	HAL_TIM_Base_Start_IT(&htim2);
//	__HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
//	
	HAL_TIM_Base_Start_IT(&htim3);
	__HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
	HAL_Delay(0); 
	/* 启动 ADC 的 DMA 模式，在循环模式下自动将转换结果传送到 adcBuffer */
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcBuffer, ADC_BUFFER_SIZE) != HAL_OK)
	{
		Error_Handler();
	}

	
	//HAL_Delay(1000);
//	for (int i = 0; i<256 ;i++)
//	{
//		Write_DAC(i);
//		HAL_Delay(1);
//	}
		int temp = 0;
		int value = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
		if(flag_halfbuf == 1 && 0){
			/* 处理缓冲区前半部分的数据 */
			DisplayHalfBuffer(&adcBuffer[0],ADC_BUFFER_SIZE / 2);
//  	ProcessADCData(&adcBuffer[0], ADC_BUFFER_SIZE / 2);
			flag_halfbuf = 0;
		}
		else if(flag_fullbuf == 1){
			/* 处理缓冲区后半部分的数据 */
			memcpy(&temp_buf[0],&adcBuffer[0],ADC_BUFFER_SIZE); //保护现场
			DisplayHalfBuffer(&temp_buf[0],ADC_BUFFER_SIZE / 2);
			mean = ProcessADCData(&temp_buf[0], ADC_BUFFER_SIZE / 2);
			OLED_ShowFloat(30,0,mean * 3.3 /4096 ,4,2,2); //显示平均值
			freq = CalculateFrequency(&temp_buf[0],ADC_BUFFER_SIZE / 2, mean);
			OLED_ShowFloat(0,0,freq,4,2,2);
			flag_fullbuf = 0;
		}
		HAL_Delay(100);
//		count = __HAL_TIM_GET_COUNTER(&htim1);
//		OLED_ShowNum(0,0,count,5,2);
		//旋转编码器测试代码
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
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
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
