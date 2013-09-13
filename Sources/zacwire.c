#include "main.h"
#include "init.h"
#include "zacwire.h"

#define ZON GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ZOFF GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define SET_LOW GPIO_ResetBits(GPIOA, GPIO_Pin_11)
#define SET_HIGH GPIO_SetBits(GPIOA, GPIO_Pin_11)
#define BUFF_SIZE 200
#define TIME_CMD 30
#define BUG_TIME_US 6
#define STARTUP_TIME 400

void setReadyToRising(void);
void setReadyToFalling(void);
void ZacReset(void);
void delayMs(uint32_t ms);
void ZacReadBitByTime(uint16_t pN);
uint8_t ZacConvUsToInt(uint16_t pN);
uint8_t convBuffToData(uint8_t pN, uint8_t* pdata0, uint8_t* pdata1, uint8_t* pdata2);
uint16_t convBuffSingle(uint8_t start, uint8_t stop);
uint8_t parityCheck(uint16_t data);
void ZacRead(uint8_t pN, uint8_t* pdata0, uint8_t* pdata1, uint8_t* pdata2);

void ZacWriteBit(uint8_t bit, uint16_t t);
void ZacWriteLastBit(uint8_t bit, uint16_t t);
void ZacWriteByte(uint8_t* byte);
uint8_t parityCalc(uint8_t* data);
void ZacQuickReset(void);
void ZacSendCmd(uint8_t* dataH, uint8_t* dataL, uint16_t t);
uint8_t ZacReadConfirm(void);
void hexToStr(uint8_t* b, uint8_t n);

uint8_t countTime;
uint32_t bitWindowTimeUs;
uint32_t bitLowTimeUs;
uint8_t buffReady;
uint32_t buff[BUFF_SIZE];
uint16_t buffRes[BUFF_SIZE];

uint8_t count;
uint8_t shortErr;
uint16_t maxT1;
double tooShort;
double shortCorr;
double shortUnCorr;
double tooLong;
double pec;
double recCount;
double toSendErr;

uint16_t max1;
uint16_t min1 = 999;
uint16_t max0;
uint16_t min0 = 999;
uint16_t maxS;
uint16_t minS = 999;


void ZacReadInfrequently(uint16_t* pBridgeDig, uint8_t* pTempDig, uint32_t delay_Ms) {
    
    while (1) {
    enterCmdMode();
    exitCmdToRaw();
    ZacReadBrTm(pBridgeDig, pTempDig);
    ZOFF;
    delayMs(delay_Ms);
    }
    
}

void ZacWriteDefaults(void)
{
    while (enterCmdMode()) {};
    //osc
    ZacSendHexCmd(0x30, 0x10);//1v trim
    ZacSendHexCmd(0x30, 0x23);//a2d
    ZacSendHexCmd(0x30, 0x30);//digital out
    ZacSendHexCmd(0x30, 0x42);//rate
    ZacSendHexCmd(0x30, 0x53);//jfet cfg
    ZacSendHexCmd(0x90, 0x98);//gain B low
    ZacSendHexCmd(0x80, 0x01);//gain B high
    ZacSendHexCmd(0xb0, 0x00);//offset b low
    ZacSendHexCmd(0xa0, 0x00);//high
    ZacSendHexCmd(0xc0, 0x80);//gain t
    ZacSendHexCmd(0xd0, 0x00);//offset t
    ZacSendHexCmd(0x70, 0x00);//tsetl
    ZacSendHexCmd(0xf0, 0x00);//t cg
    ZacSendHexCmd(0xe0, 0x00);//t co
    ZacSendHexCmd(0x30, 0x60);//tc cfg
    ZacSendHexCmd(0x60, 0x00);//sot
    ZacSendHexCmd(0x30, 0x75);//sot cfg, preamp
    ZacSendHexCmd(0x30, 0xe7);//diag cfg
    ZacSendHexCmd(0x30, 0xd0);//lock ext temp
    ZacSendHexCmd(0x08, 0x7f);//up clip lim
    ZacSendHexCmd(0x18, 0x00);//low
}

void ZacSetOutMode(uint8_t mode)
{
    enterCmdMode();
    ZacSendHexCmd(0x30, 0x30 + mode);
    exitCmdToNom();
    
}

void ZacSetDigitalOut(void)
{
    enterCmdMode();
    ZacSendHexCmd(0x30, 0x30);
    exitCmdToNom();
}

void ZacSetAnalogOut(void)
{
    enterCmdMode();
    ZacSendHexCmd(0x30, 0x31);
    exitCmdToNom();
}

//Необходим предварительный вход в командный режим
uint8_t setUpdateRate(uint8_t mode)
{
    uint8_t dh[9] = {0, 0, 1, 1, 0, 0, 0, 0};//30h
    uint8_t dl[9] = {0, 1, 0, 0, 0, 0, 0, 0};
    if ( mode == 0)     {
        dl[7] = 0;
        dl[8] = 0;
    }
    else if ( mode == 1)     {
        dl[7] = 0;
        dl[8] = 1;
    }
    else if ( mode == 2)     {
        dl[7] = 1;
        dl[8] = 0;
    }
    else if ( mode == 3)     {
        dl[7] = 1;
        dl[8] = 1;
    }        
    uint16_t t = TIME_CMD;
    ZacSendCmd(dh, dl, t);
    uint8_t err;
    err = ZacReadConfirm();
    return err;
}


