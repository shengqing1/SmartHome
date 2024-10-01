#include "mqtt.h"
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <wiringPi.h>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define SERVER_IP "117.78.5.125"
#define SERVER_PORT 1883

#define ClientID "66e7c3a7d671df42eb372386_dev1_0_0_2024091800"
#define Username "66e7c3a7d671df42eb372386_dev1"
#define Password "e9c621f8212c344cac5ed6b72599593c6dc832576c82e1d1c0d3b780c15e21e5"

#define SET_TOPIC "$oc/devices/66e7c3a7d671df42eb372386_dev1/sys/messages/down"
#define POST_TOPIC "$oc/devices/66e7c3a7d671df42eb372386_dev1/sys/properties/report"

typedef unsigned char uint8;
typedef unsigned int  uint16;
typedef unsigned long uint32;

#define HIGH_TIME 32
char mqtt_message[1024*1024];//上报数据缓存区
char request_id[100];
char mqtt_cmd_message[100];
char mqtt_cmd_data[100];
int pinNumber = 17;
uint32 databuf;
uint8 readSensorData(void)
{
    uint8 crc;
    uint8 i;

    pinMode(pinNumber, OUTPUT); // set mode to output
    digitalWrite(pinNumber, 0); // output a high level
    delay(25);
    digitalWrite(pinNumber, 1); // output a low level
    pinMode(pinNumber, INPUT); // set mode to input
    pullUpDnControl(pinNumber, PUD_UP);

    delayMicroseconds(27);
    if (digitalRead(pinNumber) == 0) //SENSOR ANS
    {
        while (!digitalRead(pinNumber))
            ; //wait to high

        for (i = 0; i < 32; i++)
        {
            while (digitalRead(pinNumber))
                ; //data clock start
            while (!digitalRead(pinNumber))
                ; //data start
            delayMicroseconds(HIGH_TIME);
            databuf *= 2;
            if (digitalRead(pinNumber) == 1) //1
            {
                databuf++;
            }
        }

        for (i = 0; i < 8; i++)
        {
            while (digitalRead(pinNumber))
                ; //data clock start
            while (!digitalRead(pinNumber))
                ; //data start
            delayMicroseconds(HIGH_TIME);
            crc *= 2;
            if (digitalRead(pinNumber) == 1) //1
            {
                crc++;
            }
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

#define MAXSIZE 1024*1024
int sockfd,len;
char buffer[MAXSIZE];
struct sockaddr_in their_addr;
FILE *fq;
void *pth_work_func(void *arg)
{


    while(!feof(fq)){
        len = fread(buffer, 1, sizeof(buffer), fq);
        printf("len is : %d\n", len);
        if(len != write(sockfd, buffer, len)){
            break;
        }
    }

    close(sockfd);
    fclose(fq);
}

extern int connectSocket;
double DHT11_T;// 	环境温度
double DHT11_H;//	环境湿度
int MQ2;//     	烟雾浓度检测
int flame;//     	火焰检测
int light;//		光强检测
int LED1;//   		LED1控制
int LED2;//   		LED2控制
int LED3;//   		LED3控制
/*
硬件连线：
MQ2烟雾传感器：GPIO4
DHT11温湿度传感器：GPIO17
火焰检测传感器：GPIO18
光敏电阻：GPIO27
蜂鸣器：GPIO22
LED灯1：GPIO23
LED灯2：GPIO24
LED灯3：GPIO25
*/
void initGPIO(){
    if (-1 == wiringPiSetupGpio()) {
        printf("Setup wiringPi failed!");
    }
    pinMode(22,OUTPUT);
    //配置GPIO口的模式
    //输出模式
    pinMode(23,OUTPUT);
    pinMode(24,OUTPUT);
    pinMode(25,OUTPUT);
    pinMode(6,OUTPUT);
    pinMode(22,OUTPUT);

    //输入模式
    pinMode(4,INPUT);
    pinMode(18,INPUT);
    pinMode(27,INPUT);
    pinMode(5,INPUT);

    //DHT11温湿度初始化
    pinMode(pinNumber, OUTPUT); // set mode to output
    digitalWrite(pinNumber, 1); // output a high level
}
void readDHT(){
    if (readSensorData())
    {
        printf("Sensor data read ok!\n");
        printf("RH:%d.%d\n", (databuf >> 24) & 0xff, (databuf >> 16) & 0xff);
        printf("TMP:%d.%d\n", (databuf >> 8) & 0xff, databuf & 0xff);
        DHT11_H=(double)((databuf >> 24) & 0xff)+(double)((databuf >> 16) & 0xff)*0.1;
        DHT11_T=(double)((databuf >> 8) & 0xff)+(double)(databuf & 0xff)*0.1;
        databuf = 0;
    }
    else
    {
        printf("Sensor dosent ans!\n");
        databuf = 0;
    }
}
void readMQ2(){
    MQ2=digitalRead (4); //读取GPIO口电平状态
    printf("MQ2:%d\r\n",MQ2);
    //火警报警
    if(MQ2==0)
    {
        digitalWrite(22,HIGH); //蜂鸣器响
    }
    else
    {
        digitalWrite(22,LOW);  //蜂鸣器关
    }
}
void readFlame(){
    flame=digitalRead (18); //读取GPIO口电平状态
    printf("flame:%d\r\n",flame);
}
void readLight(){
    light=digitalRead (27); //读取GPIO口电平状态
    printf("light:%d\r\n",light);
}
int main() {
    connectSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (connectSocket < 0) {
        perror("socket creation failed");
        return 1;
    }

    struct sockaddr_in service;
    memset(&service, 0, sizeof(service));
    service.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &service.sin_addr);
    service.sin_port = htons(SERVER_PORT);

    if (connect(connectSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        perror("connect failed");
        close(connectSocket);
        return 1;
    }

    std::cout << "Connected to server." << std::endl;
    MQTT_Init();
    while (1) {
        if (MQTT_Connect((char*)ClientID, (char*)Username, (char*)Password) == 0) {
            break;
        }
        sleep(1);
        std::cout << "MQTT服务器登录校验中....\n";
    }

    std::cout << "连接成功\n";

    int stat = MQTT_SubscribeTopic((char*)SET_TOPIC, 1, 1);
    if (stat) {
        std::cout << "订阅失败\n";
        close(connectSocket);
        return 1;
    }
    std::cout << "订阅成功\n";


    initGPIO();

    std::cout << "Starting..." << std::endl;
    while (1) {
        readDHT();
        //读取MQ2烟雾传感器状态
        readMQ2();
        //读取火焰传感器状态
        readFlame();
        //读取光敏传感器状态
        readLight();
        std::cout << "start send message." << std::endl;
        sprintf(mqtt_message, "{\"services\": [{\"service_id\": \"stm32\",\"properties\":{\"DHT11_T\":%.1f,\"DHT11_H\":%.1f,\"MQ2\":%d,\"flame\":%d,\"light\":%d}}]}", DHT11_T,DHT11_H,MQ2,flame,light);
        MQTT_PublishData((char*)POST_TOPIC, mqtt_message, 0);
        std::cout << "发布消息成功"<< std::endl;
        // 使用 std::this_thread::sleep_for
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    close(connectSocket);
    return 0;
}