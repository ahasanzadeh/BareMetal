#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

void RCC_Setup(void);
void GPIO_Setup(void);
void VirtualCOMPort_Setup(void);
void USART_Setup(void);
void ADC_Setup(void);
void DMA_Setup(void);
void SPI_Setup(void);
void TIMER_Setup(void);

/*USART prototypes*/
void USART_SendText(USART_TypeDef* USARTx, volatile char* sendText);
void USART_SendNumber(USART_TypeDef* USARTx, uint32_t sendNumber);

/*SPI prototypes*/
void SPI_Tx(uint8_t address, uint8_t data);
uint8_t SPI_Rx(uint8_t address);

/*Delay prototypes*/
void delayMS(uint32_t ms);
void delayUS(uint32_t us);

/*Global variables*/
char buffer[10];
int i = 0;
uint32_t n;
uint16_t adcData[2];
float temp;
uint16_t g_nZAx;
uint32_t g_nSysTick;

int main(void)
{
    RCC_Setup();
    GPIO_Setup();
    VirtualCOMPort_Setup();
    USART_Setup();
    ADC_Setup();
    DMA_Setup();
    SPI_Setup();
    TIMER_Setup();

    /*Config global system timer to interrupt every 1us*/
    SysTick_Config(SystemCoreClock / 1000000);

    while (1)
    {
        USART_SendNumber(USART3, n++);
        USART_SendText(USART3, "\tTIM3, TIM4, and TIM1 blink green, blue, and RED LEDs\n");

        delayMS(1000);
    }
    return 0;
}

void SysTick_Handler(void)
{
    g_nSysTick++;
}

void delayUS(uint32_t us)
{
    g_nSysTick = 0;
    while (g_nSysTick < us)
    {
        __NOP();
    }
}

void delayMS(uint32_t ms)
{
    g_nSysTick = 0;
    while (g_nSysTick < (ms * 1000))
    {
        __NOP();
    }
}

void RCC_Setup()
{
    /*Initialize GPIOB for toggling LEDs*/
    /*USART_2 (USART_B_RX: PD6 D52 on CN9, USART_B_TX: PD5 D53 on CN9) & USART_3
     * (USART_A_TX: PD8, USART_A_RX: PD9)*/
    /* Initiate clock for GPIOB and GPIOD*/
    /*Initialize GPIOA for PA3/ADC123_IN3 clock*/
    /*Initialize DMA2 clock*/
    RCC_AHB1PeriphClockCmd(
        RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD
            | RCC_AHB1Periph_DMA2,
        ENABLE);

    /*Initialize USART2 and USART3 clock*/
    /*Initialize TIM3 and TIM4 clock*/
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3 | RCC_APB1Periph_TIM3
            | RCC_APB1Periph_TIM4,
        ENABLE);

    /*Initialize ADC1 clock*/
    /*Initialize SPI1 clock*/
    /*Initialize TIM1 clock*/
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_ADC1 | RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1, ENABLE);
}

void GPIO_Setup()
{
    /* Initialize GPIOB*/
    GPIO_InitTypeDef GPIO_InitStruct;

    /*Reset every member element of the structure*/
    memset(&GPIO_InitStruct, 0, sizeof(GPIO_InitStruct));

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Connect GPIOB pins to AF for TIM3, TIM4, TIM1*/
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_TIM4);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM1);

    /* Initialize GPIOD*/
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Initialize SPI1_CS GPIOD*/
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Set SPI1_CS*/
    GPIO_SetBits(GPIOD, GPIO_Pin_14);

    /* Initialize GPIOA */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;

    /* Initialize SPI1 on GPIOA */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;

    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void VirtualCOMPort_Setup(void)
{
    /*USART_3 (USART_C_TX: PD8, USART_C_RX: PD9) has virtual COM port capability*/

    /*Configure USART3*/
    USART_InitTypeDef USART_InitStruct;

    /*Reset every member element of the structure*/
    memset(&USART_InitStruct, 0, sizeof(USART_InitStruct));

    /*Connect GPIOD pins to AF for USART3*/
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource8, GPIO_AF_USART3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource9, GPIO_AF_USART3);

    /*Configure USART3*/
    USART_InitStruct.USART_BaudRate = 9600;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    /*Initialize USART3*/
    USART_Init(USART3, &USART_InitStruct);

    /*Enable USART3*/
    USART_Cmd(USART3, ENABLE);

    /*Enable interrupt for UART3*/
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /*Enable interrupt to UART3*/
    NVIC_EnableIRQ(USART3_IRQn);
}

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE))
    {
        if (USART_ReceiveData(USART3) == 'K')
        {
            GPIO_ToggleBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14);

            USART_SendText(USART3, "LED Toggled\n");
        }
    }
}

