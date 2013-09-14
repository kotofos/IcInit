#include "stm32f10x.h"

#define ZON GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ZOFF GPIO_ResetBits(GPIOB, GPIO_Pin_0)

void test(void) {
    return;
}
uint32_t bitTim[50], bitTimCount = 1;
int main()
{
    SystemInit();
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    //data
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init (GPIOA, &GPIO_InitStructure);
    
    //power
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init (GPIOB, &GPIO_InitStructure);
    
    //Tim1 for input capture
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; //1 us
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 0xffff;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    
    TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICFilter = 0x0;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
    TIM_ICInitStructure.TIM_ICPrescaler = 0x0;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; 
    TIM_ICInit(TIM1, &TIM_ICInitStructure);
    TIM_ITConfig(TIM1, TIM_EventSource_CC4, ENABLE);
    int ms = 1000000;
    TIM_Cmd(TIM1, ENABLE);
    for(uint32_t i = 0; i<ms; i++){
        TIM_SetCounter(TIM3, 0);
        while((TIM_GetCounter(TIM1)) < 999);
    }
    NVIC_EnableIRQ(TIM1_CC_IRQn);
    
    while (1);
    TIM_Cmd(TIM1, ENABLE);
        
    ZON;
    while (bitTimCount < 31) {};
    ZOFF;
    
}

void TIM1_CC_IRQHandler(void) 
{ 
    int a;
   bitTim[bitTimCount] = (a=TIM_GetCapture4(TIM1)) - bitTim[bitTimCount - 1];
    ++bitTimCount;
}

void delayMs(uint32_t ms)
{
    for(uint32_t i = 0; i<ms; i++){
        TIM_SetCounter(TIM3, 0);
        while((TIM_GetCounter(TIM1)) < 999);
    }
}
