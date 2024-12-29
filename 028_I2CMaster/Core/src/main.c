#include "main.h"
#include "Project/project.h"

#define I2C_SLAVE_ADDRESS 0x3D // the slave address (example)

/*Global variables*/
RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;
char buffer[10];
int i = 0;
uint32_t g_nTicks; // for us
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
#ifdef DAC_PA4_PA5
    SPI_Setup();
#endif
    TIMER_Setup();
    DAC_Setup();
    RTC_Setup();
    I2C_Setup();

    /*Config global system timer to interrupt every 1us*/
    SysTick_Config(SystemCoreClock / 1000000);

    uint8_t received_data[2];

    while (1)
    {
        I2C_Start(
            I2C2,
            I2C_SLAVE_ADDRESS << 1,
            I2C_Direction_Transmitter); // start a transmission in Master transmitter mode
        I2C_Write(I2C2, 0x20);          // write one byte to the slave
        I2C_Write(I2C2, 0x03);          // write another byte to the slave
        I2C_Stop(I2C2);                 // stop the transmission

        I2C_Start(
            I2C2,
            I2C_SLAVE_ADDRESS << 1,
            I2C_Direction_Receiver);           // start a transmission in Master receiver mode
        received_data[0] = I2C_Read_Ack(I2C2); // read one byte and request another byte
        received_data[1] = I2C_Read_Nack(
            I2C2); // read one byte and don't request another byte, stop transmission
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

void RCC_Setup(void)
{
    /*Initialize GPIOB for toggling LEDs*/
    /*USART_2 (USART_B_RX: PD6 D52 on CN9, USART_B_TX: PD5 D53 on CN9) & USART_3
     * (USART_A_TX: PD8, USART_A_RX: PD9)*/
    /* Initiate clock for GPIOB and GPIOD*/
    /*Initialize GPIOA for PA3/ADC123_IN3 clock*/
    /*Initialize DMA2 clock*/
    /*I2C_B (I2C_B_SDA: PF0 D68 on CN9, I2C_B_SCL: PF1 D69 on CN9) */
    RCC_AHB1PeriphClockCmd(
        RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD
            | RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_DMA2,
        ENABLE);

    /*Initialize USART2 and USART3 clock*/
    /*Initialize TIM3 and TIM4 clock*/
    /*Initialize DAC clock*/
    /*Initialize I2C2 clock*/
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_USART2 | RCC_APB1Periph_USART3 | RCC_APB1Periph_TIM3
            | RCC_APB1Periph_TIM4 | RCC_APB1Periph_DAC | RCC_APB1Periph_I2C2,
        ENABLE);

/*Initialize ADC1 clock*/
/*Initialize SPI1 clock*/
/*Initialize TIM1 clock*/
#ifdef DAC_PA4_PA5
    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_ADC1 | RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1, ENABLE);
#else
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_TIM1, ENABLE);
#endif

    /*Initialize RTC clock*/
    PWR_BackupAccessCmd(ENABLE);

    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) != SET)
        ;

    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
    RCC_RTCCLKCmd(ENABLE);
}

void GPIO_Setup(void)
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
#ifdef DAC_PA4_PA5
    /* Initialize SPI1_CS GPIOD*/
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Set SPI1_CS*/
    GPIO_SetBits(GPIOD, GPIO_Pin_14);

    /* Initialize SPI1 on GPIOA */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
#endif
    /* Initialize GPIOA: PA3 for ADC; PA4 and PA5 for DAC*/
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;

    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Initialize I2C clock*/
    /* Initialize GPIOF: PF0 for SDA; PF1 for SCL*/
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(GPIOF, &GPIO_InitStruct);

    /*Connect GPIOF pins to AF for I2C*/
    GPIO_PinAFConfig(GPIOF, GPIO_PinSource0, GPIO_AF_I2C2);
    GPIO_PinAFConfig(GPIOF, GPIO_PinSource1, GPIO_AF_I2C2);
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
    USART_InitStruct.USART_BaudRate = 250000;
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
        if (USART_ReceiveData(USART3) == 'M')
        {
            GPIO_ToggleBits(GPIOB, GPIO_Pin_0);

            USART_SendText(USART3, "LED Toggled\n");
        }
    }
}

void USART_Setup(void)
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
            GPIO_ToggleBits(GPIOB, GPIO_Pin_7);

            USART_SendText(USART2, "LED Toggled\n");
        }
    }
}

void ADC_Setup(void)
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