void USART_Setup()
{
    /*USART_2 (USART_B_RX: PD6 D52 on CN9, USART_B_TX: PD5 D53 on CN9)*/
    /*Configure USART2*/
    USART_InitTypeDef USART_InitStruct;

    /*Reset every member element of the structure*/
    memset(&USART_InitStruct, 0, sizeof(USART_InitStruct));

    /*Connect GPIOD pins to AF to USART2*/
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);

    /*Configure USART2*/
    USART_InitStruct.USART_BaudRate = 9600;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    /*Initialize USART2*/
    USART_Init(USART2, &USART_InitStruct);

    /*Enable USART2*/
    USART_Cmd(USART2, ENABLE);

    /*Enable interrupt for UART2*/
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    /*Enable interrupt to UART2*/
    NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE))
    {
        if (USART_ReceiveData(USART2) == 'L')
        {
            GPIO_ToggleBits(GPIOB, GPIO_Pin_0 | GPIO_Pin_7 | GPIO_Pin_14);

            USART_SendText(USART2, "LED Toggled\n");
        }
    }
}

void ADC_Setup()
{
    /*ADC Common Init structure definition*/
    ADC_CommonInitTypeDef ADC_CommonInitStruct;

    /*ADC Init structure definition*/
    ADC_InitTypeDef ADC_InitStruct;

    /* Initialize the ADC_Mode, ADC_Prescaler, ADC_DMAAccessMode,
     * ADC_TwoSamplingDelay member */
    ADC_CommonInitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStruct.ADC_Prescaler = ADC_Prescaler_Div2;
    ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;

    ADC_CommonInit(&ADC_CommonInitStruct);

    /* Initialize the ADC_Mode, ADC_ScanConvMode, ADC_ContinuousConvMode,
    ADC_ExternalTrigConvEdge, ADC_ExternalTrigConv, ADC_DataAlign,
    ADC_NbrOfConversion member */
    ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStruct.ADC_ScanConvMode = ENABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfConversion = 2;

    ADC_Init(ADC1, &ADC_InitStruct);

    /*Configuring ADC1*/
    ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 1, ADC_SampleTime_144Cycles);
    ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 2, ADC_SampleTime_144Cycles);

    /*Enable temperature sensor channel*/
    ADC_TempSensorVrefintCmd(ENABLE);

    /*Enable ADC DMA request*/
    ADC_DMACmd(ADC1, ENABLE);

    /*Enable ADC1*/
    ADC_Cmd(ADC1, ENABLE);

    /*Enable ADC DMA request after last transfer (Single-ADC mode)*/
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);

    /*Start Conversion*/
    ADC_SoftwareStartConv(ADC1);
}

void DMA_Setup()
{
    DMA_InitTypeDef DMA_InitStruct;

    /* Initialize the DMA Struct members */
    DMA_InitStruct.DMA_Channel = 0;
    DMA_InitStruct.DMA_PeripheralBaseAddr =
        (uint32_t)&ADC1->DR;                                /*Address needed; not the value*/
    DMA_InitStruct.DMA_Memory0BaseAddr = (uint32_t)adcData; /*Address needed; not the value*/
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStruct.DMA_BufferSize = 2;
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
    DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
    DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

    /*Initialize DMA2*/
    DMA_Init(DMA2_Stream0, &DMA_InitStruct);

    /*Enable DMA2*/
    DMA_Cmd(DMA2_Stream0, ENABLE);
}

void SPI_Setup()
{
    /*SPI1 (SPI_A_SCK: PA5, SPI_A_MISO: PA6, SPI_A_MOSI: PA7)*/
    /*Configure SPI1*/
    SPI_InitTypeDef SPI_InitStruct;

    /*Reset every member element of the structure*/
    memset(&SPI_InitStruct, 0, sizeof(SPI_InitStruct));

    /*Connect GPIOA pins to AF*/
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

    /* Initialize the SPI struct members */
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b; /*Check the slave Datasheet*/
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;       /*Check the slave Datasheet*/
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;      /*Check the slave Datasheet*/
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft | SPI_NSSInternalSoft_Set;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB; /*Check the slave Datasheet*/
    SPI_InitStruct.SPI_CRCPolynomial = 7;

    /*Initialize SPI1*/
    SPI_Init(SPI1, &SPI_InitStruct);

    /*Enable SPI1*/
    SPI_Cmd(SPI1, ENABLE);
}