//Необходим предварительный вход в командный режим
uint8_t ZacReadEeprom(void)
{
    uint8_t dh[9];
    uint8_t dl[9];
    uint16_t t = TIME_CMD;
    hexToStr(dh, 0x0);
    hexToStr(dl, 0x0);
    ZacSendCmd(dh, dl, t);
    NVIC_EnableIRQ (EXTI15_10_IRQn);
    count = 0;
    ZacReadBitByTime(20);
    if (!ZacConvUsToInt(20)) {
        return 0;
    }
    else return 1;  
    
}

uint8_t ZacSendHexCmd(uint8_t ih, uint8_t il)
{
    uint8_t dh[9];
    uint8_t dl[9];
    uint16_t t = TIME_CMD;
    hexToStr(dh, ih);
    hexToStr(dl, il);
    
    ZacSendCmd(dh, dl, t);
    uint8_t err;
    err = ZacReadConfirm();
    return err;
}

void exitCmdToRaw(void)
{
    ZacSendHexCmd(0x40, 0x10);
}

void exitCmdToNom(void)
{
    ZacSendHexCmd(0x40, 0x00);
}

void hexToStr(uint8_t* b, uint8_t n)
{
    int i;
    for (i=0; i<8; i++, n=n/2) {
        if (n%2) {
            b[7-i] = 1;
        }
        else {
            b[7-i] = 0;
        }
    }
    b[8] = '\0';
}

uint8_t enterCmdMode(void) 
{
    uint8_t dh[9];
    uint8_t dl[9];
    uint16_t t = TIME_CMD;
    hexToStr(dh, 0x50);
    hexToStr(dl, 0x90);
    
    ZacQuickReset();
    ZacSendCmd(dh, dl, t);
    //ZacSendHexCmd(0x30, 0x00); //если время бита 20 мкс принудительно вернуть на 40
    uint8_t err;
    err = ZacReadConfirm();
    return err;
}

uint8_t ZacSendCmdArr(uint8_t* dh, uint8_t* dl) 
{   
    uint16_t t = TIME_CMD;
    ZacSendCmd(dh, dl, t);
    uint8_t err;
    err = ZacReadConfirm();
    return err;
}
void ZacSendCmd(uint8_t* dataH, uint8_t* dataL, uint16_t t)
{
    gpioSend();
    ZacWriteBit('s', t);
    ZacWriteByte(dataH);
    ZacWriteBit(parityCalc(dataH), t);
    ZacWriteByte(dataL);
    ZacWriteLastBit(parityCalc(dataL), t);
    gpioRead(); 
}

void ZacWriteByte(uint8_t* byte)
{
    int i;
    for (i = 0; i < 8; i++) {
        ZacWriteBit(byte[i], TIME_CMD);
    }
}

void ZacWriteBit(uint8_t bit, uint16_t t)
{
    uint16_t time;
    if (bit == 1 || bit == '1') {
        time = (uint16_t)(0.25 * t);
    }
    else if (bit == 0 || bit == '0') {
        time = (uint16_t)(0.75 * t);
    }
    else if (bit == 's') {
        time = (uint16_t)(0.5 * t);
    }
    else {
        time = 0;
        toSendErr++;
    }
    
    TIM_SetCounter(TIM3, t);
    SET_LOW;
    while (TIM_GetCounter(TIM3) > t - time) {};
    SET_HIGH;
    while (TIM_GetCounter(TIM3) > 0) {};    
}

void ZacWriteLastBit(uint8_t bit, uint16_t t)
{
    uint16_t time;
    if (bit == 1 || bit == '1') {
        time = (uint16_t)(0.25 * t);
    }
    else if (bit == 0 || bit == '0') {
        time = (uint16_t)(0.75 * t);
    }
    else {
        time = 0;
        toSendErr++;
    }
    
    TIM_SetCounter(TIM3, t);
    SET_LOW;
    while (TIM_GetCounter(TIM3) > t - time) {};
    SET_HIGH;  
}


uint8_t ZacReadConfirm(void) 
{
    uint8_t data;
    ZacRead(1, &data, NULL, NULL);
    if (data != 0xA5) {
        return 1;
    }
    else {
        return 0;
    }
}


void ZacReadBrTm(uint16_t* pBridgeDig, uint8_t* pTempDig)
{
    uint8_t data0, data1, data2;
    uint16_t tmp;
        ZacRead(3, &data0, &data1, &data2);
        *pTempDig = data2;    
        *pBridgeDig = 0;
        tmp = 0;
        tmp |= (data0 << 8) | data1;
    *pBridgeDig = tmp;
}

