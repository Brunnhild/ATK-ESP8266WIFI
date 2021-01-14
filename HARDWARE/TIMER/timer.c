#include "timer.h"
#include "led.h"
#include "adc.h"
#include "delay.h"
#include "lcd.h"
#include "adc3.h"
#include "lsens.h"
#include "app.h"

short temp; 
u8 adcx;
extern u16 USART3_RX_STA;

void TIM7_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)
    {
		if (!is_it()) {
            TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
            return;
        }
        USART3_RX_STA |= 1 << 15;
        TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
        TIM_Cmd(TIM7, DISABLE);
    }
}

void TIM7_Int_Init(u16 arr, u16 psc)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);

    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void TIM3_Int_Init(u16 arr, u16 psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);

    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM3, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// ADC handler
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        temp = Get_Temprate();
        if (temp < 0)
        {
            temp = -temp;
            LCD_ShowString(30 + 10 * 8, 110, 16, 16, 16, "-");
        }
        else
            LCD_ShowString(30 + 10 * 8, 110, 16, 16, 16, " ");

        LCD_ShowxNum(30 + 11 * 8, 110, temp / 100, 2, 16, 0);
        LCD_ShowxNum(30 + 14 * 8, 110, temp % 100, 2, 16, 0);

        adcx = Lsens_Get_Val();
        set_temp_adc(temp, adcx);
        LCD_ShowxNum(30 + 10 * 8, 90, adcx, 3, 16, 0);
        LED1 = !LED1;
    }
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
}
