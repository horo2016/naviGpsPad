#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif



#include "MQTTPacket.h"
#include "lib/transport.h"

#define ClientID                "0A0B0C0D|securemode=3,signmethod=hmacsha1,timestamp=1534728682167|"
#define UserName                " "
#define UserPassword            " "
#define HOST_NAME				"127.0.0.1"
#define HOST_PORT				1883

#define ReleaseMsg              "/update/state"
#define SubscribeMsg            "/download/uniform"

#define KEEPLIVE_TIME			500

#define MSG_MAX_LEN				300
#define MSG_TOPIC_LEN			50

#define MQTT_BUFFERLENTH        512

// 数据交互结构体
typedef struct __MQTTMessage
{
	unsigned int qos;
	unsigned char  retained;
	unsigned char  dup;
	unsigned short id;
	unsigned char  type;
	void *payload;
	int payloadlen;
}MQTTMessage;

// 用户接受消息结构体
typedef struct __MQTT_MSG
{
	unsigned char msgqos;
	//消息质量
	unsigned char msg[MSG_MAX_LEN];
	//消息
	unsigned int msglenth;
	//消息长度
	unsigned char topic[MSG_TOPIC_LEN];
	//主题
	unsigned short packetid;
	//消息ID
	unsigned char   valid;
	//标明消息是否有效
}MQTT_USER_MSG;
// 用户接受消息结构体

typedef enum
{
	DisConnect = 0,
	Connect,
}MQTT_ClientStatus;

typedef struct __MQTT_CLIENT_STRUCT
{
	int sock;
	int length;
	MQTT_ClientStatus Status;
	char buffer[MQTT_BUFFERLENTH];
	int (*open)(char* addr, int port);
	int (*getdata)(unsigned char* buf, int count);
	int (*sendPacket)(int sock,unsigned char* buf, int count);
	void (*MsgCtl)(MQTT_USER_MSG  *msg);
	int (*GetNextPackID)(void);
}MQTT_ClientStruct;

typedef enum 
{
	MqttClientNoResubscribe = 1,
	MqttClientSuccess = 0,
	MqttClientSentErr = -1,
	MqttClientReadErr = -2,
	MqttClientReadNone = -3,
	MqttClientWaitReadErr = -4,
	MqttClientConnackErr = -5,
	MqttClientMemoryErr = -6,
	MqttClientSubackErr = -7,
	MqttClientDataErr = -8,
	MqttClientSubscribeNone = -9,
} MqttClentErr_t;

enum QoS { QOS0, QOS1, QOS2 };

int MQTTClientInit
		(MQTT_ClientStruct User_MQTTClient, MQTTPacket_connectData connectData);
int MQTTClientSubscribe
		(MQTT_ClientStruct User_MQTTClient, char *topic, enum QoS pos);
int MQTTClientSendPingReq (MQTT_ClientStruct User_MQTTClient);
void MQTTClientCtl(MQTT_ClientStruct User_MQTTClient, unsigned char  packtype);
int MQTTClientReadPacketTimeout (MQTT_ClientStruct *User_MQTTClient, unsigned int timeout);
int MQTTMsgPublish(MQTT_ClientStruct User_MQTTClient, char *topic, char qos, char retained, unsigned char * msg, unsigned int msg_len);

#ifdef __cplusplus
}
#endif
#endif
