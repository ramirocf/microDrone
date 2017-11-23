#ifndef _APP_H
#define _APP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "system_config.h"
#include "system_definitions.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END 


// *****************************************************************************
// For arrays containing info related to those parameters, following convention
// of storage will be used
#define Throttle 0
#define Yaw 1
#define Pitch 2
#define Roll 3

#define MAX_PWM 624
#define THROTTLE_PROPORTION ((float) ( 0.91 * MAX_PWM ) )
#define YAW_PROPORTION ( (float) ( 0.03 * MAX_PWM ) )
#define PITCH_PROPORTION ( (float) ( 0.03 * MAX_PWM ) )
#define ROLL_PROPORTION ( (float) ( 0.03 * MAX_PWM ) )
#define MAX_THROTTLE 162
#define YAW_MID_VAL 89
#define YAW_NUM_VALS_LEFT 89
#define YAW_NUM_VALS_RIGHT 84
#define PITCH_MID_VAL 83
#define PITCH_NUM_VALS_LEFT 82
#define PITCH_NUM_VALS_RIGHT 82
#define ROLL_MID_VAL 90
#define ROLL_NUM_VALS_LEFT 90 
#define ROLL_NUM_VALS_RIGHT 90
/* Application states

  Summary:
    Application states enumeration

  Description:
    This enumeration defines the valid application states.  These states
    determine the behavior of the application at various times.
*/    
typedef enum
{
  APP_STATE_INIT=0,
  APP_STATE_SERVICE_TASKS,
  APP_STATE_ENABLE_PILOT,
  APP_STATE_READ_IMU,
  APP_STATE_READ_WIFI,
  APP_STATE_MAP_APP_DATA,
  APP_STATE_MODULE_PWM,

} APP_STATES;

typedef enum {
  WAIT_IMU_TX_START,
  WAIT_EQUAL,
  PARSE_VALUE
} READ_IMU_STATES;

typedef enum {
  WAIT_CLIENT_TX_START,
  GET_THROTTLE,
  GET_YAW,
  GET_PITCH,
  GET_ROLL
} READ_CLIENT_STATE;

typedef enum {
    INIT_MOTORS,
    INIT_CLIENT
} INIT_STATES;

typedef enum {
    WAIT_SERVER,
    WAIT_CLIENT
} INIT_CLIENT_STATES;

typedef struct {
    INIT_CLIENT_STATES initClientState;
} INIT_INFO;
// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
 */

typedef struct
{
    /* The application's current state */
    APP_STATES state;
    READ_IMU_STATES readIMUState;
    READ_CLIENT_STATE readClientState;
    INIT_STATES initState;
    INIT_INFO initInfo;
} APP_DATA;


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************


void APP_Initialize ( void );
void APP_Tasks( void );


#endif /* _APP_H */

#ifdef __cplusplus
}
#endif