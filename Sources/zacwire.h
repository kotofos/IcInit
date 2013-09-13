#ifdef __cplusplus
extern "C" {
#endif
void ZacReadBrTm(uint16_t* pBridgeDig, uint8_t* pTempDig);
uint8_t enterCmdMode(void); 
void ZacReset(void);
void exitCmdToRaw(void);
uint8_t ZacSendHexCmd(uint8_t ih, uint8_t il);
uint8_t ZacSendCmdArr(uint8_t* dh, uint8_t* dl) ;
void delayMs(uint32_t ms);
void exitCmdToNom(void);
uint8_t ZacReadEeprom(void);
uint8_t setUpdateRate(uint8_t mode);
void ZacSetOutMode(uint8_t mode);
void ZacWriteDefaults(void);
void ZacReadInfrequently(uint16_t* pBridgeDig, uint8_t* pTempDig, uint32_t delay_Ms);


enum out_mode {
    Out_Mode_Dig_Dual = 0,
    Out_Mode_Analog,
    Out_Mode_RTRR,
    Out_Mode_Dig
};
#ifdef __cplusplus
}
#endif