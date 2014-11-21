#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "main.h"
#include "stdint.h"
#include "stdlib.h"
#include <stdio.h>

#include "spi_AD7980_ADC.h"
#include "spi_MAX5541_DAC.h"
#include "function.h"
#include "uart_com1.h"

#include "lwip/tcp.h"
#include "lwip/sockets.h"
#include "lwip/tcp_impl.h"

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "tcp_echoclient.h"
#include "sdram.h"
#define DAC_MAX  65535
//39321
//39321  
#define DAC_MIN		0

#define SYSTEMTICK_PERIOD_MS  10
uint16_t dac_data,adc_data1[DAC_MAX];
//short center[DAC_MAX]={0};
int8_t dac_step=1;
volatile uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
uint32_t timingdelay;

extern uint16_t data[data_length];
extern uint8_t connect_sucess;

void uart_send(void);
void sMAX5541_writer(uint16_t data);
void sAD7980_reader(void);
void RCC_clock_set(void);
void LED_set(void);
void STM_EVAL_PBInit(void);//(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode);
void Trig_set(void);
static void USART_Config(void);
void tcp_echoclient_connect2(void);
static err_t tcp_connected2(void *arg, struct tcp_pcb *pcb, err_t err);
void TIM6_Config(void);
void filter(uint16_t* data);
void find_peak(uint16_t peak_mindle2[],const uint16_t adc_data2[],const uint16_t data_count2);
void uart_send_peak(void);
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

static __IO uint32_t TimingDelay;

uint8_t tcp_conneced=0;
struct tcp_pcb *echoclient_pcb2;
uint8_t lwip_called=0;

#define peak_value 10000
#define PEAK_COUNT	160
#define PEAK_LENGTH 200
#define sdram_addr_low	0xc0000000
#define sdram_addr_high	0xC07fffff
	
uint16_t peak_start[PEAK_COUNT]={0},peak_end[PEAK_COUNT]={0};
uint16_t peak_middle[PEAK_COUNT]={0};

uint8_t *uwInternelBuffer;
uint8_t data_temp;
//#define lenth 65535
//uint16_t test1[lenth]={0},test2[lenth]={0},test3[lenth]={0};

uint8_t*(p[5]);

//int aa[512] __attribute__ ((section ("EXRAM")));
//char bb __attribute__ ((section ("EXRAM"))) = 0;
//char cc __attribute__ ((section ("EXRAM"))) = 1;

int ff[512] __attribute__((at(0xc0100000)));
int gg[512] __attribute__((at(0xc0100000+sizeof(int)*512)));
uint8_t dd[512];
char ee = 1;

