#include "app.h"
#include <stdint.h> 
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>

#define Throttle 0
#define Yaw 1
#define Pitch 2
#define Roll 3

unsigned char readcode1;
unsigned char readcode2;
static unsigned int x = 0;
static unsigned int y = 0;
unsigned int a = 200;
unsigned int b = 200;
unsigned int c = 200;
unsigned int d = 200; 
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
char *p = &IMUYPR[0][0];
APP_DATA appData;

void APP_Initialize ( void ){
    while (delay <= 400){
        delay++;
    }
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
    /* Check the application's current state. */
    switch ( appData.state ){
        /* Application's initial state. */
        case APP_STATE_INIT :{
            bool appInitialized = true;
            if (appInitialized){
                appData.state = APP_STATE_READ_IMU;
            }
        }
        break;

        case APP_STATE_READ_IMU :{
            // Before receiving data, we need to ask the IMU for a sample
            while (DRV_USART1_ReceiverBufferIsEmpty()){
                DRV_USART1_WriteByte('Y');
                //DRV_USART0_WriteByte('Y');
                /*i = 1000;
                while(i) {
                    i--;
                }*/
            }
            // IMU Reading
            // IMU sends Yaw, Pitch and Roll in the format:
            // #YPR=sign?num?num(\.num num?)?\,
            // sign?num?num(\.num num?)?\,
            // sign?num?num(\.num num?)?\n
            // Instead of parsin the whole #YPR= string we will only wait 
            // for the equals sign
            readcode1 = 0;
            //DRV_USART0_WriteByte('e');
            while (readcode1 != '='){
                while(DRV_USART1_ReceiverBufferIsEmpty()){} 
                readcode1 = DRV_USART1_ReadByte();
                if(readcode1 != NULL){
                    DRV_USART0_WriteByte(readcode1);
                }
            }
            x = 0;
            y = 0;
            //DRV_USART0_WriteByte('s');
            while (readcode1 != '\n'){
                while(DRV_USART1_ReceiverBufferIsEmpty()){}
                readcode1 = DRV_USART1_ReadByte();
                if(readcode1 != NULL){
                    if (readcode1 == '\n' ){  
                        DRV_USART0_WriteByte(readcode1);
                        IMUYPR[y][x] = '\0';
                    }
                    else{
                        if (readcode1 == ',' ){
                            DRV_USART0_WriteByte(readcode1);
                            IMUYPR[y][x] = '\0';
                            x = 0;
                            y++;
                        } 
                        else{
                            IMUYPR[y][x] = readcode1;
                            DRV_USART0_WriteByte(readcode1);
                            x++;
                        }
                    }
                }
            }
            p = &IMUYPR[0][0];
            TYPR_lecture [Yaw] = (float)atof(p); 
            p +=8;
            TYPR_lecture [Pitch] = (float)atof(p);
            p +=8;
            TYPR_lecture [Roll] = (float)atof(p);
            /*i = 1000;
            while(i) {
                i--;
            }
            while(!DRV_USART1_ReceiverBufferIsEmpty()) {
               DRV_USART0_WriteByte('T');
                DRV_USART1_ReadByte();
            }*/
            appData.state = APP_STATE_READ_WIFI;
        }
        break;
        
        case APP_STATE_READ_WIFI :{
            readcode2 = 0b10000000;
            // El control envía el movimiento a seguir con el formato
            // '\n'(valor de Throttle)(valor de Yaw)(valor de Pitch)(valor de Roll)
            while (readcode2 != '\n'){ //Espera recibir el salto de línea del contol, los valores de los potenciómetros no llegan a valores tan bajos
                while(DRV_USART0_ReceiverBufferIsEmpty()){} // Espera revibir algún dato del control
                if(DRV_USART0_ReadByte() != NULL){
                    readcode2 = DRV_USART0_ReadByte(); // Almacena el valor obtenido de la lectura en la variable readcode2
                }
            }
            while(DRV_USART0_ReceiverBufferIsEmpty()){} // Espera revibir algún dato del control
            TYPR_control [Throttle] = DRV_USART0_ReadByte(); // Almacena el dato #1 después del salto de línea en una matriz
            while(DRV_USART0_ReceiverBufferIsEmpty()){} // Espera revibir algún dato del control
            TYPR_control [Yaw] = DRV_USART0_ReadByte(); // Almacena el dato #2 después del salto de línea en una matriz
            while(DRV_USART0_ReceiverBufferIsEmpty()){} // Espera revibir algún dato del control
            TYPR_control [Pitch] = DRV_USART0_ReadByte(); // Almacena el dato #3 después del salto de línea en una matriz
            while(DRV_USART0_ReceiverBufferIsEmpty()){} // Espera revibir algún dato del control
            TYPR_control [Roll] = DRV_USART0_ReadByte(); // Almacena el dato #4 después del salto de línea en una matriz
            TYPR_PWM [Throttle] = TYPR_control [Throttle]*400/255; // Convierte el valor de un máximo de 255 a uno de 400
            TYPR_PWM [Yaw] = TYPR_control [Yaw]*400/255; // Convierte el valor de un máximo de 255 a uno de 400
            TYPR_PWM [Pitch] = TYPR_control [Pitch]*400/255; // Convierte el valor de un máximo de 255 a uno de 400
            TYPR_PWM [Roll] = TYPR_control [Roll]*400/255; // Convierte el valor de un máximo de 255 a uno de 400
            appData.state = APP_STATE_MODULE_PWM; // Cambia el status de la máquina de estados para modificar el PWM de los motores
        }
        break;
        
        case APP_STATE_MODULE_PWM:{
            DRV_OC0_Width (TYPR_PWM [Throttle]); // Modifica el valor del PWM en el motor 1
            DRV_OC1_Width (TYPR_PWM [Yaw]); // Modifica el valor del PWM en el motor 2
            DRV_OC2_Width (TYPR_PWM [Pitch]); // Modifica el valor del PWM en el motor 3
            DRV_OC3_Width (TYPR_PWM [Roll]); // Modifica el valor del PWM en el motor 4
            appData.state = APP_STATE_READ_IMU; // Cambia el status de la máquina de estados leer la IMU
        }
        break;
        
        default:{
        /* TODO: Handle error in application's state machine. */
        }
        break;
    }
}