void ZacRead(uint8_t pN, uint8_t* pdata0, uint8_t* pdata1, uint8_t* pdata2)
{
    NVIC_EnableIRQ (EXTI15_10_IRQn);
    count = 0;
    ZacReadBitByTime(pN);
    if (!ZacConvUsToInt(pN)) {
        convBuffToData(pN, pdata0, pdata1, pdata2);
    }  
}

uint8_t convBuffToData(uint8_t pN, uint8_t* pdata0, uint8_t* pdata1, uint8_t* pdata2) 
{
    uint8_t parityErr = 0;
    uint16_t data0, data1, data2;
    
    if (pN > 0) {
        data0 = convBuffSingle(1, 10);
        parityErr += parityCheck(data0);
        data0 >>= 1;
        *pdata0 = data0;
    }
    if (pN - 1 > 0 ) {
        data1 = convBuffSingle(11, 21);
        parityErr += parityCheck(data1);
        data1 >>= 1;
        *pdata1 = data1;
    }
    if (pN - 2 > 0) {
        data2 = convBuffSingle(21, 31);
        parityErr += parityCheck(data2);
        data2 >>= 1;
        *pdata2 = data2;
    }    
    return parityErr;
}

uint16_t convBuffSingle(uint8_t start, uint8_t stop)
{
    int i;
    uint16_t data = 0;
    
    for(i = start; i < stop; i++) {
        data |= buffRes[i] << (8 - i + start);
    }
    return data;
}

uint8_t parityCheck(uint16_t data)
{
    int i, res = 0;
    
    for (i = 1; i < 9; i++) {
        res += (data >> i) & 1;
    }
    int p;
    p = (res % 2);
    if (p != (data & 1)) {
        pec++;
        return 1;
    }
    else {
        return 0;
    }
}

uint8_t parityCalc(uint8_t* data)
{
    int i, res = 0;
    for (i = 0; i < 8; i++) {
        res += data[i];
    }
    int p;
    p = (res % 2);
    return p;
}

uint8_t ZacConvUsToInt(uint16_t pN)
{
    uint8_t reciveErr = 0;
    uint16_t bitTime;
    for (count = 0; count < pN * 10; count++) {
        recCount++;
        
        if (count % 10 == 0 ) {
            bitWindowTimeUs = buff[0] * 2;
            bitTime = bitWindowTimeUs;
            buffRes[count] = 's';
        }
        else if (buff[count] < 0.11 * bitTime) {
            tooShort++;
        }
        else if (0.11 * bitTime < buff[count] && buff[count] <= 0.36 * bitTime) {
            buffRes[count] = 1;
            max1 = (max1 > buff[count]) ? max1 : buff[count];
            min1 = (min1 < buff[count]) ? min1 : buff[count];
        } 
        else if (0.36 * bitTime < buff[count] && buff[count] <= 0.61 * bitTime) {
            buffRes[count] = 'd';
        }
        else if (0.61 * bitTime < buff[count] && buff[count] <= bitTime) {
            buffRes[count] = 0;
        }
        else if (buff[count] > bitTime) {
            tooLong++;
        }
    }
    return reciveErr;
}


void ZacReset(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_0);
    delayMs(3);
    GPIO_SetBits(GPIOB, GPIO_Pin_0);
    delayMs(3);
}

void ZacQuickReset(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_0);
    delayMs(3);
    GPIO_SetBits(GPIOB, GPIO_Pin_0);
    TIM_SetCounter(TIM2, 0);
    while((TIM_GetCounter(TIM2)) < STARTUP_TIME);
}
void ZacReadBitByTime(uint16_t pN)
{
    uint16_t t1;
    while (count < pN * 10) {
        setReadyToFalling();
        countTime = 1;
        while (countTime != 0) {};
        if ((t1 = bitLowTimeUs) < BUG_TIME_US) {
            countTime = 2;
            while (countTime != 0) {};
            if (bitLowTimeUs < BUG_TIME_US) {
                shortUnCorr++;
            }
            else shortCorr++;
            maxT1 = (t1 > maxT1) ? t1 : maxT1; 
        }
        buff[count] = bitLowTimeUs;
        count++;
    }
}

void doCountTime(void)
{
    if (countTime == 2) {
        bitLowTimeUs = (TIM_GetCounter(TIM2));
        countTime = 0;
        return;
    }
    else
        if (countTime == 1) {
            TIM_SetCounter(TIM2, 0);
            setReadyToRising();
            countTime = 2; 
            return;
        }
}

void EXTI15_10_IRQHandler(void)
{
    EXTI_ClearFlag(EXTI_Line11);
    doCountTime();
}

void setReadyToRising(void)
{
    EXTI_DeInit();
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void setReadyToFalling(void)
{
    EXTI_DeInit();
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void delayMs(uint32_t ms)
{
    for(int i = 0;i<ms;i++){
        TIM_SetCounter(TIM2, 0);
        while((TIM_GetCounter(TIM2)) < 999);
    }
}