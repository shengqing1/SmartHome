#ifndef MQTT_H
#define MQTT_H

#include <iostream>
#include <unistd.h>
#include <sys/socket.h> //包含了用于创建和操作套接字的函数和数据结构定义
#include <netinet/in.h> //提供与网络协议相关的数据结构和宏定义，特别是与Internet协议族（IPv4）相关的
#include <arpa/inet.h>	//包含了用于处理网络地址的函数
#include <netdb.h>		//提供了与网络数据库操作相关的函数
#include <cstring>
// 用于从32位整数中提取各个字节。
#define BYTE0(temp) (*(char *)(&temp))
#define BYTE1(temp) (*((char *)(&temp) + 1))
#define BYTE2(temp) (*((char *)(&temp) + 2))
#define BYTE3(temp) (*((char *)(&temp) + 3))

// 全局变量定义
extern unsigned char mqtt_rxbuf[1024 * 1024]; // MQTT接收缓冲区，用于存储从服务器接收到的数据
extern unsigned char mqtt_txbuf[256];		  // MQTT接收缓冲区，用于存储从服务器接收到的数据
extern unsigned int mqtt_rxlen;				  // MQTT接收数据的长度
extern unsigned int mqtt_txlen;				  // MQTT发送数据的长度

// 函数声明
void MQTT_Init(void);																	  // MQTT初始化函数
unsigned char MQTT_Connect(char *ClientID, char *Username, char *Password);				  // MQTT连接函数
unsigned char MQTT_SubscribeTopic(char *topic, unsigned char qos, unsigned char whether); // MQTT订阅主题函数
unsigned char MQTT_PublishData(char *topic, char *message, unsigned char qos);			  // MQTT发布数据函数
void MQTT_SendBuf(unsigned char *buf, unsigned short len);								  // MQTT发送缓冲区函数
int Client_SendData(unsigned char *buff, unsigned int len);								  // 客户端发送数据函数
int Client_GetData(unsigned char *buff);												  // 客户端接收数据函数

#endif // MQTT_H