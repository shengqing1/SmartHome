#include "mqtt.h"

// 全局变量定义
unsigned char mqtt_rxbuf[1024 * 1024];
unsigned char mqtt_txbuf[256];
unsigned int mqtt_rxlen;
unsigned int mqtt_txlen;

// 定义MQTT控制报文的固定响应数据
const unsigned char parket_connetAck[] = {0x20, 0x02, 0x00, 0x00}; // 连接成功响应
const unsigned char parket_disconnet[] = {0xe0, 0x00};             // 断开连接控制报文
const unsigned char parket_heart[] = {0xc0, 0x00};                 // 心跳请求控制报文
const unsigned char parket_heart_reply[] = {0xc0, 0x00};           // 心跳响应控制报文
const unsigned char parket_subAck[] = {0x90, 0x03};                // 订阅成功响应
int connectSocket;                                                 // 网络套接字

// MQTT协议缓冲区初始化函数
void MQTT_Init(void)
{
    mqtt_rxlen = sizeof(mqtt_rxbuf);   // 初始化接收缓存区长度
    mqtt_txlen = sizeof(mqtt_txbuf);   // 初始化发送缓存区长度
    memset(mqtt_rxbuf, 0, mqtt_rxlen); // 清零接收缓存区
    memset(mqtt_txbuf, 0, mqtt_txlen); // 清零发送缓存区
}

/*
 * 函数功能: 连接MQTT服务器
 * 函数参数: ClientID - 客户端ID, Username - 用户名, Password - 密码
 * 函数返回值: 0表示成功, 1表示失败
 */
unsigned char MQTT_Connect(char *ClientID, char *Username, char *Password)
{
    unsigned short i, j;
    int ClientIDLen = (int)strlen(ClientID);
    int UsernameLen = (int)strlen(Username);
    int PasswordLen = (int)strlen(Password);
    unsigned int DataLen;
    mqtt_txlen = 0;
    unsigned int size = 0;
    unsigned char buff[256];

    // 可变报头+Payload  每个字段包含两个字节的长度标识
    DataLen = 10 + (ClientIDLen + 2) + (UsernameLen + 2) + (PasswordLen + 2);

    // 固定报头
    // 控制报文类型
    mqtt_txbuf[mqtt_txlen++] = 0x10; // MQTT Message Type CONNECT

    // 剩余长度(不包括固定头部)
    do
    {

        unsigned char encodedByte = DataLen % 128;
        DataLen = DataLen / 128;
        // if there are more data to encode, set the top bit of this byte
        if (DataLen > 0)
            encodedByte = encodedByte | 128;
        mqtt_txbuf[mqtt_txlen++] = encodedByte;
    } while (DataLen > 0);

    // 可变报头
    // 协议名
    mqtt_txbuf[mqtt_txlen++] = 0;   // Protocol Name Length MSB
    mqtt_txbuf[mqtt_txlen++] = 4;   // Protocol Name Length LSB
    mqtt_txbuf[mqtt_txlen++] = 'M'; // ASCII Code for M
    mqtt_txbuf[mqtt_txlen++] = 'Q'; // ASCII Code for Q
    mqtt_txbuf[mqtt_txlen++] = 'T'; // ASCII Code for T
    mqtt_txbuf[mqtt_txlen++] = 'T'; // ASCII Code for T
    // 协议级别
    mqtt_txbuf[mqtt_txlen++] = 4; // MQTT Protocol version = 4
    // 连接标志
    mqtt_txbuf[mqtt_txlen++] = 0xc2; // conn flags
    mqtt_txbuf[mqtt_txlen++] = 0;    // Keep-alive Time Length MSB
    mqtt_txbuf[mqtt_txlen++] = 100;  // Keep-alive Time Length LSB  100S心跳包

    mqtt_txbuf[mqtt_txlen++] = BYTE1(ClientIDLen); // Client ID length MSB
    mqtt_txbuf[mqtt_txlen++] = BYTE0(ClientIDLen); // Client ID length LSB
    memcpy(&mqtt_txbuf[mqtt_txlen], ClientID, ClientIDLen);
    mqtt_txlen += ClientIDLen;
    if (UsernameLen > 0)
    {
        mqtt_txbuf[mqtt_txlen++] = BYTE1(UsernameLen); // username length MSB
        mqtt_txbuf[mqtt_txlen++] = BYTE0(UsernameLen); // username length LSB
        memcpy(&mqtt_txbuf[mqtt_txlen], Username, UsernameLen);
        mqtt_txlen += UsernameLen;
    }

    if (PasswordLen > 0)
    {
        mqtt_txbuf[mqtt_txlen++] = BYTE1(PasswordLen); // password length MSB
        mqtt_txbuf[mqtt_txlen++] = BYTE0(PasswordLen); // password length LSB
        memcpy(&mqtt_txbuf[mqtt_txlen], Password, PasswordLen);
        mqtt_txlen += PasswordLen;
    }

    for (i = 0; i < 5; i++)
    {
        memset(mqtt_rxbuf, 0, mqtt_rxlen);
        MQTT_SendBuf(mqtt_txbuf, mqtt_txlen);
        size = Client_GetData(buff); // 从服务器获取数据
        if (size <= 0)
            continue;
        memcpy(mqtt_rxbuf, buff, size);

        printf("登录应答:\r\n");
        for (j = 0; j < size; j++)
        {
            printf("%#X ", buff[j]);
        }

        printf("\r\n");
        if (mqtt_rxbuf[0] == parket_connetAck[0] && mqtt_rxbuf[1] == parket_connetAck[1]) // 连接成功
        {
            return 0; // 连接成功
        }
    }
    return 1;
}

