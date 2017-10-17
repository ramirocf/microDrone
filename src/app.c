#include "app.h"
#include <stdint.h> 
#include <stdlib.h>
#include <stdio.h>

#define Throttle 0
#define Yaw 1
#define Pitch 2
#define Roll 3

static unsigned int x = 0;
static unsigned int y = 0;
unsigned int TYPR_control [11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned int TYPR_PWM [4] = {0, 0, 0, 0};
float TYPR_lecture [4] = {0, 0, 0, 0};
float TYPR_target [4] = {0, 0, 0, 0};
unsigned char IMUYPR[3][8]={{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}, 
                            {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                            {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}};
unsigned char TYPR[4][8]={{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}};
unsigned char readcode3 [4] = {'\0', '\0', '\0', '\0'};
unsigned char readcode4 = 0;

APP_DATA appData;

void readIMU();
bool parseValue();
void readClient();
void setMotors();

void APP_Initialize ( void ){
    DRV_TMR0_Start(); // Start timer 2
    DRV_TMR1_Start();
    DRV_OC0_Start (); // Start motor 1 PWM
    DRV_OC1_Start (); // Start motor 2 PWM
    DRV_OC2_Start (); // Start motor 3 PWM
    DRV_OC3_Start (); // Start motor 4 PWM
    // All motors will be set with a 100% Duty Cycle
    DRV_OC0_PulseWidthSet(400); 
    DRV_OC1_PulseWidthSet(400); 
    DRV_OC2_PulseWidthSet(400);
    DRV_OC3_PulseWidthSet(400);
    appData.state = APP_STATE_INIT;
}

void APP_Tasks ( void ){
    switch ( appData.state ){
        case APP_STATE_INIT :{
            bool appInitialized = true;
            if (appInitialized){
                //appData.state = APP_STATE_READ_IMU;
                appData.state = APP_STATE_READ_WIFI;
            }
            while (DRV_USART0_ReceiverBufferIsEmpty()){
                
            }
            readcode4 = 0;
            while (!DRV_USART0_ReceiverBufferIsEmpty()){
                readcode4 = DRV_USART0_ReadByte();
                DRV_USART2_WriteByte(readcode4);
            }
            
        }
        break;

        case APP_STATE_READ_IMU :
            readIMU();
            appData.state = APP_STATE_READ_WIFI;
            break;
        
        case APP_STATE_READ_WIFI :
            readClient();
            appData.state = APP_STATE_MODULE_PWM;
            break;
        
        case APP_STATE_MODULE_PWM:{
            setMotors();
            appData.state = APP_STATE_READ_IMU;
        }
        break;
        
        default:{
        /* TODO: Handle error in application's state machine. */
        }
        break;
    }
}

void readIMU() {
    char readCode;
    switch (appData.readIMUState) {
        case WAIT_IMU_TX_START : 
            if (DRV_USART1_ReceiverBufferIsEmpty()) {
                //DRV_USART0_WriteByte('Y');
            }
            else {
                appData.readIMUState = WAIT_EQUAL;
            }
            break;
        case WAIT_EQUAL:
            if (!DRV_USART1_ReceiverBufferIsEmpty()) {
                readCode = DRV_USART1_ReadByte();
                if (readCode == '=') {
                    appData.readIMUState = PARSE_VALUE;
                }
            }
            break;
        case PARSE_VALUE:
            if (parseValue()) {
                char *p = &IMUYPR[0][0];
                TYPR_lecture [Yaw] = (float)atof(p); 
                p +=8;
                TYPR_lecture [Pitch] = (float)atof(p);
                p +=8;
                TYPR_lecture [Roll] = (float)atof(p);
                appData.readIMUState = WAIT_IMU_TX_START;                
            }
            break;
    }
}

bool parseValue() {
    char readCode;
    bool retVal = false;
    if (!DRV_USART1_ReceiverBufferIsEmpty()){
        readCode = DRV_USART1_ReadByte();
        DRV_USART0_WriteByte(readCode);
        if (readCode == '\n' ){  
            IMUYPR[y][x] = '\0';
            retVal = true;
        }
        else if (readCode == ',' ){
            IMUYPR[y][x] = '\0';
            x = 0;
            y++;
        } 
        else{
            IMUYPR[y][x] = readCode;
            x++;
        }
    }
    return retVal;
}

void sendClientData(){
    char val[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
    DRV_USART0_WriteByte('Y');
    DRV_USART0_WriteByte('P');
    DRV_USART0_WriteByte('R');
    DRV_USART0_WriteByte('=');
    int i,j;
    for (i = 0; i < 3; i++){
        itoa(val[i],TYPR_control[i],10);
        for (j = 0; j < 3 && val[i][j] != '\0'; j++){
            DRV_USART0_WriteByte(val[i][j]);
        }
        if (i < 2){
            DRV_USART0_WriteByte(',');
        }
    }
    DRV_USART0_WriteByte('\n');
}

/*
    This FSM expects an string in a format like this:
    "\n(valor de Throttle)(valor de Yaw)(valor de Pitch)(valor de Roll)"
*/
void readClient() {
    /*char receiveCode;
    switch(appData.readClientState) {
        case WAIT_CLIENT_TX_START:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                receiveCode = DRV_USART0_ReadByte();
                if (receiveCode == '\n'){
                    appData.readClientState = GET_THROTTLE;
                }
            }
            break;
        case GET_THROTTLE:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                TYPR_control[Throttle] = DRV_USART0_ReadByte();
                appData.readClientState = GET_YAW;
            }
            break;
        case GET_YAW:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                TYPR_control[Yaw] = DRV_USART0_ReadByte();
                appData.readClientState = GET_PITCH;
            }
            break;
        case GET_PITCH:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                TYPR_control[Pitch] = DRV_USART0_ReadByte();
                appData.readClientState = GET_ROLL;
            }
            break;
        case GET_ROLL:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                TYPR_control[Roll] = DRV_USART0_ReadByte();
                appData.readClientState = WAIT_CLIENT_TX_START;
                // Just for debugging
                //sendClientData();
            }
            break;
        default :
            appData.readClientState = WAIT_CLIENT_TX_START;
            break;    
    }*/
    int i = 0;
    for (i=0; i <= 3; i++){
        readcode3[i] = '\0';
    }
    
    while (readcode3[0] != '#' && ((readcode3[1] != 'A' && readcode3[2] != 'P') || (readcode3[1] != 'R' && readcode3[2] != 'C')) && readcode3[3] != '='){
        for (i=0; i <= 3; i++){
            while (DRV_USART0_ReceiverBufferIsEmpty()){
                
            }
            readcode3[i] = DRV_USART0_ReadByte();
            DRV_USART2_WriteByte(readcode3[i]);
        }
    }
        
    
    for (i = 0; i <= 10; i++){
        while (DRV_USART0_ReceiverBufferIsEmpty()){
            
        }
        TYPR_control[i] = DRV_USART0_ReadByte();
        DRV_USART2_WriteByte(TYPR_control[i]);
    }
    /*readcode3[2] = 0;
    while (!DRV_USART0_ReceiverBufferIsEmpty()){
        readcode3 = DRV_USART0_ReadByte();
        DRV_USART2_WriteByte(readcode3);
    }*/
    //appData.state = APP_STATE_READ_WIFI;
    appData.state = APP_STATE_MODULE_PWM;
}

void setMotors() {
    TYPR_PWM [Throttle] = TYPR_control [Throttle]*510/162;
    if (TYPR_control [Yaw] < 89 ){
        TYPR_PWM [Yaw] = (TYPR_control [Yaw] - 89)*30/88;
    }
    else{
        if (TYPR_control [Yaw] > 89 ){
            TYPR_PWM [Yaw] = (TYPR_control [Yaw] - 89)*30/83;
        }
        else{
            TYPR_PWM [Yaw] = 0;
        }
    }
    //TYPR_PWM [Yaw] =  TYPR_control [Yaw]*30/255;
    if (TYPR_control [Pitch] < 83 ){
        TYPR_PWM [Pitch] = (TYPR_control [Pitch] - 83)*30/83;
    }
    else{
        if (TYPR_control [Pitch] > 83 ){
            TYPR_PWM [Pitch] = (TYPR_control [Pitch]-83)*30/77;
        }
        else{
            TYPR_PWM [Pitch] = 0;
        }
    }
    //TYPR_PWM [Pitch] = TYPR_control [Pitch]*30/255;
    if (TYPR_control [Roll] < 89 ){
        TYPR_PWM [Roll] = (TYPR_control [Roll] - 89)*30/88;
    }
    else{
        if (TYPR_control [Roll] > 83 ){
            TYPR_PWM [Roll] = (TYPR_control [Roll]-89)*30/85;
        }
        else{
            TYPR_PWM [Roll] = 0;
        }
    }
    //TYPR_PWM [Roll] = TYPR_control [Roll]*30/255;
    DRV_OC0_PulseWidthSet(TYPR_PWM [Throttle] + TYPR_PWM [Yaw] + TYPR_PWM [Pitch] + TYPR_PWM [Roll]);
    DRV_OC1_PulseWidthSet(TYPR_PWM [Throttle] - TYPR_PWM [Yaw] + TYPR_PWM [Pitch] - TYPR_PWM [Roll]); 
    DRV_OC2_PulseWidthSet(TYPR_PWM [Throttle] + TYPR_PWM [Yaw] - TYPR_PWM [Pitch] + TYPR_PWM [Roll]);
    DRV_OC3_PulseWidthSet(TYPR_PWM [Throttle] - TYPR_PWM [Yaw] - TYPR_PWM [Pitch] - TYPR_PWM [Roll]); 
}