void DMA_Setup(void)
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
#ifdef DAC_PA4_PA5
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
#endif
void TIMER_Setup(void)
{
    /*LED pins: PB0: TIM1_CH2, TIM3_CH3, TIM8_CH2; PB7: TIM4_CH2; PB14: TIM1_CH2,
     * TIM8_CH2*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;

    /* Set the default configuration */
    TIM_TimeBaseInitStruct.TIM_Period = 1023; // 0xFFFFFFFF;
    TIM_TimeBaseInitStruct.TIM_Prescaler = 83;
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0x0000;

    /*Initialize TIM4*/
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStruct);
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);

    TIM_OCInitTypeDef TIM_OCInitStruct;

    /* Set the default configuration */
    TIM_OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStruct.TIM_OutputNState = TIM_OutputNState_Enable; // Just for timers 1 & 8
    TIM_OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStruct.TIM_OCNPolarity = TIM_OCPolarity_High; // Just for timers 1 & 8
    TIM_OCInitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset; // Just for timers 1 & 8

    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_OCInitStruct.TIM_Pulse = 700; // 0 to 9999:TIM_Period
    /*Initialize TIM3 Output Compare channel 3*/
    TIM_OC3Init(TIM3, &TIM_OCInitStruct);

    TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);
    TIM_OCInitStruct.TIM_Pulse = 400;
    /*Initialize TIM4 Output Compare channel 2*/
    TIM_OC2Init(TIM4, &TIM_OCInitStruct);

    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OCInitStruct.TIM_Pulse = 700;
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

    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    /*Enable Timers*/
    TIM_Cmd(TIM3, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void DAC_Setup(void)
{
    /*Configure DAC*/
    DAC_InitTypeDef DAC_InitStruct;

    /* Initialize the DAC member */
    DAC_InitStruct.DAC_Trigger = DAC_Trigger_None;
    DAC_InitStruct.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStruct.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bit0;
    DAC_InitStruct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

    /* Initialize PA4 as DAC1 and PA5 as DAC2*/
    DAC_Init(DAC_Channel_1, &DAC_InitStruct);
    DAC_Init(DAC_Channel_2, &DAC_InitStruct);

    /*Enable DAC*/
    DAC_Cmd(DAC_Channel_1, ENABLE);
    DAC_Cmd(DAC_Channel_2, ENABLE);
}

void RTC_Setup(void)
{
    RTC_InitTypeDef RTC_InitStruct;

    /* Initialize the RTC member */
    RTC_InitStruct.RTC_HourFormat = RTC_HourFormat_24;
    RTC_InitStruct.RTC_AsynchPrediv = (uint32_t)0x7F;
    RTC_InitStruct.RTC_SynchPrediv = (uint32_t)0xFF;

    while (RTC_Init(&RTC_InitStruct) != SUCCESS)
        ;

    /* Time = 00h:00min:00sec */
    RTC_TimeStruct.RTC_H12 = RTC_H12_AM;
    RTC_TimeStruct.RTC_Hours = 0;
    RTC_TimeStruct.RTC_Minutes = 0;
    RTC_TimeStruct.RTC_Seconds = 0;

    while (RTC_SetTime(RTC_Format_BIN, &RTC_TimeStruct) != SUCCESS)
        ;

    /* Monday, January 01 xx00 */
    RTC_DateStruct.RTC_WeekDay = RTC_Weekday_Monday;
    RTC_DateStruct.RTC_Date = 1;
    RTC_DateStruct.RTC_Month = RTC_Month_January;
    RTC_DateStruct.RTC_Year = 0;

    while (RTC_SetDate(RTC_Format_BIN, &RTC_DateStruct) != SUCCESS)
        ;

    RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
}

void I2C_Setup(void)
{
    I2C_InitTypeDef I2C_InitStruct;

    /* initialize the ClockSpeed, Mode, DutyCycle, OwnAddress1, Ack, AcknowledgedAddress member
     */
    I2C_InitStruct.I2C_ClockSpeed = 5000;
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1 = 0;
    I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    /*Initialize I2C B*/
    I2C_Init(I2C2, &I2C_InitStruct);

    /*Enable I2C B*/
    I2C_Cmd(I2C2, ENABLE);

    /*Enable interrupt for I2C B*/
    // I2C_ITConfig(I2C2, I2C_IT_EVT, ENABLE);

    /*Enable interrupt to I2C B*/
    // NVIC_EnableIRQ(I2C2_EV_IRQn);
}
/*
void I2C2_EV_IRQHandler(void)
{
    if (I2C_GetITStatus(I2C2, I2C_IT_RXNE))
    {
        if (I2C_ReceiveData(I2C2) == 'K')
        {
            GPIO_ToggleBits(GPIOB, GPIO_Pin_14);

            I2C_SendText(I2C2, "LED Toggled\n");
        }
    }
}
*/
