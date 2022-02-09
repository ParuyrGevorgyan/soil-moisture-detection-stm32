/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "st7789.h"
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



#define BUF_LEN 256

uint8_t RxBuf[BUF_LEN];
uint8_t TxBuf[BUF_LEN];

__IO int Rx_Empty=0;
__IO int Rx_Busy=0;

__IO int Tx_Empty=0;
__IO int Tx_Busy=0;

char tmp_buf[256];



uint8_t frameState = 0;			// 0 - czekamy na początek ramki
								// 1 - ramka jest przetwarzana

const char device_address[3] = "ST";
char source_address[3];
uint8_t frameLen = 0; 			// ilość odebranych znaków w ramce
int cmdLength; 					// długość komendy
int cmd_len_check = 0; 			// sprawdzanie, czy w komendzie nie ma zbędnych znaków
uint8_t checkSum[3];

uint16_t adc_buf[256];

int count = 0;
int average=0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/*sprawdza czy wszytkie znaki w buforze zostali odczytane*/
uint8_t USART_kbhit(){
	if(Rx_Empty==Rx_Busy){
		return 0;
	}else{
		return 1;
	}
}//USART_kbhit
/*Funkcja pobierająca znak do bufora*/
uint8_t USART_getchar(){
uint8_t tmp;
	if(Rx_Empty!=Rx_Busy){
		tmp=RxBuf[Rx_Busy];
		Rx_Busy++;
		if(Rx_Busy >= BUF_LEN)Rx_Busy=0;
		return tmp;
	}else return 0;
}//USART_getchar

uint8_t USART_getline(char *buf){
static uint8_t bf[256];
static uint8_t idx=0;
int i;
uint8_t ret;
	while(USART_kbhit()){
		bf[idx]=USART_getchar();
		if(((bf[idx]==10)||(bf[idx]==13))){
			bf[idx]=0;
			for(i=0;i<=idx;i++){
				buf[i]=bf[i];
			}
			ret=idx;
			idx=0;
			return ret;
		}else{
			idx++;
			if(idx>=256)idx=0;
		}
	}
	return 0;
}//USART_getline

