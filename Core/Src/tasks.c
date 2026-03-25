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
#include "timer2.h"
#include "sd_data_logger.h"

#define MAX_RETRIES 5

// Global variables to store DHT11 data
static float dht11_humidity = 0.0f;
static float dht11_temperature = 0.0f;

// Global variables to store DHT11 data
volatile uint8_t dht11_humidity1 = 0;
volatile uint8_t dht11_humidity2 = 0;
volatile uint8_t dht11_temperature1 = 0;
volatile uint8_t dht11_temperature2 = 0;

// Structure for feedback display
typedef struct
{
  char line1[16];
  char line2[16];
  uint32_t end_time;      // System time when feedback should end
  uint8_t active;         // Feedback currently active
} Feedback_t;

static Feedback_t feedback = {0};

// Show a message on LCD for specified duration
static void Feedback_Show(const char *line1, const char *line2, uint16_t duration_ms)
{
  // Copy line1
  int i = 0;
  while(line1[i] && i < 15)
  {
    feedback.line1[i] = line1[i];
    i++;
  }
  feedback.line1[i] = '\0';

  // Copy line2
  i = 0;
  while(line2[i] && i < 15)
  {
    feedback.line2[i] = line2[i];
    i++;
  }
  feedback.line2[i] = '\0';

  // Calculate end time
  feedback.end_time = TIMER2_GetMillis() + duration_ms;
  feedback.active = 1;

  // Show immediately on LCD
  LCD_Clear();
  LCD_SetCursor(0, 0);
  LCD_SendString(feedback.line1);
  LCD_SetCursor(1, 0);
  LCD_SendString(feedback.line2);
}

// Check if feedback time has expired
void Task_Feedback_Update(void)
{
  if(!feedback.active)
    return;

  uint32_t now = TIMER2_GetMillis();

  // Check if we've reached/passed the end time
  if(now >= feedback.end_time)
  {
    feedback.active = 0;
  }
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
  if(feedback.active)
    return;

  DisplayMode_t mode = Button_GetMode();

  switch(mode)
  {
    case DISPLAY_MODE_TEMP_HUM:
      LCD_DisplayReading_Temp(dht11_temperature1, dht11_temperature2, dht11_humidity1, dht11_humidity2);
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
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");

        // Format and display date
        FormatDateString(current_time.dayofmonth, current_time.month, current_time.year, buffer);
        LCD_SetCursor(1, 0);
        LCD_SendString(buffer);
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
        LCD_SendString("  ");
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
  ftoa(dht11_temperature, temp_str, 1);
  ftoa(dht11_humidity, hum_str, 1);

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

  // Convert to strings
  ftoa(mpu6050_scaled.accel_x, ax_str, 2);
  ftoa(mpu6050_scaled.accel_y, ay_str, 2);
  ftoa(mpu6050_scaled.accel_z, az_str, 2);
  ftoa(mpu6050_scaled.gyro_x, gx_str, 2);
  ftoa(mpu6050_scaled.gyro_y, gy_str, 2);
  ftoa(mpu6050_scaled.gyro_z, gz_str, 2);

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

void Task_Button_Status(void)
{
  // Button 2 - SAVE data
  if(g_button2_pressed)
  {
    if(SD_DataLogger_SaveEntry() == SD_LOGGER_OK)
    {
      USART2_SendString("Logged entry #");
      USART2_SendNumber(entry_count);
      USART2_SendString("\r\n");

      char line2[16] = "ENTRY #";
      char num_str[6];
      itoa_32(SD_DataLogger_GetEntryCount(), num_str);

      // Find the end of "ENTRY #" to append number
      uint8_t i = 6; // length of "ENTRY #"
      uint8_t j = 0;
      while(num_str[j] && i < 15)
      {
        line2[i++] = num_str[j++];
      }
      line2[i] = '\0';

      Feedback_Show("SAVED !!!", line2, 1000);

    }
    else
    {
      USART2_SendString("Save FAILED!\r\n");  // DEBUG
      Feedback_Show("ERROR!", "SAVE FAILED", 1000);
    }
    g_button2_pressed = 0;
  }

  // Button 3 - READ data
  if(g_button3_pressed)
  {
    // Read and display all data from SD card
    uint32_t entries = SD_DataLogger_ReadAll();

    if(entries > 0)
    {
      char feedback_str[16];
      itoa_32(entries, feedback_str);
      Feedback_Show("DATA READ:", feedback_str, 500);
      USART2_SendString("Read ");
      USART2_SendNumber(entries);
      USART2_SendString(" entries from SD card\r\n");
    }
    else
    {
      Feedback_Show("NO DATA", "LOG IS EMPTY", 500);
      USART2_SendString("No data found on SD card!\r\n");
    }
    g_button3_pressed = 0;
  }
}