int main()
{
	int i;

	RCC_DeInit();
	RCC_clock_set();
	LED_set();
	STM_EVAL_PBInit();//(2, 1); //init S2 button for interrupt
	Trig_set();
	sMAX5541_DAC_Init();
	sAD7980_ADC_Init();
	BSP_SDRAM_Init();
//	ETH_BSP_Config();
//	LwIP_Init();
	TIM6_Config();
	USART_Config();
	dac_data=1;
	p[0] = (uint8_t *)(sdram_addr_low);
	p[1] = (uint8_t *)(p[0]+sizeof(uint8_t)*512);
	p[2] = (uint8_t *)(p[1]+sizeof(uint8_t)*512);
	p[3] = (uint8_t *)(p[2]+sizeof(uint8_t)*512);
	p[4] = (uint8_t *)(p[3]+sizeof(uint8_t)*512);
//	  if (SysTick_Config(500))
//	  {
//	    /* Capture error */
//	    while (1);
//	  }
//	printf(" uart ok\n\r");

	for(i=0;i<512;i++)
	{
		*(__IO uint8_t*)(p[0]+i)=100;

	}
	for(i=0;i<512;i++)
	{
		*(__IO uint8_t*)(p[1]+i)=2;
	}
	for(i=0;i<512;i++)
		{
			*(__IO uint8_t*)(p[2]+i)=3;
		}
	for(i=0;i<512;i++)
		{
			*(__IO uint8_t*)(p[3]+i)=4;
		}
	for(i=0;i<512;i++)
		{
			*(__IO uint8_t*)(p[4]+i)=5;
		}
	for(i=0;i<512;i++)
	{
		dd[i]=0;
	}
	for(i=0;i<512;i++)
	{
		dd[i]=*(__IO uint8_t*)(p[0]+i);
//		dd[i]+=*(__IO uint8_t*)(p[0]+i);
//		dd[i]+=*(__IO uint8_t*)(p[1]+i);
//		dd[i]+=*(__IO uint8_t*)(p[2]+i);
//		dd[i]+=*(__IO uint8_t*)(p[3]+i);
//		dd[i]+=*(__IO uint8_t*)(p[4]+i);

	}

	  uwInternelBuffer = (uint8_t *)(0xC0300000);
	  *(__IO uint8_t*) (uwInternelBuffer)=200;
	  _delay_ms(1000);
	  data_temp=*(__IO uint8_t*) (uwInternelBuffer);

	while(1)
	{
		__asm("NOP");
	}
	for(;;)
	{
//	    int i;
		// Sawtooth
      if (dac_data >= DAC_MAX)
	{
	  dac_step = 1;
	  dac_data = DAC_MIN;
	  sMAX5541_writer(dac_data);
	  _delay_ms(100);
//	  	  uart_send();
//		filter(adc_data1);
	  find_peak(peak_middle,adc_data1,DAC_MAX);
//	  uart_send_peak();

//	  uart_send();
	}
#if 0
      if (ETH_CheckFrameReceived ())
	{
	  /* process received ethernet packet */
	  LwIP_Pkt_Handle ();
	}
//      _delay_ms(10);
//      if(jj++>1000)

      if(connect_sucess==2)
	{
	  static uint8_t k;

	  /*connect to tcp server */
	  uint32_t ii;
	  jj=0;
	  if(k++>5)k=0;
	  for (ii = 0; ii < data_length; ii++)
	    {
	      data[ii] = ii*k;
	    }
	  tcp_echoclient_connect ();
	}
#endif
      dac_data+=dac_step;
      sMAX5541_writer(dac_data);
      _delay_us(5);
      sAD7980_reader();
	}
}



//uint16_t peak_middle[16]={0};
void find_peak(uint16_t peak_mindle2[],const uint16_t adc_data2[],const uint16_t data_count2)
{
//	int i,j=0;
//	uint32_t tempxy=0,tempx=0;
//	for(i=0;i<PEAK_LENGTH;i++)
//	{
//		tempxy+=i*adc_data2[i];
//		tempx+=i;
//	}
//
//	for(j=PEAK_LENGTH/2;j<DAC_MAX-PEAK_LENGTH/2;j++)
//	{
//		tempxy=tempxy-j*adc_data2[j]+(j+PEAK_LENGTH)*adc_data2[j+PEAK_LENGTH];
//		tempx+=PEAK_LENGTH;
//		center[j+PEAK_LENGTH/2]=tempxy/tempx;
//	}
//
//	for(j=0,i=0+1;i<DAC_MAX-1;i++)
//	{
//		if(adc_data2[i]>800&&center[i]>center[i-1]&&center[i]>=center[i+1])
//		{
//			peak_mindle2[j]=i;
//			if(peak_mindle2[j]-peak_mindle2[j-1]<500)
//			{
//				peak_mindle2[j-1]=center[j-1]>=center[j]?peak_mindle2[j-1]:peak_mindle2[j];
//				peak_mindle2[j]=0;
//			}
//			else
//			{
//			j++;
//			}
//		}
//	}
//	for(i=0;i<DAC_MAX;i++)
//	{
//		adc_data1[i]=0;	//clear data
//	}

//	__asm__("nop");
}

