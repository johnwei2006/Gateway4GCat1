#ifndef __LTE_DEF_H
#define __LTE_DEF_H

#define ARRAY_SIZE(a)					(sizeof(a)/sizeof(a[0]))
#define AT_COMMAND_ARRAY(a)				              a,ARRAY_SIZE(a)		//命令列表

#define AT_RESP_OK_STRING				                "OK"				//AT OK
#define AT_RESP_ERROR_STRING				                "ERROR"				//AT ERROR
#define AT_RESP_GREATERTHAN_SIGN                  ">"

#define AT_COMMAND_RETRY_MAX_COUNT		          10


#define AT_COMMAND_PARAM_FUNCTION(param)		    Get_##param
#define CREAT_AT_COMMAND_PARAM_FUNCTION(param)	static char *Get_##param(void){return param;}

typedef enum{
	AT_COMMAND_HANDLE_STATE_SUCCESS     = 0,
	AT_COMMAND_HANDLE_STATE_PARSE_ERROR = -1,
	AT_COMMAND_HANDLE_STATE_NO_SUPPORT  = -2,
	AT_COMMAND_HANDLE_STATE_OTHER_ERROR = -3,
}at_command_handle_state_t;

typedef struct{
	char 				    *at_cmd;
	char 				    *(*get_parameters)(void);
	unsigned int 		timeout;
	const char 			*expect_response;;
	at_command_handle_state_t (*command_handle)(void);
}at_command;

typedef struct at_command_process{
	at_command 		*command_array;
	unsigned short 	commandSize;
	unsigned int 	waitSecond;
	void (*ok_function)(void);
	struct at_command_process *next;
}at_command_process_t;

typedef enum{
  LTE_EVENT_NONE,
  LTE_EVENT_HAPPENED,
}lte_event_t; 

typedef struct{
  unsigned short year;
  unsigned char month;
  unsigned char day;
  
  unsigned char hour;
  unsigned char minute;
  unsigned char second;
  signed char zone;
}lte_time_t;

#endif
