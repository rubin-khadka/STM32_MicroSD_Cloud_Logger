/*
 * esp8266.h
 *
 *  Created on: Mar 19, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_ESP8266_H_
#define INC_ESP8266_H_

#include <stdint.h>

// THINGSPEAK MQTT CONFIGURATION
#define MQTT_BROKER         "mqtt3.thingspeak.com"
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "NCwODB4IGScaOjM6PQYaDxA"
#define MQTT_USERNAME       "NCwODB4IGScaOjM6PQYaDxA"
#define MQTT_PASSWORD       "+pzQtxMdwTBnIxNmvVLst1tL"
#define MQTT_KEEPALIVE      60

// Thinkspeak Channel IDs
#define CHANNEL_DHT11       "3309559"
#define CHANNEL_MPU6050     "3309560"

// MQTT Topics
#define TOPIC_DHT11         "channels/" CHANNEL_DHT11 "/publish"
#define TOPIC_MPU6050       "channels/" CHANNEL_MPU6050 "/publish"

// ESP8266 Status codes
typedef enum
{
  ESP8266_OK = 0,
  ESP8266_ERROR,
  ESP8266_TIMEOUT,
  ESP8266_NO_RESPONSE,
  ESP8266_BUSY
} ESP8266_Status;

// Connection states
typedef enum
{
  ESP8266_DISCONNECTED = 0,
  ESP8266_CONNECTED_NO_IP,
  ESP8266_CONNECTED_IP
} ESP8266_ConnectionState;

// Helper Functions, other file use it too
uint16_t ESP_StrCopy(char *dest, const char *src, uint16_t n);

// ESP8266 Functions
ESP8266_Status ESP_Init(void);
ESP8266_Status ESP_ConnectWiFi(const char *ssid, const char *password, char *ip_buffer, uint16_t buffer_len);
ESP8266_Status ESP_GetFirmwareVersion(char *buffer, uint16_t buffer_len);
ESP8266_ConnectionState ESP_GetConnectionState(void);

// MQTT Functions
void MQTT_Init(void);
ESP8266_Status ESP_MQTT_Connect(
    const char *broker,
    uint16_t port,
    const char *clientID,
    const char *username,
    const char *password,
    uint16_t keepalive);
ESP8266_Status ESP_MQTT_Publish(const char *topic, const char *message, uint8_t qos);
ESP8266_Status ESP_MQTT_Subscribe(const char *topic, uint8_t qos);
ESP8266_Status ESP_MQTT_Ping(void);
ESP8266_Status ESP_MQTT_HandleIncoming(
    char *topic_buffer,
    uint16_t topic_buf_len,
    char *msg_buffer,
    uint16_t msg_buf_len);

#endif /* INC_ESP8266_H_ */
