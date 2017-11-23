#include "app.h"
#include <stdint.h> 
#include <stdlib.h>
#include <stdio.h>
#include "utilities.h"

static unsigned int x = 0;
static unsigned int y = 0;
static unsigned char c = 0;
unsigned short motors[4];
unsigned int TYPR_control [4] = {0, YAW_MID_VAL, PITCH_MID_VAL, ROLL_MID_VAL};
int TYPR_PWM [4] = {0, 0, 0, 0};
float TYPR_lecture [4] = {0, 0, 0, 0};
float TYPR_target [4] = {0, 0, 0, 0};
unsigned char IMUYPR[3][8]={{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}, 
                            {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                            {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}};
unsigned char TYPR[4][8]={{'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'},
                          {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'}};
APP_DATA appData;

void readIMU();
bool parseValue();
void readClient();
void setMotors();
void mapAppData();

bool initMotors();
bool initClient();

bool serverUp();
bool clientPresent();

float scaleValue(int unscaledVal, int middle, int numValuesLeft, int numValuesRight);
void APP_Initialize ( void ){
    initMotors();
    appData.state = APP_STATE_INIT;
    appData.initInfo.initClientState = WAIT_SERVER;
}

void APP_Tasks ( void ){
    switch ( appData.state ){
        case APP_STATE_INIT :{
            if (initClient()){
                appData.state = APP_STATE_READ_IMU;
            }
        }
        break;

        case APP_STATE_READ_IMU :
            readIMU();
            appData.state = APP_STATE_READ_WIFI;
            break;
        
        case APP_STATE_READ_WIFI : {
            readClient();
            appData.state = APP_STATE_MAP_APP_DATA;
            break;
        }
        case APP_STATE_MAP_APP_DATA:{
            mapAppData();
            appData.state = APP_STATE_MODULE_PWM;
            break;
        }
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
        case WAIT_IMU_TX_START : {
            if (!DRV_USART1_ReceiverBufferIsEmpty()) {
              appData.readIMUState = WAIT_EQUAL;
            }
            break;
        }
        case WAIT_EQUAL:
            if (!DRV_USART1_ReceiverBufferIsEmpty()) {
                readCode = DRV_USART1_ReadByte();
                if (readCode == '=') {
                    appData.readIMUState = PARSE_VALUE;
                    DRV_USART0_WriteByte(readCode);
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
    for (i = 0; i < 4; i++){
        itoa(val[i],TYPR_control[i],10);
        for (j = 0; j < 3 && val[i][j] != '\0'; j++){
            DRV_USART0_WriteByte(val[i][j]);
        }
        if (i < 3){
            DRV_USART0_WriteByte(',');
        }
    }
    DRV_USART0_WriteByte('\n');
}

void printMotorsPwm() {
    int i;
    Serial0Print("Motors PWM:  ");
    for (i = 0; i < 4; i++){
        Serial0PrintInt(motors[i]);
        if (i < 3)
            Serial0Print(", ");
    }
    Serial0Print("\n");
}

void printMappedData() {
    Serial0Print("Mapped Throttle: ");
    Serial0PrintInt(TYPR_PWM[Throttle]);
    Serial0Print("\nMapped Yaw: ");
    Serial0PrintInt(TYPR_PWM[Yaw]);
    Serial0Print("\nMapped Pitch: ");
    Serial0PrintInt(TYPR_PWM[Pitch]);
    Serial0Print("\nMapped Roll: ");
    Serial0PrintInt(TYPR_PWM[Roll]);
    Serial0Print("\n");
}

/*
    This FSM expects an string in a format like this:
    "(*)=(valor de Throttle)(valor de Yaw)(valor de Pitch)(valor de Roll)"
*/
void readClient() {
    char receiveCode;
    switch(appData.readClientState) {
        case WAIT_CLIENT_TX_START:
            if (!DRV_USART0_ReceiverBufferIsEmpty()) {
                receiveCode = DRV_USART0_ReadByte();
                if (receiveCode == '='){
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
                //printMappedData();
                //printMotorsPwm();
                // After the ESP8266 module finishes initialization for some 
                // reason is sending some data that is enabling this FSM. 
                // In a future release is expected to correct his problem in 
                // a formal way. For now, I will set a counter to ignore the first
                // 10 values. The comparison to determine if the samples should 
                // be ignored is done in setMotors() function.
                c++;
            }
            break;
        default :
            appData.readClientState = WAIT_CLIENT_TX_START;
            break;
    }
}

void setMotors() {
    // In a future release this part should dissapear. Check read client FSM 
    // to know about more details in this part.
    if (c < 10){
        DRV_OC0_Width(0);
        DRV_OC1_Width(0); 
        DRV_OC2_Width(0);
        DRV_OC3_Width(0);
        return;
    }
    c = 100;
    motors[0] = TYPR_PWM [Throttle] + TYPR_PWM [Yaw] + TYPR_PWM [Pitch] + TYPR_PWM [Roll];
    motors[1] = TYPR_PWM [Throttle] - TYPR_PWM [Yaw] + TYPR_PWM [Pitch] - TYPR_PWM [Roll];
    motors[2] = TYPR_PWM [Throttle] + TYPR_PWM [Yaw] - TYPR_PWM [Pitch] + TYPR_PWM [Roll];
    motors[3] = TYPR_PWM [Throttle] - TYPR_PWM [Yaw] - TYPR_PWM [Pitch] - TYPR_PWM [Roll];
    DRV_OC0_Width(motors[3] + 12);
    DRV_OC1_Width(motors[2] + 12); 
    DRV_OC2_Width((motors[0] - 12) > 0 ? motors[0] - 12 : motors[0]);
    DRV_OC3_Width((motors[1] - 12) > 0 ? motors[1] - 12 : motors[1]);
}

void mapAppData() {
     TYPR_PWM [Throttle] = THROTTLE_PROPORTION * scaleValue(TYPR_control[Throttle]-3, 0, 1, MAX_THROTTLE);
     TYPR_PWM[Yaw] = YAW_PROPORTION * scaleValue(TYPR_control[Yaw], YAW_MID_VAL, YAW_NUM_VALS_LEFT, YAW_NUM_VALS_RIGHT);
     TYPR_PWM[Pitch] = PITCH_PROPORTION * scaleValue(TYPR_control[Pitch], PITCH_MID_VAL, PITCH_NUM_VALS_LEFT, PITCH_NUM_VALS_RIGHT);
     TYPR_PWM[Roll] = ROLL_PROPORTION * scaleValue(TYPR_control[Roll], ROLL_MID_VAL, ROLL_NUM_VALS_LEFT, ROLL_NUM_VALS_RIGHT);
 }

bool initMotors(){
    bool res = true;
    DRV_TMR0_Start(); // Start timer 2
    DRV_OC0_Start (); // Start motor 1 PWM
    DRV_OC1_Start (); // Start motor 2 PWM
    DRV_OC2_Start (); // Start motor 3 PWM
    DRV_OC3_Start (); // Start motor 4 PWM
    // Motors should always being off
    DRV_OC0_Width (0); 
    DRV_OC1_Width (0);
    DRV_OC2_Width (0); 
    DRV_OC3_Width (0); 
    return res;
}

/*
 * So far, it's been decided the microDrone won't fly unless there is some client 
 * app requesting it. Even for automatic mode, client will decide when to 
 * start operating in automatic mode.
 */
bool initClient(){
    bool res = false;
    switch (appData.initInfo.initClientState) {
        case WAIT_SERVER: {
            if (DRV_USART0_ReceiverBufferIsEmpty()) {
                res = true;
                appData.initInfo.initClientState = WAIT_CLIENT;
            }
            else {
                DRV_USART0_ReadByte();
            }
            break;
        }
        case WAIT_CLIENT: {
            res = true;
            break;
        }
    }
    return res;
}

float scaleValue(int unscaledVal, int middle, int numValuesLeft, int numValuesRight) {
    float res = 0;
     if (unscaledVal < middle ){
       res = (float)(unscaledVal - middle) / numValuesLeft;
    }
     else if (unscaledVal > middle ){
       res = (float)(unscaledVal - middle) / numValuesRight;
    }   
    return res;
}