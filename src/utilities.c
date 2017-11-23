#include "utilities.h"
#include <stdlib.h>
#include <stdio.h>

char vStr[34];

void Serial0Println(unsigned char *buffer) {
    int i = 0;
    while (buffer[i] != '\0') {
        DRV_USART0_WriteByte(buffer[i++]);
    }
    DRV_USART0_WriteByte('\n');
}

void Serial0Print(unsigned char *buffer) {
    int i = 0;
    while (buffer[i] != '\0') {
        DRV_USART0_WriteByte(buffer[i++]);
    }
}

void Serial0PrintInt(int v) {
    memset(vStr, '\0', sizeof(vStr));
    sprintf(vStr, "%d", v);
    Serial0Print(vStr);
}

void ftoa(double number,char * string, int precision) {
    sprintf (string,"%d.%02u", (int) number, (int) ((number - (int) number ) * precision));
}

void Serial0PrintFloat(float v){
    ftoa(v, vStr, 1000);
    Serial0Print(vStr);
}