/*
 * 函数功能: 订阅或取消订阅MQTT主题
 * 函数参数: topic - 主题, qos - 服务质量等级, whether - 是否订阅（1订阅，0取消订阅）
 * 函数返回值: 0表示成功, 1表示失败
 */
unsigned char MQTT_SubscribeTopic(char *topic, unsigned char qos, unsigned char whether)
{
    unsigned char i, j;
    mqtt_txlen = 0;
    unsigned int size = 0;
    unsigned char buff[256];
    unsigned int topiclen = (int)strlen(topic);
    unsigned int DataLen = 2 + (topiclen + 2) + (whether ? 1 : 0); // 可变报头的长度（2字节）加上有效载荷的长度
    // 固定报头
    // 控制报文类型
    if (whether)
        mqtt_txbuf[mqtt_txlen++] = 0x82; // 消息类型和标志订阅
    else
        mqtt_txbuf[mqtt_txlen++] = 0xA2; // 取消订阅
    // 剩余长度
    do
    {
        unsigned char encodedByte = DataLen % 128;
        DataLen = DataLen / 128;
        // if there are more data to encode, set the top bit of this byte
        if (DataLen > 0)
            encodedByte = encodedByte | 128;
        mqtt_txbuf[mqtt_txlen++] = encodedByte;
    } while (DataLen > 0);
    // 可变报头
    mqtt_txbuf[mqtt_txlen++] = 0;    // 消息标识符 MSB
    mqtt_txbuf[mqtt_txlen++] = 0x01; // 消息标识符 LSB
    // 有效载荷
    mqtt_txbuf[mqtt_txlen++] = BYTE1(topiclen); // 主题长度 MSB
    mqtt_txbuf[mqtt_txlen++] = BYTE0(topiclen); // 主题长度 LSB
    memcpy(&mqtt_txbuf[mqtt_txlen], topic, topiclen);
    mqtt_txlen += topiclen;
    if (whether)
    {
        mqtt_txbuf[mqtt_txlen++] = qos; // QoS级别
    }
    for (i = 0; i < 100; i++)
    {
        memset(mqtt_rxbuf, 0, mqtt_rxlen);
        MQTT_SendBuf(mqtt_txbuf, mqtt_txlen);
        // printf("订阅消息发布成功\n");
        size = Client_GetData(buff); // 从服务器获取数据
        if (size <= 0)
        {
            continue;
        }
        memcpy(mqtt_rxbuf, buff, size);

        printf("订阅应答:\r\n");
        for (j = 0; j < size; j++)
        {
            printf("%#X ", buff[j]);
        }
        printf("\r\n");

        if (mqtt_rxbuf[0] == parket_subAck[0] && mqtt_rxbuf[1] == parket_subAck[1]) // 连接成功
        {
            return 0; // 连接成功
        }
        sleep(1);
    }
    return 1; // 失败
}