void Send(char* message, ...){
	char temp[64];

	volatile int idx = Tx_Empty;


	va_list arglist;
	va_start(arglist, message);
	vsprintf(temp, message, arglist);
	va_end(arglist);

	for(int i = 0; i < strlen(temp); i++){
		TxBuf[idx] = temp[i];
		idx++;
		if(idx >= BUF_LEN){
			idx = 0;
		}

	}
	__disable_irq();

	if(Tx_Empty == Tx_Busy){
		Tx_Empty = idx;
		uint8_t tmp = TxBuf[Tx_Busy];
		Tx_Busy++;
		if(Tx_Busy >= BUF_LEN){
			Tx_Busy = 0;
		}

		HAL_UART_Transmit_IT(&huart2, &tmp, 1);

		Tx_Empty = idx;
	}
	__enable_irq();
}
/*sprawdza jaka komenda została wysyłąna z PC*/
void ShLive(){
	if (count>=0){
		uint8_t msg[5];
		sprintf(msg, "%d ", average);
		msg[4]='\0';
		Send(msg);

		count--;
	}
	else{
		HAL_TIM_Base_Stop_IT(&htim11);
	}

}
void DoCmd(char* cmd)
{
	//int count = 0;
	//int adc_rn = 0;
	if (sscanf(cmd,"SHLIVE(%d)",&count)==1){

		//while(count>0){
			//show avg calc x times

			/*HAL_ADC_Start(&hadc1);
			HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
			adc_rn = HAL_ADC_GetValue(&hadc1);
			char msg[5];
			sprintf(msg, "%d", adc_rn);
			HAL_Delay(1000);//no delay
			Send(msg);*/


			//char msg[5];
			//sprintf(msg, "%d, ", average);
			//Send(msg);

			//count--;
			//HAL_Delay(500);
		//}
		ShLive();
		HAL_TIM_Base_Start_IT(&htim11);
	}
	else if (strcmp(cmd,"AVG")==0){
		//int average=0;
		/*for(int i = 255; i>=0; i-- ){
			average += adc_buf[i];
		}
		average /= 256;*/
		char msg[5];
		sprintf(msg, "%d", average);
		Send(msg);


	}
	else if(strcmp(cmd,"MDN")==0){
	    int temp=0;
	    char msg[5];
	    for(int i=0 ; i<256 ; i++)
	    {
	        for(int j=0 ; j<256-1 ; j++)
	        {
	            if(adc_buf[j]>adc_buf[j+1])
	            {
	                temp = adc_buf[j];
	                adc_buf[j] = adc_buf[j+1];
	                adc_buf[j+1] = temp;
	            }
	        }
	        int median=0;

			// if number of elements are even
			median = (adc_buf[(256-1)/2] + adc_buf[256/2])/2;


			sprintf(msg, "%d", median);
	    }
	    Send(msg);

	}
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  uint16_t timer_val;

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
  MX_USART2_UART_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM11_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_buf, 256);

  //HAL_TIM_Base_Start_IT(&htim11);
  /* USER CODE END 2 */
  ST7789_Init();

  ST7789_Fill_Color(GRAY);
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  HAL_UART_Receive_IT(&huart2,&RxBuf[Rx_Empty],1);
  while (1)
  {
	  HAL_ADC_Start(&hadc1);
	  uint8_t singleFrameChar;
	  if(USART_kbhit()){
		  // rozpoczęcie poszukiwania ramki
		  if (frameState == 0)
		  {                                      // poszukiwanie początku ramki
			  singleFrameChar = USART_getchar(); // pobieranie pierwszego znaku

			  if (singleFrameChar == 35 /* # */)
			  { // jeżeli znaleziono znak początku ramki
				  frameState = 1;
				  frameLen = 0;
			  }
		  }
		  else if (frameState == 1)
		  {                                      // Jeżeli znak początku ramki został znaleziony
			  singleFrameChar = USART_getchar(); // pobieranie kolejnego znaku

			  tmp_buf[frameLen] = singleFrameChar; // kolekcjonowanie ramki
			  frameLen++;

			  //256 - maksymalna ilość danych
			  //6 - minimalna długość ramki
			  //1 - znak końca ramki (jest pobierany przy zczytywaniu ramki z bufora odbiorczego)
			  if (frameLen > (256 + 6 + 1))
			  {                   // jeżeli ilość znaków w ramce przekroczy dopuszczalny rozmiar ramki (266 znaków)
				  frameLen = 0;   // ramka jest resetowana
				  frameState = 0; // i następuje poszukiwanie nowej ramki
				  //return;
				  break;
			  }

			  if (singleFrameChar == 35 /* # */)
			  { // jeżeli znak początku się powtarza
				  if (frameLen > 0)
				  {
					  frameLen = 0; // ramka jest resetowana
				  }
			  }

			  if (singleFrameChar == 36 /* $ */)
			  { // kolekcjonowanie do końca ramki

				  if (frameLen - 1 > 6)
				  { // jeżeli podano więcej znaków, niż minimalna ilość znaków w ramce
					  char destination_address[3];
					  char data_length[3];

					  memcpy(source_address, &tmp_buf[0], 2);
					  memcpy(destination_address, &tmp_buf[2], 2);
					  memcpy(data_length, &tmp_buf[4], 2);

					  source_address[2] = '\0';
					  destination_address[2] = '\0';
					  data_length[2]='\0';

					  cmdLength = atoi(data_length);

					  if (strncmp(device_address, destination_address, 2) == 0)
					  {
						  if (cmdLength <= 256)//delete
						  {

							  if ((6 + cmdLength) == (frameLen - 1 - 2 /*forchksum*/))
							  {                            // jeżeli komenda została przysłana
								  char cmd[cmdLength + 1]; // komenda

								  memcpy(cmd, &tmp_buf[6], cmdLength);
								  memcpy(checkSum, &tmp_buf[6 + cmdLength], 2);
								  checkSum[2]='\0';

								  uint8_t hexSum = (int)strtol(checkSum, NULL, 16);//uint8
								  int result = 0, ctrl = 0;
								  for (int i = cmdLength - 1; i >= 0; i--)
								  {
									  result = cmd[i] ^ ctrl;
									  ctrl = result;
								  }
								  if ((result ^ hexSum) == 0)
								  {
									  cmd[cmdLength] = '\0'; // dodanie znaku null na końcu tablicy

									  DoCmd(cmd);
								  }else{/*some error about the wrong cvhecksum*/}
							  }else{/*wrong command lenght field*/}
						  }else{/*error frame data fiel too long*/}
					  }else{/*wrong destination addrs*/}
				  }else{/*wrong frame len less than 6*/}

				  frameState = 0;
				  frameLen = 0;
			  }
		  }
	  }


		int percentage = floor(average/40);

		char displayMsg[21];
		sprintf(displayMsg, "CURRENT AVERAGE: %02d%%", percentage);
		displayMsg[20]='\0';
		ST7789_WriteString(10, 10, displayMsg, Font_11x18, GBLUE, GRAY);
		if(percentage<=20){
			ST7789_WriteString(10, 75, "CONDITION:  BAD", Font_11x18, RED, GRAY);
		}
		else{
			ST7789_WriteString(10, 75, "CONDITION: GOOD", Font_11x18, GREEN, GRAY);
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance==TIM11){
		ShLive();
	}
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	//HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	//avg calculate
	for(int i = 255; i>=0; i-- ){
		average += adc_buf[i];
	}
	average /= 256;

}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(Tx_Busy != Tx_Empty){
		uint8_t temp = TxBuf[Tx_Busy];
		Tx_Busy++;
		if(Tx_Busy >= BUF_LEN)Tx_Busy = 0;
		HAL_UART_Transmit_IT(&huart2, &temp, 1);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart->Instance==USART2){
		Rx_Empty++;
		if(Rx_Empty>=BUF_LEN)Rx_Empty=0;
		HAL_UART_Receive_IT(&huart2,&RxBuf[Rx_Empty],1);
	}
}



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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
