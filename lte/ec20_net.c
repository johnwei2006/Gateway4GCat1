/*
 * Copyright (c) 2020-2021, Kebai Development Team
 * Describe:
 * License-Identifier: KB_Base_RTT
 * Change Logs:
 * Date          Author         Notes
 * 2021-6-28     magnin          the first version
 */

#include "ec20_hal.h"
#include "lte_def.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>

#define APN_LENGTH 20 //APN����
#define IMEI_LENGTH 20
#define ICCID_LENGTH 21
#define NTP_SERVER   "ntp.aliyun.com"

	
//��Ӫ��APN
const struct ec_apn
{
  const char *isp;
  const char apn[APN_LENGTH + 1];
} ecApn[] = {
    {"CHINA MOBILE", "\"CMNET\",\"\",\"\",0"},
    {"CHN-UNICOM", "\"UNINET\",\"\",\"\",0"},
    {"CHN-CT", "\"CTNET\",\"\",\"\",0"},
};
//ע���������??
const struct ec_reg
{
  const uint8_t code;
  const char *tips;
} ecReg[] = {
    {0, "Not registered. ME is not currently searching a new operator to register"},
    {1, "Registered, home network"},
    {2, "Not registered, but ME is currently searching a new operator to register"},
    {3, "Registration denied"},
    {4, "Unknown"},
    {5, "Registered, roaming"},
};

static at_response_t atResp = RT_NULL;
//APN
static char apnStr[APN_LENGTH + 1] = {0};
//ָ����ָ??
static at_command_process_t *at_command_array = RT_NULL;

static char gmi[20] = {0};
static char gmm[20] = {0};
static char gmr[20] = {0};
static char imei[IMEI_LENGTH] = {0};
static char iccid[ICCID_LENGTH] = {0};
static char imsi[20] = {0};
static char operator[20] = {0};
static uint8_t csq;
static lte_time_t ntpTime;

static lte_event_t eventTimeSync = LTE_EVENT_NONE;

//ע��AT������غ���
CREAT_AT_COMMAND_PARAM_FUNCTION(apnStr)