// MQTT发布数据打包函数
// topic   主题
// message 消息
// qos     消息等级
unsigned char MQTT_PublishData(char *topic, char *message, unsigned char qos)
{
    unsigned int topicLength = (int)strlen(topic);
    unsigned int messageLength = (int)strlen(message);
    unsigned short id = 0;
    unsigned int DataLen;
    mqtt_txlen = 0;

    printf("上报JSON消息长度:%d\r\n", messageLength);
    printf("message=%s\r\n", message);
    std::cout << "test"<< std::endl;
    // 有效载荷的长度这样计算：用固定报头中的剩余长度字段的值减去可变报头的长度
    // QOS为0时没有标识符
    // 数据长度             主题名   报文标识符   有效载荷
    if (qos)
        DataLen = (2 + topicLength) + 2 + messageLength;
    else
        DataLen = (2 + topicLength) + messageLength;

    // 固定报头
    // 控制报文类型
    mqtt_txbuf[mqtt_txlen++] = 0x30; // MQTT Message Type PUBLISH

    // 剩余长度
    do
    {
        unsigned char encodedByte = DataLen % 128;
        DataLen = DataLen / 128;
        // if there are more data to encode, set the top bit of this byte
        if (DataLen > 0)
            encodedByte = encodedByte | 128;
        mqtt_txbuf[mqtt_txlen++] = encodedByte;
    } while (DataLen > 0);
    mqtt_txbuf[mqtt_txlen++] = BYTE1(topicLength);       // 主题长度MSB
    mqtt_txbuf[mqtt_txlen++] = BYTE0(topicLength);       // 主题长度LSB
    memcpy(&mqtt_txbuf[mqtt_txlen], topic, topicLength); // 拷贝主题
    mqtt_txlen += topicLength;

    // 报文标识符
    if (qos)
    {
        mqtt_txbuf[mqtt_txlen++] = BYTE1(id);
        mqtt_txbuf[mqtt_txlen++] = BYTE0(id);
        id++;
    }
    memcpy(&mqtt_txbuf[mqtt_txlen], message, messageLength);
    mqtt_txlen += messageLength;

    MQTT_SendBuf(mqtt_txbuf, mqtt_txlen);
    return mqtt_txlen;
}

// 调用底层接口发送数据包
void MQTT_SendBuf(unsigned char *buf, unsigned short len)
{
    Client_SendData(buf, len); // 发送数据到服务器
}
// 发送数据到服务器
int Client_SendData(unsigned char *buff, unsigned int len)
{
    int result = send(connectSocket, buff, len, 0);
    if (result < 0)
    {
        std::cerr << "send failed with error: " << strerror(errno) << std::endl;
        return -1;
    }
    return 0;
}

// 从服务器获取数据
int Client_GetData(unsigned char *buff)
{
    int result = recv(connectSocket, buff, 200, 0);
    if (result < 0)
    {
        std::cerr << "recv failed with error: " << strerror(errno) << std::endl;
        return -1;
    }
    else if (result == 0)
    {
        std::cerr << "Connection closed by server" << std::endl;
        return -1;
    }
    return result;
}