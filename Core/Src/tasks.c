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

void Task_LCD_Update(void)
{
  LCD_DisplayReading_Temp(dht11_temperature1, dht11_temperature2, dht11_humidity1, dht11_humidity2);
}

