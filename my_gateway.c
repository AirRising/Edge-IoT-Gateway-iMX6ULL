#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"

// ================= 华为云配置区 (请填入你的三要素) =================
#define HW_ADDRESS     "tcp://dfc25cbd0f.st1.iotda-device.cn-north-4.myhuaweicloud.com:1883"
#define HW_CLIENTID    "69c51826cbb0cf6bb94ae04a_imx6ull_dht22_01_0_1_2026032612"
#define HW_USER        "69c51826cbb0cf6bb94ae04a_imx6ull_dht22_01"
#define HW_PASS        "fe821199446dcbeb88bab25ed6da7e469324e651a30339a7ae35f0ffd0e13759"
// 华为云属性上报 Topic (固定格式)
#define HW_TOPIC       "$oc/devices/69c51826cbb0cf6bb94ae04a_imx6ull_dht22_01/sys/properties/report"

// ================= 本地开发板配置区 =================
#define LOCAL_ADDRESS  "tcp://localhost:1883" 
#define LOCAL_TOPIC    "sensor/data"

MQTTClient hw_client; // 全局变量，方便在回调函数里使用

// 【核心逻辑】当收到开发板的消息时，自动执行这个函数
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char* payloadptr = (char*)message->payload;
    float t, h;
    
    // 从开发板发的 "25.5,60.2" 中提取数字
    if (sscanf(payloadptr, "%f,%f", &t, &h) != 2) {
        printf("数据格式错误，无法解析: %s\n", payloadptr);
        return 1;
    }

    char hw_json[512];
    // 终极合体 JSON：一个 Service，两个 Properties
    sprintf(hw_json, 
        "{"
          "\"services\": ["
            "{"
              "\"service_id\": \"Data\","
              "\"properties\": {"
                "\"temperature\": %.1f,"     // 这里的名称必须和云端属性名一模一样
                "\"humidity\": %.1f"
              "}"
            "}"
          "]"
        "}", t, h);

    printf("【中转站】合体包上报: %s\n", hw_json);
    
    // 发送给华为云的逻辑保持原样...
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = hw_json;
    pubmsg.payloadlen = (int)strlen(hw_json);
    MQTTClient_publishMessage(hw_client, HW_TOPIC, &pubmsg, NULL);

    return 1;
}

int main() {
    MQTTClient local_client;
    MQTTClient_connectOptions hw_conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connectOptions local_conn_opts = MQTTClient_connectOptions_initializer;

    // A. 连接华为云 (外网)
    MQTTClient_create(&hw_client, HW_ADDRESS, HW_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    hw_conn_opts.username = HW_USER;
    hw_conn_opts.password = HW_PASS;
    hw_conn_opts.keepAliveInterval = 60;
    if (MQTTClient_connect(hw_client, &hw_conn_opts) != MQTTCLIENT_SUCCESS) {
        printf("❌ 连接华为云失败！请检查网络或三要素。\n");
        return -1;
    }
    printf("✅ 成功连接华为云！\n");

    // B. 连接本地 Mosquitto (局域网)
    MQTTClient_create(&local_client, LOCAL_ADDRESS, "Ubuntu_Gateway", MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(local_client, NULL, NULL, msgarrvd, NULL); // 设置收货回调
    if (MQTTClient_connect(local_client, &local_conn_opts) != MQTTCLIENT_SUCCESS) {
        printf("❌ 连接本地 Mosquitto 失败！\n");
        return -1;
    }
    
    // C. 订阅开发板的话题
    MQTTClient_subscribe(local_client, LOCAL_TOPIC, 1);
    printf("🚀 中转站启动成功，等待开发板消息...\n");

    while(1) {
        sleep(1); // 保持运行，不要退出
    }

    return 0;
}