void uart_send(void)
{
	uint16_t i=0;
	  if (adc_data1[DAC_MAX - 1000] != 0 || adc_data1[DAC_MAX - 1001] != 0|| adc_data1[DAC_MAX - 1002] != 0)
			{
				for (i = DAC_MIN; i < DAC_MAX; i++)
				{
					printf("%d,%d\n", i, adc_data1[i]);
//					printf("%d,%d,", i, adc_data1[i]);
//					filter(&adc_data1[i]);
//					printf("%d\n\r", adc_data1[i]);
				}
			}
}
void uart_send_peak(void)
{
	uint8_t i;
	//printf("ok\n");
	for(i=0;i<PEAK_COUNT;i++)
	{
		if(peak_middle[i]!=0)
		{
			printf("%d,", peak_middle[i]);
		}
	}
	printf("\n");
}
void sMAX5541_writer(uint16_t data)
{
_delay_us(10);
sMAX5541_DAC_CS_LOW();
_delay_us(10);
SPI_I2S_SendData(SPI2,data);
_delay_us(10);
sMAX5541_DAC_CS_HIGH();
}
void sAD7980_reader(void)
{
#define adc_times	4
		uint32_t data_temp[3]={0};
		int i;
		for (i = 0; i < adc_times; i++) {
			sAD7980_ADC_CS_HIGH();
			while (GPIO_ReadInputDataBit(sAD7980_IRQ_GPIO_PORT,
					sAD7980_IRQ_PIN) != 0) {
				uint8_t i;
				for (i = 0; i < 3; i++) {
//						uint16_t temp1;
					SPI_I2S_SendData(SPI5, 0);
					_delay_us(20);
					data_temp[i] += SPI_I2S_ReceiveData(SPI5);
//						if(data_temp[i]<temp1)
//							data_temp[i]=temp1;
				}
				sAD7980_ADC_CS_LOW();
			}
		}
		printf("%d,%d,%d\n", dac_data, data_temp[0]/4,data_temp[1]/4);
//		adc_data1[dac_data] = data_temp[0]/adc_times;
		//adc_data1[dac_data] = data_temp[2]/adc_times;

}
////中值平均滤波算法
#define N 10
void filter(uint16_t* data)
{
	uint16_t count, i, j;
	uint16_t value_buf[N];
	uint32_t sum = 0;
	for (count = 0; count < N; count++)
	{
		value_buf[count] = *(data+count);
	}
	for (j = 0; j < N - 1; j++)
	{
		for (i = 0; i < N - j; i++)
		{
			if (value_buf[i] > value_buf[i + 1])
			{
				uint16_t temp;
				temp = value_buf[i];
				value_buf[i] = value_buf[i + 1];
				value_buf[i + 1] = temp;
			}
		}
	}
	for (count = 1; count < N - 1; count++)
		sum += value_buf[count];
	*data =(sum / (N - 2));
}
void tcp_echoclient_connect2(void)
{
////////////////////////////////////////
  struct ip_addr DestIPaddr;

  /* create new tcp pcb */
  echoclient_pcb2 = tcp_new();

  if (echoclient_pcb2 != NULL)
  {
    IP4_ADDR( &DestIPaddr, DEST_IP_ADDR0, DEST_IP_ADDR1, DEST_IP_ADDR2, DEST_IP_ADDR3 );

    /* connect to destination address/port */
    tcp_connect(echoclient_pcb2,&DestIPaddr,DEST_PORT,tcp_connected2);
  }
  else
  {
    /* deallocate the pcb */
    memp_free(MEMP_TCP_PCB, echoclient_pcb2);
#ifdef SERIAL_DEBUG
    printf("\n\r can not create tcp pcb");
#endif
  }
  /////////
}
static err_t tcp_connected2(void *arg, struct tcp_pcb *pcb, err_t err)
{
  //{
////#define data_length	1000
////u8_t   data[100];
//  uint16_t data[data_length];
//
//  uint32_t i;
//  s8_t tcp_send_stat = -1;
//  for (i = 0; i < data_length; i++)
//    {
//      data[i] = i;
//    }
//
//  tcp_write (pcb, data, sizeof(data), 1); /* 发送数据 */
////  tcp_output(pcb);
//  tcp_close (pcb);
////	tcp_conneced=1;
  return ERR_OK;
}
void RCC_clock_set(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  /* GPIOC,GPIOD and GPIOI Periph clock enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

	RCC_MCO1Config(RCC_MCO1Source_HSE,RCC_MCO1Div_1);
	//RCC_HSICmd(ENABLE); //enable internal clock 16M
	RCC_HSEConfig(RCC_HSE_ON);
}


void LED_set(void)
{
	  GPIO_InitTypeDef  GPIO_InitStructure;

	  /* GPIOC,GPIOD and GPIOI Periph clock enable */
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOD, &GPIO_InitStructure);
}
void Trig_set(void)
{
	  GPIO_InitTypeDef  GPIO_InitStructure;

	  /* GPIOC,GPIOD and GPIOI Periph clock enable */
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);

	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	  GPIO_Init(GPIOH, &GPIO_InitStructure);
}


