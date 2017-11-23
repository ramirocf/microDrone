#ifndef _UTILITIES_H    /* Guard against multiple inclusion */
#define _UTILITIES_H

#include "system_config.h"
#include "system_definitions.h"

void Serial0Println(unsigned char *buffer);
void Serial0PrintInt(int v);
void Serial0PrintFloat(float v);
void Serial0Print(unsigned char *buffer);

#endif /* _UTILITIES_H */
