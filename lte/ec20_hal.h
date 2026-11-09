#ifndef __EC20_HAL_H
#define __EC20_HAL_H
#include "utils.h"
#include "at.h"
#include "lte_def.h"
#define AT_CLIENT_NAME                    "uart6"
#define AT_BUFFER_SIZE                    4096
#define DEFAULT_AT_RESPONE_BUFFER_SIZE    4096
#define DEFAULT_AT_RESPONE_LINE_NUMBER    6
#define DEFAULT_AT_RESPONE_TIMEOUT        300

//#define EC20_WAKEIN_PIN                   GET_PIN(E, 1)    //EC20_WAKE_IN
#define EC20_USIM_PIN                     GET_PIN(D, 4)    //EC20_USIM_PRESENCE
//#define EC20_WAKEOUT_PIN                  GET_PIN(E, 13)    //EC20_WAKE_OUT
#define EC20_RESET_PIN                    GET_PIN(E, 11)    //EC20_RESET
//#define EC20_WDISABLE_PIN                 GET_PIN(E, 15)    //EC20_W_DISABLE
#define EC20_POWER_PIN                    GET_PIN(E, 10)     //EC20_POWER

#if LOG_EC20_USE_COLOR
    #define LOG_EC20(...)       do { \
        rt_kprintf("\033[1;34;40m[%s]EC200:", get_current_time_stamp()); \
        rt_kprintf(__VA_ARGS__); \
        rt_kprintf("\r\n\033[0m"); \
    } while(0)
#else
    #define LOG_EC20(...)       do { \
        rt_kprintf("[%s]EC200:", get_current_time_stamp()); \
        rt_kprintf(__VA_ARGS__); \
        rt_kprintf("\r\n"); \
    } while(0)
#endif

#define KEBAI_BASE_VERSION                     "V1.0.0"
#define KEBAI_CLOUD_DOMIN                        "api.kebaiiot.com"
#define KEBAI_CLOUD_PORT                          6002                            
//USIM插入状态
typedef enum{
  EC20_USIM_UNINSERTED,
  EC20_USIM_INSERTED,
}ec20_usim_state_t;                        
void EC20_Wakeup(void);
void EC20_Sleep(void);
at_response_t EC20_GetAtResp(void);
void EC20_Reboot(void);
at_response_t EC20_Boot(void);
ec20_usim_state_t EC20_UsimInsertState(void);
at_command_handle_state_t EC20_SimDetection(void);
#endif