//��ȡ��Ӫ��CCID
static at_command_handle_state_t EC20_ReadCOPS(void)
{
  char isp[20];
	atResp=EC20_GetAtResp();
  int res = at_resp_parse_line_args_by_kw(atResp, "+COPS:", "+COPS:%*[^\"]\"%[^\"]", isp);
  if (res != 1)
  {
    LOG_EC20("COPS error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }

  for (uint8_t i = 0; i < ARRAY_SIZE(ecApn); i++)
  {
    if (rt_strcmp(ecApn[i].isp, isp) == 0) //ƥ����Ӫ??
    {
      rt_strncpy(apnStr, ecApn[i].apn, rt_strlen(ecApn[i].apn));
      rt_strncpy(operator, ecApn[i].isp, rt_strlen(ecApn[i].isp));
      LOG_EC20("COPS isp:%s,apn:%s", ecApn[i].isp, (const char *)ecApn[i].apn);
      return AT_COMMAND_HANDLE_STATE_SUCCESS;
    }
  }
  rt_strncpy(apnStr, (const char *)ecApn[2].apn, rt_strlen(ecApn[2].apn)); //Ĭ��ʹ�õ���APN���ֲ���û��������Ӫ����??
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

at_command_handle_state_t EC20_SimDetection(void)
{
  at_command_handle_state_t result = AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
  ec20_usim_state_t state;
  do
  {
    state = EC20_UsimInsertState();
    if (state == EC20_USIM_INSERTED)
      result = AT_COMMAND_HANDLE_STATE_SUCCESS;
    else
    {
      LOG_EC20("EC20 SIM card uninserted,please insert sim card!");
      rt_thread_mdelay(2000);
    }

  } while (state != EC20_USIM_INSERTED); //SIM��������??

  return result;
}

static at_command_handle_state_t EC20_SetGMI(void)
{
  char *pStr;
	atResp=EC20_GetAtResp();
  pStr = (char *)at_resp_get_line(atResp, 2);
  if (pStr == RT_NULL)
  {
    LOG_EC20("EC20 get gmi error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  sscanf(pStr, "%s", gmi);
  LOG_EC20("EC20 GMI %s", gmi);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

static at_command_handle_state_t EC20_SetGMM(void)
{
  char *pStr;
	atResp=EC20_GetAtResp();
  pStr = (char *)at_resp_get_line(atResp, 2);
  if (pStr == RT_NULL)
  {
    LOG_EC20("EC20 get gmm error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  sscanf(pStr, "%s", gmm);
  LOG_EC20("EC20 GMM %s", gmm);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

static at_command_handle_state_t EC20_SetGMR(void)
{
  char *pStr;
	atResp=EC20_GetAtResp();
  pStr = (char *)at_resp_get_line(atResp, 2);
  if (pStr == RT_NULL)
  {
    LOG_EC20("EC20 get gmr error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  sscanf(pStr, "%s", gmr);
  LOG_EC20("EC20 GMR %s", gmr);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}
//IMEI
static at_command_handle_state_t EC20_SetIMEI(void)
{
  char *pStr;
	atResp=EC20_GetAtResp();
  pStr = (char *)at_resp_get_line(atResp, 2);
  if (pStr == RT_NULL)
  {
    LOG_EC20("EC20 get imei error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  while (*pStr < '0' || *pStr > '9')
    pStr += 1;
  sscanf(pStr, "%s", imei);
  LOG_EC20("EC20 IMEI %s", imei);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}
//ICCID
static at_command_handle_state_t EC20_SetICCID(void)
{
	 
  int res = at_resp_parse_line_args_by_kw(atResp, "+QCCID:", "+QCCID:%s", iccid);
  if (res != 1)
  {
    LOG_EC20("ICCID error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  LOG_EC20("EC20 ICCID %s", iccid);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

static at_command_handle_state_t EC20_SetIMSI(void){
  char *pStr;
	atResp=EC20_GetAtResp();
  pStr = (char *)at_resp_get_line(atResp, 2);
  if (pStr == RT_NULL)
  {
    LOG_EC20("EC20 get imsi error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  while (*pStr < '0' || *pStr > '9')
    pStr += 1;
  sscanf(pStr, "%s", imsi);
  LOG_EC20("EC20 IMSI %s", imsi);
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}

//��ȡע����ʾ
static const char *EC20_GetRegTips(int code)
{
  for (uint8_t i = 0; i < ARRAY_SIZE(ecReg); i++)
  {
    if (code == ecReg[i].code)
      return ecReg[i].tips;
  }
  return "Unknow reg code";
}
//����״??
static at_command_handle_state_t EC20_NetState(void)
{
  at_command_handle_state_t result = AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
  int i = 0;
  int res = at_resp_parse_line_args_by_kw(atResp, "+CREG:", "+CREG:%*d,%d", &i);
  if (res != 1)
  {
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  if (i == 1 || i == 5)
  {
    result = AT_COMMAND_HANDLE_STATE_SUCCESS;
  }
  else
    rt_thread_mdelay(1000);
  LOG_EC20("EC20 Regist net code %d", i);
  LOG_EC20(EC20_GetRegTips(i));

  return result;
}
//����CSQ
static at_command_handle_state_t EC20_NetCSQ(void)
{
  at_command_handle_state_t result = AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
  int i = 0;
  int res = at_resp_parse_line_args_by_kw(atResp, "+CSQ:", "+CSQ:%d,", &i);
  if (res != 1)
  {
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  csq = i;
  result = AT_COMMAND_HANDLE_STATE_SUCCESS;
  LOG_EC20("EC20 csq %d", csq);

  return result;
}
//GPRS����״??
static at_command_handle_state_t EC20_GprsNetState(void)
{
  at_command_handle_state_t result = AT_COMMAND_HANDLE_STATE_OTHER_ERROR;
  int i = 0;
  int res = at_resp_parse_line_args_by_kw(atResp, "+CGREG:", "+CGREG:%*d,%d", &i);
  if (res != 1)
  {
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  if (i == 1 || i == 5)
  {
    result = AT_COMMAND_HANDLE_STATE_SUCCESS;
  }
  else
    rt_thread_mdelay(1000);
  LOG_EC20("EC20 Regist gprs net code %d", i);
  LOG_EC20(EC20_GetRegTips(i));

  return result;
}
//��ȡNTP
static at_command_handle_state_t EC20_ReadNtp(void)
{
  int time[6];
  int res = at_resp_parse_line_args_by_kw(atResp, "+QNTP:", "+QNTP:%*[^\"]\"%d/%d/%d,%d:%d:%d", &time[0], &time[1], &time[2], &time[3], &time[4], &time[5]);
  if (res != 6)
  {
    LOG_EC20("NTP sync error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}
//��ȡTIME
static at_command_handle_state_t EC20_ReadTime(void)
{
  int time[6];
  int zone;
  int res = at_resp_parse_line_args_by_kw(atResp, "+CCLK:", "+CCLK:%*[^\"]\"%d/%d/%d,%d:%d:%d%d", &time[0], &time[1], &time[2], &time[3], &time[4], &time[5],&zone);
  if (res != 7)
  {
    LOG_EC20("get time error");
    return AT_COMMAND_HANDLE_STATE_PARSE_ERROR;
  }
  ntpTime.year = time[0] + 2000;
  ntpTime.month = time[1];
  ntpTime.day = time[2];
  ntpTime.hour = time[3];
  ntpTime.minute = time[4];
  ntpTime.second = time[5];
  ntpTime.zone = zone / 4;
  LOG_EC20("time:%d-%d-%d %d:%d:%d,time zone:%d", ntpTime.year, ntpTime.month, ntpTime.day, ntpTime.hour, ntpTime.minute, ntpTime.second,ntpTime.zone);
  eventTimeSync = LTE_EVENT_HAPPENED;
  return AT_COMMAND_HANDLE_STATE_SUCCESS;
}
//AT���ָ??
static at_command commandAt[] = {
    //{"AT",                        RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    {"AT",                        RT_NULL, 800,   AT_RESP_OK_STRING, EC20_SimDetection},
    {"ATE0",                      RT_NULL, 1000,  AT_RESP_OK_STRING, RT_NULL},      //�رջ���
    {"AT+QCFG=\"NWSCANMODE\",3",  RT_NULL, 1000,  AT_RESP_OK_STRING, RT_NULL},      //����2G??3G??4G
    {"AT+QSCLK=0",                RT_NULL, 1000,  AT_RESP_OK_STRING, RT_NULL},      //�رյ���ʱ??
    {"AT+CGMI",                   RT_NULL, 800,   AT_RESP_OK_STRING, EC20_SetGMI},      //ģ�鹩Ӧ??
    {"AT+CGMM",                   RT_NULL, 800,   AT_RESP_OK_STRING, EC20_SetGMM},      //ģ��汾ID
    {"AT+CGMR",                   RT_NULL, 800,   AT_RESP_OK_STRING, EC20_SetGMR},      //ģ��̼�ID
    {"AT+CGSN",                   RT_NULL, 800,   AT_RESP_OK_STRING, EC20_SetIMEI}, //IMEI 
};
//SIM�����ָ??
static at_command commandSim[] = {
    //{"AT&F",                        RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT&W",                        RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT+CFUN=0",                   RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT+CFUN=1,1",                 RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT+CSIM=2",                   RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT+CSIM?",                    RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},
    //{"AT+CFUN=1",                   RT_NULL, 800,   AT_RESP_OK_STRING, RT_NULL},

    {"AT+CPIN?",  RT_NULL, 800, "READY",            RT_NULL},       //��״̬��??
    {"AT+QCCID",  RT_NULL, 800, AT_RESP_OK_STRING,  EC20_SetICCID}, //��sim��ICCID
    {"AT+CIMI",   RT_NULL, 800, AT_RESP_OK_STRING,  EC20_SetIMSI},  //��sim��IMSI
    //{"AT+QSIMDET",        RT_NULL,    800,  AT_RESP_OK_STRING,  RT_NULL        },      //����SIM���Ȳ��
};

//����ע��״??
static at_command commandCS[] = {
    {"AT+CSQ",    RT_NULL, 800,  AT_RESP_OK_STRING, EC20_NetCSQ},   //�źż�??
    {"AT+CREG?",  RT_NULL, 1000, AT_RESP_OK_STRING, EC20_NetState}, //����״̬��??
};
//����CSQ
static at_command commandCSQ[] = {
    {"AT+CSQ",    RT_NULL, 800,  AT_RESP_OK_STRING, EC20_NetCSQ},   //�źż�??
};

//PDP ����
static at_command commandPDP[] = {
    {"AT+COPS?",        RT_NULL,                            800, AT_RESP_OK_STRING,   EC20_ReadCOPS},//COPS
    {"AT+QICSGP=1,1,",  AT_COMMAND_PARAM_FUNCTION(apnStr),  800, AT_RESP_OK_STRING,   RT_NULL},      //APN
    {"AT+QIDEACT=1",    RT_NULL,                            800, AT_RESP_OK_STRING,   RT_NULL},      //�ر��ƶ�����
    {"AT+QIACT=1",      RT_NULL,                            2000, AT_RESP_OK_STRING,  RT_NULL},      //�����ƶ���??
};
//NTP ����
static at_command commandNTPConfig[] = {
    {"AT+CTZU=3", RT_NULL, 800, AT_RESP_OK_STRING, RT_NULL}, //NTP
};
//NTP ��ȡʱ��
static at_command commandNTP[] = {
    {"AT+QNTP=1,\""NTP_SERVER"\"",    RT_NULL, 4000,  "+QNTP:",          EC20_ReadNtp},   //NTP
    {"AT+CCLK?",                      RT_NULL, 800,   AT_RESP_OK_STRING, EC20_ReadTime},  //TIME
    {"AT+QICFG=?",                    RT_NULL, 800,   RT_NULL,           RT_NULL},
};
//NTP ��ȡʱ��
static at_command commandPing[] = {
    {"AT+QPING=2,\"baidu.com\",2,1",     RT_NULL,         3000,  AT_RESP_OK_STRING,  RT_NULL  },      //PING
}; 
//ָ���
static at_command_process_t commandProcess[] = {
    {AT_COMMAND_ARRAY(commandAt),         10, RT_NULL, &commandProcess[1]},
    {AT_COMMAND_ARRAY(commandSim),        20, RT_NULL, &commandProcess[2]},
    {AT_COMMAND_ARRAY(commandCS),         90, RT_NULL, &commandProcess[3]},
    {AT_COMMAND_ARRAY(commandPDP),        0,  RT_NULL, &commandProcess[4]},
    {AT_COMMAND_ARRAY(commandNTPConfig),  0,  RT_NULL, &commandProcess[5]},
    {AT_COMMAND_ARRAY(commandNTP),        10, RT_NULL, RT_NULL},
    {AT_COMMAND_ARRAY(commandCSQ),        10, RT_NULL, RT_NULL},
//    {AT_COMMAND_ARRAY(commandPing),       10, RT_NULL, RT_NULL},

};

const char *EC20_GetGMI(void){
  return (const char *)gmi;
}
const char *EC20_GetGMM(void){
  return (const char *)gmm;
}
const char *EC20_GetGMR(void){
  return (const char *)gmr;
}
//IMEI
const char *EC20_GetIMEI(void)
{
  return (const char *)imei;
}
//ICCID
const char *EC20_GetICCID(void)
{
  return (const char *)iccid;
}
//IMSI
const char *EC20_GetIMSI(void)
{
  return (const char *)imsi;
}
//��Ӫ��
const char *EC20_GetOperator(void)
{
  return (const char *)operator;
}

lte_time_t *EC20_GetTime(void)
{
  return &ntpTime;
}
//��ȡ�����ź�ǿ��
uint8_t EC20_GetCSQ(void)
{
   return csq;
}
//��������??
void EC20_SetNetCommandsFirst(at_response_t resp)
{
  atResp = resp;
  at_command_array = &commandProcess[0];
  LOG_EC20("EC20 set network first command");
}
//��ȡ��������
at_command_process_t **EC20_GetNetCommands(void)
{
  return &at_command_array;
}

/*************************************************end file*******************************************************/
