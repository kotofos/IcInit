#include "main.h"
#include "zacwire.h"
#include "init.h"
#include "stat.h"

#define ZON GPIO_SetBits(GPIOB, GPIO_Pin_0)
#define ZOFF GPIO_ResetBits(GPIOB, GPIO_Pin_0)
#define RED_CND 1
#define AVG_BUF 2000
#define AVG_BUF_PRE 10
#define OFFSET 0

void ZacReadBrTemp(uint8_t rate, uint16_t samplesCount);

uint16_t bridgeDigital;
uint8_t temp;
uint8_t tmp;
uint16_t brMid;
double height;
static uint8_t err;
static uint32_t retries;
double tempCorr;
float tmpstd;
float tmpstdA;
double rate0;
 Statistics stats(10000);
 Statistics statsAvg(1000);
double minH = 99999;
double maxH = -10000;

#define MODE 4
int main()
{
    
    SystemInit();
    initTim();
    initGpio();
#if MODE == 0
    ZacReadInfrequently(&bridgeDigital, &temp, 1000);
#elif MODE == 1
    ZacReset();
    while (1) {
        ZacReadBrTm(&bridgeDigital, &temp);
    }
#elif MODE == 2
    ZacReadBrTemp(0, 200);
#elif MODE == 3
    ZacSetOutMode(Out_Mode_Dig);
#elif MODE == 4
    
    while (enterCmdMode()) {
        retries++;
    } 
    //ZacSendHexCmd(0xd0, 0x00);//offset t
    ZacSendHexCmd(0x90, 0x00);//gain B low
    ZacSendHexCmd(0x80, 0x0e);//gain B high
    setUpdateRate(3);
    exitCmdToNom();
    //exitCmdToRaw();
    //ZacReadEeprom();
    ZacReadBrTemp(3, 100);
#elif MODE == 5
    ZacWriteDefaults();
#elif !defined(MODE)
    ZacReset();
    while (1) {
        ZacReadBrTm(&bridgeDigital, &temp);
    }
#endif
}


//ZacSendHexCmd(0x30, 0x2f);//a2d
//ZacSendHexCmd(0x30, 0x77);//sot cfg, preamp
//ZacSendHexCmd(0x90, 0xff);//gain B low
//ZacSendHexCmd(0x80, 0x0e);//gain B high
//    ZacSendHexCmd(0x30, 0x40);//rate

void ZacReadBrTemp(uint8_t rate, uint16_t samplesCount)
{
    //    while (enterCmdMode()) {
    //        retries++;
    //    }
    //    setUpdateRate(rate);
    //    
    //    //while (ZacSendHexCmd(0x80, 0x0a)) {};
    //    
    //    exitCmdToNom();
    
    int i, j, k;
    uint64_t tmpTempMid;
    static uint16_t window[AVG_BUF] = {0};
    static uint16_t pre_avg[AVG_BUF_PRE];
    uint64_t preMid;
    uint8_t fFull = 0;
    uint16_t zeroLvl = 15516;
    ZacReadBrTm(&bridgeDigital, &temp);
    ZacReadBrTm(&bridgeDigital, &temp);
    while (1) {
        for (i = 0; i < samplesCount; i++) {
            for (k = 0; k < AVG_BUF_PRE; k++) {           
                ZacReadBrTm(&bridgeDigital, &temp);
                tempCorr = (temp - 86.28) * 0.90256;//gain = 0.5
                stats.addData(bridgeDigital);
                pre_avg[k] = bridgeDigital;
            }
            
            float mean = stats.mean();
            float dev = stats.stdDeviation();
            tmpstd = dev / mean * 100;
            preMid = 0;
            
            for (k = 0; k < AVG_BUF_PRE; k++) {
                preMid += pre_avg[k];
            }
            window[i] = preMid / AVG_BUF_PRE;
            tmpTempMid = 0;
            if (fFull == 1) {
                for (j = 0; j< samplesCount; j++) {
                    tmpTempMid += window[j];
                }
                tmpTempMid = tmpTempMid / samplesCount / RED_CND;
            }
            
            else {
                for (j = 0; j< i + 1; j++) {
                    tmpTempMid += window[j];
                }
                tmpTempMid = tmpTempMid / (i + 1) / RED_CND;
            }
            
            brMid = tmpTempMid;
            statsAvg.addData(brMid);
            float meanA = statsAvg.mean();
            float devA = statsAvg.stdDeviation();
            tmpstdA = devA / meanA * 100;
            rate0 = tmpstd / tmpstdA;
            height = (brMid - zeroLvl) * 1/*.52*/;
            maxH = (height > maxH) ? height : maxH;
            minH = (height < minH) ? height : minH;
        }
        if (fFull == 0) {
            fFull = 1;
        }
        
    }
    
}