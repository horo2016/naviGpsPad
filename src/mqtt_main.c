#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>

#include <time.h> 
#include <sys/time.h> 

#include "Mqtt_Client.h"
#include "cJSON.h"
#include<pthread.h>
#include "imu.h"
#include "cpu_sys.h"
#include "stm32_control.h"
#include "navi_manage.h"
// int mysock = 0;

#define TURNROUND "turnround"
#define RUNFORWARD "runforward"



int toStop = 0;

MQTT_USER_MSG  User_MqttMsg;

static void User_MsgCtl(MQTT_USER_MSG  *msg);
static int GetNextPackID(void);

MQTT_ClientStruct user_Client = 
{
	.sock = -1,
	.Status = DisConnect,
	.length = MQTT_BUFFERLENTH,
	.open = transport_open,
	.getdata = transport_getdata,
	.sendPacket = transport_sendPacketBuffer,
	.MsgCtl = User_MsgCtl,
	.GetNextPackID = GetNextPackID,
};
 char  send_buf[0xff]={0};
int Creatstatejson(float head,float roll,float pitch)
{
	

    cJSON * root =  cJSON_CreateObject();
   if(!root) {
         printf("get root faild !\n");
     }

  //  cJSON_AddItemToObject(root, "\"devid\"", cJSON_CreateString(chargename));
    cJSON_AddItemToObject(root, "heading", cJSON_CreateNumber((short)head));
    cJSON_AddItemToObject(root, "roll", cJSON_CreateNumber((short)roll));
    cJSON_AddItemToObject(root, "pitch",cJSON_CreateNumber((short)pitch));
    cJSON_AddItemToObject(root, "cpuload", cJSON_CreateNumber((char)cpuPercentage));
    cJSON_AddItemToObject(root, "cputemp", cJSON_CreateNumber((char)cpuTemperature));
    cJSON_AddItemToObject(root, "wifisignal", cJSON_CreateNumber(wifiSignalStrength));
    cJSON_AddItemToObject(root, "velspeed", cJSON_CreateNumber((int)velspeed));
    cJSON_AddItemToObject(root, "angspeed", cJSON_CreateNumber(angspeed));
    cJSON_AddItemToObject(root, "targetheading", cJSON_CreateNumber((int)targetHeading));
    cJSON_AddItemToObject(root, "distance", cJSON_CreateNumber((int)waypointRange));
    cJSON_AddItemToObject(root, "nextwaypoint_lon", cJSON_CreateNumber(waypointlongitude));
    cJSON_AddItemToObject(root, "nextwaypoint_lat", cJSON_CreateNumber(waypointlatitude));
    
    memcpy(send_buf,cJSON_Print(root),strlen(cJSON_Print(root)));
      
    
    
    cJSON_Delete(root);

    return 0;
}
/*******************************************************************************
 * function name	: get_strchr_len
 * description	: Get the migration length of the characters in a string
 * param[in] 	: str-string, c-char
 * param[out] 	: none
 * return 		: >0-len, -1-fail
 *******************************************************************************/
int get_strchr_len(char *str, char c)
{
	int i = 0;
	int len = 0;

	if(!str)
		return -1;

	while(*(str+i++) != c)
		len ++;

	return len;
}
/*******************************************************************************
 * function name	: get_value_from_config_file
 * description	: get value from config
 * param[in] 	: file-config file, key-dest string, value-key value
 * param[out] 	: none
 * return 		: 0-exist, -1-not exist
 *******************************************************************************/
int get_value_from_cmdline( char* buf, char *key, char *value)
{
	int value_len = 0 ;

	char *sub_str = NULL ;
	size_t line_len = 0 ;
	size_t len = 0;


		//printf("zdf config file : %s\n", buf);
		sub_str = strstr(buf, key);

		if(sub_str != NULL){//video="/home/linaro/video"
			sub_str += (strlen(key) + 1); //=" length is 2 bytes
		//	printf("zdf sub_str : %s\n", sub_str);
			value_len = get_strchr_len(sub_str, '\n');
		//	printf("zdf value len = %d\n", value_len);
			memcpy(value, sub_str, value_len);
			
			return 0;
		}

	


	return -1;
}
void cfinish(int sig)
{
    signal(SIGINT, NULL);
	toStop = 1;
}

void stop_init(void)
{
	signal(SIGINT, cfinish);
	signal(SIGTERM, cfinish);
}