void TIMER_Setup(void)
{
    /*LED pins: PB0: TIM1_CH2, TIM3_CH3, TIM8_CH2; PB7: TIM4_CH2; PB14: TIM1_CH2,
     * TIM8_CH2*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

    /* Set the default configuration */
    TIM_TimeBaseInitStruct.TIM_Period = 9999; // 0xFFFFFFFF;
    TIM_TimeBaseInitStruct.TIM_Prescaler = 8399;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0x0000;

    /*Initialize TIM4*/
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStruct);
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);

    TIM_OCInitTypeDef TIM_OCInitStruct;

    /* Set the default configuration */
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_Toggle;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable; // Just for timers 1 & 8
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCPolarity_High; // Just for timers 1 & 8
    TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset; // Just for timers 1 & 8

    TIM_OCInitStruct.TIM_Pulse = 5000; // 0 to 9999:TIM_Period
    /*Initialize TIM3 Output Compare channel 3*/
    TIM_OC3Init(TIM3, &TIM_OCInitStruct);

    TIM_OCInitStruct.TIM_Pulse = 5000;
    /*Initialize TIM4 Output Compare channel 2*/
    TIM_OC2Init(TIM4, &TIM_OCInitStruct);

    TIM_OCInitStruct.TIM_Pulse = 5000;
    /*Initialize TIM1 Output Compare channel 2*/
    TIM_OC2Init(TIM1, &TIM_OCInitStruct);

    /*Configuration for TIM1 and TIM8*/
    TIM_BDTRInitTypeDef TIM_BDTRInitStruct;

    TIM_BDTRInitStruct.TIM_OSSRState = TIM_OSSRState_Enable;
    TIM_BDTRInitStruct.TIM_OSSIState = TIM_OSSIState_Enable;
    TIM_BDTRInitStruct.TIM_LOCKLevel = TIM_LOCKLevel_3;
    TIM_BDTRInitStruct.TIM_DeadTime = 0x00;
    TIM_BDTRInitStruct.TIM_Break = TIM_Break_Enable;
    TIM_BDTRInitStruct.TIM_BreakPolarity = TIM_BreakPolarity_Low;
    TIM_BDTRInitStruct.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;

    TIM_BDTRStructInit(&TIM_BDTRInitStruct);

    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStruct);

    TIM_CCPreloadControl(TIM1, ENABLE);
    /*The below line is necessary to enable TIM1 and TIM8*/
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    /*Enable Timers*/
    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void SPI_Tx(uint8_t address, uint8_t data)
{
    /*Reset SPI1_CS*/
    GPIO_ResetBits(GPIOD, GPIO_Pin_14);

    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE))
        ;
    SPI_I2S_SendData(SPI1, address);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE))
        ;
    SPI_I2S_ReceiveData(SPI1);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE))
        ;
    SPI_I2S_SendData(SPI1, data);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE))
        ;
    SPI_I2S_ReceiveData(SPI1);

    /*Set SPI1_CS and wait*/
    GPIO_SetBits(GPIOD, GPIO_Pin_14);
}

uint8_t SPI_Rx(uint8_t address)
{
    /*Reset SPI1_CS*/
    GPIO_ResetBits(GPIOD, GPIO_Pin_14);

    address = 0x00 | address;
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE))
        ;
    SPI_I2S_SendData(SPI1, address);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE))
        ;
    SPI_I2S_ReceiveData(SPI1);
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE))
        ;
    SPI_I2S_SendData(SPI1, 0x00); /*Check the slave Datasheet*/
    while (!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE))
        ;

    /*Set SPI1_CS and wait*/
    GPIO_SetBits(GPIOD, GPIO_Pin_14);

    return SPI_I2S_ReceiveData(SPI1);
}

void USART_SendText(USART_TypeDef* USARTx, volatile char* sendText)
{
    while (*sendText)
    {
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) != SET)
            ;
        USART_SendData(USARTx, *sendText);
        *sendText++;
    }
}

void USART_SendNumber(USART_TypeDef* USARTx, uint32_t sendNumber)
{
    /*A temp array to build results of conversion*/
    char value[10];
    /*Loop index*/
    int i = 0;

    do
    {
        /*Convert integer to character*/
        value[i++] = (char)(sendNumber % 10) + '0';
        sendNumber /= 10;
    } while (sendNumber);

    /*Send data*/
    while (i)
    {
        // USART_SendNumber8b(USARTx, value[--i]);
        /*Wait until data register is empty*/
        while (!USART_GetFlagStatus(USARTx, USART_FLAG_TXE))
            ;
        USART_SendData(USARTx, value[--i]);
    }
}