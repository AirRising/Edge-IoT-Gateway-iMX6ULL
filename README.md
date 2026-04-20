# 基于 I.MX6ULL 与华为云的智能温湿度监控系统

> **项目状态：** 已完工 (Completed)
> **核心技术：** Linux 嵌入式开发、IIO 子系统、MQTT 协议、华为云 IoTDA、C语言多线程网关

## 1. 项目背景
本项目旨在构建一个完整的物联网（IoT）闭环系统。通过 I.MX6ULL 工业级开发板采集 DHT22 传感器数据，利用局域网 MQTT 中转，最终将数据上传至华为云 IoT 平台进行远程监控。

## 2. 核心技术栈
* **硬件平台：** NXP i.MX6ULL (Cortex-A7)
* **操作系统：** 
* **转发层 (Ubuntu)：** 充当中转网关，解析原始数据并封装为华为云标准的 JSON 格式上报。
* **平台层 (华为云)：** 负责设备在线管理、数据影子存储及历史曲线展示。

## 3. 系统架构
系统采用“传感器 - 终端节点 - 云端网关 - 云平台”的四层架构，实现了协议的解耦与高效转发。
* **硬件层：** DHT22 传感器通过单总线（1-Wire）连接至 I.MX6ULL GPIO。
* **采集层 (ARM)：** 基于 Linux IIO 子系统读取传感器数值，通过 Paho MQTT 库上报。
* **转发层 (Ubuntu)：** 充当中转网关，解析原始数据并封装为华为云标准的 JSON 格式上报。
* **平台层 (华为云)：** 负责设备在线管理、数据影子存储及历史曲线展示。

## 4. 硬件连接
| 传感器引脚 | I.MX6ULL 引脚 | 说明 |
| :--- | :--- | :--- |
| VCC | 3.3V | 电源供电 |
| DATA | GPIO1_IO02 (典型) | 需要 10kΩ 上拉电阻 |
| GND | GND | 接地 |

## 5. 软件实现
### 5.1 传感器读取 (Linux驱动)
利用内核自带的 `dht11` 驱动，数据映射在虚拟文件系统：
* 温度路径：`/sys/bus/iio/devices/iio:device0/in_temp_input`
* 湿度路径：`/sys/bus/iio/devices/iio:device0/in_humidityrelative_input`

### 5.2 核心代码逻辑
* **`dht22_report`**: 运行于开发板，负责自动扫描 IIO 节点，定时读取并发送 `temp,humi` 格式字符串。由dht22.c编译而来，使用交叉编译器'arm-linux-gnueabihf-gcc dht22.c -o dht22_report -I 你的目录/paho.mqtt.c-master/src -L 你的目录/paho.mqtt.c-master/build/output -lpaho-mqtt3c'
* * **`my_sender`**: 运行测试，查看是否能连接上华为云服务器并成功传送数据。同样使用交叉编译器
* **`my_gateway`**: 运行于 Ubuntu，负责 MQTT 消息回调、JSON 拼装及华为云鉴权连接。使用gcc即可编译

## 6. 数据格式
**上报 JSON 示例：**
```json
{
  "services": [{
    "service_id": "Data",
    "properties": {
      "temperature": 26.5,
      "humidity": 62.1
    }
  }]
}
```

## 7. 实际运行
* ubuntu运行'./my_gateway'，开发板运行'./dht22_report'
* 华为云IoTDA添加产品-设备-服务

## 8. 项目成果展示
* **实时性：** 数据上报频率 5s/次。
* **稳定性：** 支持传感器重连机制，自动处理 `Connection timed out` 异常。
* **可视化：** 华为云控制台实时在线，支持历史数据曲线回溯。