void User_MsgCtl(MQTT_USER_MSG  *msg)
{
	char tmp_value[36];
	switch(msg->msgqos)
	{
		case 0:
			printf("MQTT>>消息质量: QoS0\n");
			break;
		case 1:
			printf("MQTT>>消息质量: QoS1\n");
			break;
		case 2:
			printf("MQTT>>消息质量: QoS2\n");
			break;
		default:
			printf("MQTT>>error quantity\n");
			break;
	}
	printf("MQTT>>msg topic %s\r\n",msg->topic);	
	printf("MQTT>>msg: %s\r\n",msg->msg);	
	printf("MQTT>>msg length: %d\r\n",msg->msglenth);	 
	
      if( get_value_from_cmdline((char *)msg->msg,RUNFORWARD,tmp_value )==0){
            printf("runforward %d \n",atoi(tmp_value));
			GLOBAL_STATUS = MOVE_STATUS;
			MoveDistance(atoi(tmp_value));
	}
      else  if( get_value_from_cmdline((char *)msg->msg,TURNROUND,tmp_value )==0)
	{
	   GLOBAL_STATUS = MANUAL_STATUS;
           RotateDegrees(atoi(tmp_value));
	}  else  if( get_value_from_cmdline((char *)msg->msg,"start",tmp_value )==0)
	{
           GLOBAL_STATUS = STANDBY_STATUS;
	   GLOBAL_SWITCH = 1;
	}else  if( get_value_from_cmdline((char *)msg->msg,"stop",tmp_value )==0)
        {
           GLOBAL_STATUS = STOP_STATUS;
           GLOBAL_SWITCH = 0;
        }
	
	// 处理后销毁数据
	
	msg->valid  = 0;
}

static int GetNextPackID(void)
{
	 static unsigned int pubpacketid = 0;
	 return pubpacketid++;
}

void *Mqtt_ClentTask(void *argv)
{
	MQTTPacket_connectData connectData= MQTTPacket_connectData_initializer;

	int rc = 0, type;
	int PublishTime = 0;
	int heartbeatTime = 0;

    struct timeval    sys_tv, tv;
    struct timezone		tz; 
	fd_set readfd;

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	connectData.willFlag = 0;
	// 创建 MQTT 客户端连接参数
	connectData.MQTTVersion = 4;
	// MQTT 版本
	connectData.clientID.cstring = ClientID;
	// 客户端ID
	connectData.keepAliveInterval = KEEPLIVE_TIME;
	// 保活间隔
	connectData.username.cstring = UserName;
	// 用户名
	connectData.password.cstring = UserPassword;
	// 用户密码
	connectData.cleansession = 1;

	user_Client.Status = DisConnect;
	while (1)
	{
		user_Client.sock = user_Client.open(HOST_NAME, HOST_PORT);
		if(user_Client.sock >= 0)
		{
			break;
		}

		printf("ReConnect\n");
		sleep(3);
	}

	rc = MQTTClientInit(user_Client, connectData);
	if (rc < MqttClientSuccess)
	{
		printf("MQTT_reconnect %d\n", rc);
		goto MQTT_reconnect;
	}

	rc = MQTTClientSubscribe(user_Client, SubscribeMsg, QOS0);
	// 订阅 SubscribeMsg
	if(rc < 0)
		goto MQTT_reconnect;

	printf("开始循环接收消息...\r\n");

	user_Client.Status = Connect;
	while (1)
	{
		gettimeofday(&sys_tv, &tz);

		FD_ZERO(&readfd);
		// 推送消息
		FD_SET(user_Client.sock,&readfd);						  

		select(user_Client.sock+1, &readfd, NULL, NULL, &tv);
		// 等待可读事件

		if(FD_ISSET(user_Client. sock, &readfd) != 0)
		{
			// 判断 MQTT 服务器是否有数据
			type = MQTTClientReadPacketTimeout(&user_Client, 0);
			if(type != -1)
				MQTTClientCtl(user_Client, type);
		} 

		if ((sys_tv.tv_sec-heartbeatTime ) > 15)
		{
			// 发送心跳包
			printf("tv_sec:%ld\n",sys_tv.tv_sec);
			heartbeatTime = sys_tv.tv_sec;

			if(MQTTClientSendPingReq(user_Client) < 0)
				goto MQTT_reconnect;
		}
	
	}
MQTT_reconnect:
	printf("MQTT_reconnect\n");

	return 0;
}
void *Mqtt_PublishTask(void *argv)
{
	
	stop_init();

	
	// Mqtt_ClentTask(NULL);

	while (1)
	{
		

		if (user_Client.Status == Connect){
			Creatstatejson(heading,rollAngle,pitchAngle);
			//printf("mqtt sensor: %s \n",send_buf);
			if (MQTTMsgPublish(user_Client, "/update/state", QOS0, 0,
				(unsigned char *)send_buf,strlen(send_buf)) != 0)
			{
				printf("<0\n");
			}	
			memset(send_buf,0,sizeof(send_buf));
		}
		sleep(1);
	}

	return 0;
}
