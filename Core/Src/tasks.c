/*
 * tasks.c
 *
 *  Created on: Mar 8, 2026
 *      Author: Rubin Khadka
 */

#include "stm32f103xb.h"
#include "dht11.h"
#include "dwt.h"
#include "lcd.h"
#include "esp8266.h"
#include "usart2.h"
#include "mpu6050.h"
#include "ds3231.h"
#include "button.h"
#include "utils.h"

#define MAX_RETRIES 5

// Global variables to store DHT11 data
volatile float dht11_humidity = 0.0f;
volatile float dht11_temperature = 0.0f;

static uint8_t dht11_humidity1 = 0;
static uint8_t dht11_humidity2 = 0;
static uint8_t dht11_temperature1 = 0;
static uint8_t dht11_temperature2 = 0;

// Helper function to convert float to string without sprintf
static void Float_To_String(float value, char *buffer, uint8_t decimals)
{
  uint8_t is_negative = 0;
  uint32_t int_part;
  uint32_t dec_part;
  uint8_t i = 0;
  uint8_t j = 0;
  char temp[12];

  // Handle negative
  if(value < 0)
  {
    is_negative = 1;
    value = -value;
  }

  // Get integer and decimal parts
  int_part = (uint32_t) value;
  dec_part = (uint32_t) ((value - int_part) * 10);

  // Convert integer part to string (reversed)
  if(int_part == 0)
  {
    temp[i++] = '0';
  }
  else
  {
    while(int_part > 0 && i < sizeof(temp) - 1)
    {
      temp[i++] = '0' + (int_part % 10);
      int_part /= 10;
    }
  }

  // Add negative sign
  if(is_negative)
  {
    buffer[j++] = '-';
  }

  // Reverse integer part into buffer
  while(i > 0)
  {
    buffer[j++] = temp[--i];
  }

  // Add decimal point
  buffer[j++] = '.';

  // Add decimal part
  buffer[j++] = '0' + dec_part;

  // Add null terminator
  buffer[j] = '\0';
}

void Task_DHT11_Read(void)
{
  uint8_t hum1, hum2, temp1, temp2, checksum;

  // Disable interrupts for critical section
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  // Try up to MAX_RETRIES times
  for(int retry = 0; retry < MAX_RETRIES; retry++)
  {
    DHT11_Start();

    if(DHT11_Check_Response())
    {
      hum1 = DHT11_Read();
      hum2 = DHT11_Read();
      temp1 = DHT11_Read();
      temp2 = DHT11_Read();
      checksum = DHT11_Read();

      uint8_t calc = hum1 + hum2 + temp1 + temp2;

      if(calc == checksum)
      {
        dht11_humidity1 = hum1;
        dht11_humidity2 = hum2;
        dht11_temperature1 = temp1;
        dht11_temperature2 = temp2;

        // Humidity: combine integer and decimal parts
        dht11_humidity = (float) hum1 + (float) hum2 / 10.0f;

        // Temperature: check sign bit (0x80) for negative values
        if(temp1 & 0x80)
          dht11_temperature = -((float) (temp1 & 0x7F) + (float) temp2 / 10.0f);
        else
          dht11_temperature = (float) temp1 + (float) temp2 / 10.0f;

        break;
      }
    }
  }

  // Re-enable interrupts
  __set_PRIMASK(primask);
}

