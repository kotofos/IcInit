#include "stm32f10x.h"

#define ZON GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ZOFF GPIO_ResetBits(GPIOB, GPIO_Pin_0)

uint16_t period, capture_1, capture_2;
uint8_t n = 0x00;

void test(void) {
    return;
}
void setEdge(uint16_t pol);
void delayMs(uint32_t ms, TIM_TypeDef* TIMx);
void initAll(void);
uint32_t bitTim[50], bitTimCount = 0;
int main()
{    
    initAll();
    ZON;
    delayMs(3, TIM3);
    

    TIM_ITConfig(TIM1, TIM_EventSource_CC4, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
       int a = TIM_GetITStatus(TIM1, TIM_IT_Update);//флаг сброшен, но прерыванеи происходит
       ZOFF;
    NVIC_EnableIRQ(TIM1_CC_IRQn);//сразу же после разрешения происходит прерывание
    delayMs(1, TIM3);
    NVIC_DisableIRQ(TIM1_CC_IRQn);
    delayMs(1, TIM3);
    NVIC_EnableIRQ(TIM1_CC_IRQn);
    
    TIM_SetCounter(TIM1, 0);
    while (bitTimCount < 29) {};
    NVIC_DisableIRQ(TIM1_CC_IRQn);
    delayMs(3, TIM3);
    NVIC_EnableIRQ(TIM1_CC_IRQn);
    
    ZOFF;
}

void TIM1_CC_IRQHandler(void) 
{ 
    GPIO_SetBits(GPIOA, GPIO_Pin_8);
    int a = TIM_GetITStatus(TIM1, TIM_IT_Update);
    if(!n) {
        capture_1 = TIM1->CCR4;
        n = ~n;
        setEdge(TIM_ICPolarity_Rising);
    }
    else
    {
        capture_2 = TIM1->CCR4;
        period = capture_2 - capture_1;
        n = ~n;
        bitTim[bitTimCount] = period;
        ++bitTimCount;
        setEdge(TIM_ICPolarity_Falling);
        
    }
    GPIO_ResetBits(GPIOA, GPIO_Pin_8);
    
}

void delayMs(uint32_t ms, TIM_TypeDef* TIMx)
{
    for(uint32_t i = 0; i<ms; i++){
        TIM_SetCounter(TIMx, 0);
        while((TIM_GetCounter(TIMx)) < 999);
    }
}

void setEdge(uint16_t pol)
{
    TIM_ICInitTypeDef TIM_ICInitStructure;
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICFilter = 0xf;
    TIM_ICInitStructure.TIM_ICPolarity = pol;
    TIM_ICInitStructure.TIM_ICPrescaler = 0x0;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; 
    TIM_ICInit(TIM1, &TIM_ICInitStructure);
}

void initAll(void)
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
    
    //debug
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init (GPIOA, &GPIO_InitStructure);
    
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
    
    setEdge(TIM_ICPolarity_Falling);
    
    
    //tim3 for delay
    RCC_APB1PeriphClockCmd((RCC_APB1Periph_TIM3), ENABLE);
    //TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseStructInit(&TIM_TimeBaseInitStructure);
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; //1 us
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 0xffff;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStructure);
    TIM_Cmd(TIM3, ENABLE);
}