/**
  * @brief  Inserts a delay time.
  * @param  nCount: number of 10ms periods to wait for.
  * @retval None
  */
void Delay(uint32_t nCount)
{
  /* Capture the current local time */
  timingdelay = LocalTime + nCount;

  /* wait until the desired delay finish */
  while(timingdelay > LocalTime)
  {
  }
}
/**
  * @brief  Updates the system local time
  * @param  None
  * @retval None
  */
void Time_Update(void)
{
  LocalTime += SYSTEMTICK_PERIOD_MS;
}

void TIM6_Config(void)
{	//set to 250ms per times for ethenet
  uint16_t PrescalerValue = 0;
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

  /* Enable the TIM3 gloabal Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = TIM6_DAC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 3000000) - 1;

  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = 60000;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

  /* Prescaler configuration */
  TIM_PrescalerConfig(TIM6, PrescalerValue, TIM_PSCReloadMode_Immediate);

  /* TIM Interrupts enable */
  TIM_ITConfig(TIM6, TIM_IT_Update, ENABLE);

  /* TIM3 enable counter */
  TIM_Cmd(TIM6, ENABLE);
}

/*************************************************
  * @brief  Configures the USART Peripheral.
  * @param  None
  * @retval None
  */
static void USART_Config(void)
{
  USART_InitTypeDef USART_InitStructure;

  /* USARTx configured as follows:
        - BaudRate = 115200 baud
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 256000;//380400;//921600;//256000;//115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

  STM_EVAL_COMInit(COM1, &USART_InitStructure);
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(EVAL_COM1, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET)
  {}

  return ch;
}

//#ifdef  USE_FULL_ASSERT
//
///**
//  * @brief  Reports the name of the source file and the source line number
//  *         where the assert_param error has occurred.
//  * @param  file: pointer to the source file name
//  * @param  line: assert_param error line source number
//  * @retval None
//  */
//void assert_failed(uint8_t* file, uint32_t line)
//{
//  /* User can add his own implementation to report the file name and line number,
//     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
//
//  /* Infinite loop */
//  while (1)
//  {
//  }
//}
//#endif

/**
  * @brief  Configures Button GPIO and EXTI Line.
  * @param  Button: Specifies the Button to be configured.
  *   This parameter can be one of following parameters:
  *     @arg BUTTON_WAKEUP: Wakeup Push Button
  *     @arg BUTTON_TAMPER: Tamper Push Button
  *     @arg BUTTON_TAMPER: Tamper Push Button
  * @param  Button_Mode: Specifies Button mode.
  *   This parameter can be one of following parameters:
  *     @arg BUTTON_MODE_GPIO: Button will be used as simple IO
  *     @arg BUTTON_MODE_EXTI: Button will be connected to EXTI line with interrupt
  *                     generation capability
  * @retval None
  */

//#define KEY_BUTTON_PIN                   GPIO_Pin_13
//#define KEY_BUTTON_GPIO_PORT             GPIOC
//#define KEY_BUTTON_GPIO_CLK              RCC_AHB1Periph_GPIOC
//#define KEY_BUTTON_EXTI_LINE             EXTI_Line13
//#define KEY_BUTTON_EXTI_PORT_SOURCE      EXTI_PortSourceGPIOC
//#define KEY_BUTTON_EXTI_PIN_SOURCE       EXTI_PinSource13
//#define KEY_BUTTON_EXTI_IRQn             EXTI15_10_IRQn

void STM_EVAL_PBInit(void)//(Button_TypeDef Button, ButtonMode_TypeDef Button_Mode)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;


  /* Enable the BUTTON Clock */
  RCC_AHB1PeriphClockCmd(KEY_BUTTON_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure Button pin as input */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = KEY_BUTTON_PIN;
  GPIO_Init(KEY_BUTTON_GPIO_PORT, &GPIO_InitStructure);


 // if (Button_Mode == BUTTON_MODE_EXTI)
  {
    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(KEY_BUTTON_EXTI_PORT_SOURCE, KEY_BUTTON_EXTI_PIN_SOURCE);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = KEY_BUTTON_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

    if(1)//(Button != BUTTON_WAKEUP)
    {
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    }
    else
    {
      EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    }
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = KEY_BUTTON_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
  }
}