// Task to update LCD display
void Task_LCD_Update(void)
{
  DisplayMode_t mode = Button_GetMode();

  switch(mode)
  {
    case DISPLAY_MODE_TEMP_HUM:
      LCD_DisplayReading_Temp(dht11_temperature, dht11_temperature2, dht11_humidity, dht11_humidity2);
      break;

    case DISPLAY_MODE_ACCEL:
      LCD_DisplayAccelScaled(mpu6050_scaled.accel_x, mpu6050_scaled.accel_y, mpu6050_scaled.accel_z);
      break;

    case DISPLAY_MODE_GYRO:
      LCD_DisplayGyroScaled(mpu6050_scaled.gyro_x, mpu6050_scaled.gyro_y, mpu6050_scaled.gyro_z);
      break;

    case DISPLAY_MODE_DATE_TIME:
      // Get current time
      if(DS3231_GetTime(&current_time) == DS3231_OK)
      {
        static char buffer[17];
        // Format and display time on LCD
        FormatTimeString(current_time.hour, current_time.minutes, current_time.seconds, buffer);
        LCD_SetCursor(0, 0);
        LCD_SendString(buffer);

        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");

        // Format and display date
        FormatDateString(current_time.dayofmonth, current_time.month, current_time.year, buffer);
        LCD_SetCursor(1, 0);
        LCD_SendString(buffer);
      }
      break;

    default:  // Handles DISPLAY_MODE_COUNT and any invalid values
      break;
  }
}

// Task to read MPU6050 sensor
void Task_MPU6050_Read(void)
{
  if(MPU6050_ReadAll() == I2C_OK)
  {
    MPU6050_ScaleAll();
  }
}

// Publish DHT11 data to ThingSpeak Channel 3309559
void Task_MQTT_Publish_DHT11(void)
{
  char payload[64];
  char temp_str[10];
  char hum_str[10];
  uint8_t i = 0;

  // Convert floats to strings
  Float_To_String(dht11_temperature, temp_str, 1);
  Float_To_String(dht11_humidity, hum_str, 1);

  // Build: field1=25.5&field2=60.2
  i += ESP_StrCopy(payload + i, "field1=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, temp_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field2=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, hum_str, sizeof(payload) - i);

  USART2_SendString("\r\n[DHT11] Publishing: ");
  USART2_SendString(payload);
  USART2_SendString("\r\n");

  if(ESP_MQTT_Publish(TOPIC_DHT11, payload, 0) == ESP8266_OK)
  {
    USART2_SendString("[DHT11] Sent to ThingSpeak Channel ");
    USART2_SendString(CHANNEL_DHT11);
    USART2_SendString("\r\n");
  }
  else
  {
    USART2_SendString("[DHT11] Publish failed!\r\n");
  }
}

// Publish MPU6050 data to ThingSpeak Channel 3309560
void Task_MQTT_Publish_MPU6050(void)
{
  char payload[128];
  char ax_str[10], ay_str[10], az_str[10];
  char gx_str[10], gy_str[10], gz_str[10];
  uint8_t i = 0;

  // Use the already scaled data from mpu6050_scaled structure
  extern volatile MPU6050_ScaledData_t mpu6050_scaled;

  // Convert to strings (2 decimal places for sensor data)
  Float_To_String(mpu6050_scaled.accel_x, ax_str, 2);
  Float_To_String(mpu6050_scaled.accel_y, ay_str, 2);
  Float_To_String(mpu6050_scaled.accel_z, az_str, 2);
  Float_To_String(mpu6050_scaled.gyro_x, gx_str, 2);
  Float_To_String(mpu6050_scaled.gyro_y, gy_str, 2);
  Float_To_String(mpu6050_scaled.gyro_z, gz_str, 2);

  // Format: field1=X&field2=Y&field3=Z&field4=GX&field5=GY&field6=GZ
  i += ESP_StrCopy(payload + i, "field1=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, ax_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field2=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, ay_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field3=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, az_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field4=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, gx_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field5=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, gy_str, sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, "&field6=", sizeof(payload) - i);
  i += ESP_StrCopy(payload + i, gz_str, sizeof(payload) - i);

  USART2_SendString("\r\n[MPU6050] Publishing: ");
  USART2_SendString(payload);
  USART2_SendString("\r\n");

  if(ESP_MQTT_Publish(TOPIC_MPU6050, payload, 0) == ESP8266_OK)
  {
    USART2_SendString("[MPU6050] Sent to ThingSpeak Channel ");
    USART2_SendString(CHANNEL_MPU6050);
    USART2_SendString("\r\n");
  }
  else
  {
    USART2_SendString("[MPU6050] Publish failed!\r\n");
  }
}
