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
unsigned int TYPR_control [4] = {0, 0, 0, 0};
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
APP_DATA appData;

void readIMU();
bool parseValue();
void readClient();
void setMotors();

void APP_Initialize ( void ){
    DRV_TMR0_Start(); // Start timer 2
    DRV_OC0_Start (); // Start motor 1 PWM
    DRV_OC1_Start (); // Start motor 2 PWM
    DRV_OC2_Start (); // Start motor 3 PWM
    DRV_OC3_Start (); // Start motor 4 PWM
    // All motors will be set with a 100% Duty Cycle
    DRV_OC0_Width (400); 
    DRV_OC1_Width (400);
    DRV_OC2_Width (400); 
    DRV_OC3_Width (400); 
    appData.state = APP_STATE_INIT;
}

void APP_Tasks ( void ){
    switch ( appData.state ){
        case APP_STATE_INIT :{
            bool appInitialized = true;
            if (appInitialized){
                appData.state = APP_STATE_READ_IMU;
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
    char receiveCode;
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
    }
}

void setMotors() {
    TYPR_PWM [Throttle] = TYPR_control [Throttle]*400/255;
    TYPR_PWM [Yaw] = TYPR_control [Yaw]*400/255; 
    TYPR_PWM [Pitch] = TYPR_control [Pitch]*400/255;
    TYPR_PWM [Roll] = TYPR_control [Roll]*400/255;
    DRV_OC0_Width(TYPR_PWM [Throttle]);
    DRV_OC1_Width(TYPR_PWM [Yaw]); 
    DRV_OC2_Width(TYPR_PWM [Pitch]);
    DRV_OC3_Width(TYPR_PWM [Roll]); 
}
