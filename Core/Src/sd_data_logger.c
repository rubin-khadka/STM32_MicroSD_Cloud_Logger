/*
 * sd_data_logger.c
 *
 *  Created on: Mar 4, 2026
 *  Author: Rubin Khadka
 */

#include "sd_data_logger.h"
#include "sd_functions.h"
#include "timer2.h"
#include "utils.h"
#include "usart2.h"
#include "mpu6050.h"
#include "tasks.h"

#define CSV_FILENAME        "saved_sensor_data.csv"
#define CSV_HEADER          "Entry,DHT11_C,DHT11_%,AX_g,AY_g,AZ_g,GX_dps,GY_dps,GZ_dps\r\n"
#define MAX_LINE_LENGTH     128

static uint8_t initialized = 0;
volatile uint32_t entry_count = 0;

// Structure to track logger state
static struct
{
  uint32_t total_entries;
  uint8_t file_exists;
} sd_logger;

// Helper: Format float with specified width
static void print_csv_float(char **ptr, float value, uint8_t decimals)
{
  char buffer[16];
  ftoa(value, buffer, decimals);
  char *s = buffer;
  while(*s)
    *(*ptr)++ = *s++;
}

// Helper: Format integer
static void print_csv_int(char **ptr, uint32_t value)
{
  char buffer[16];
  itoa_32(value, buffer);
  char *s = buffer;
  while(*s)
    *(*ptr)++ = *s++;
}

// Format DHT11 data with decimal point (integer part and decimal part)
static void format_dht11_data(char **ptr, uint8_t integer_part, uint8_t decimal_part)
{
  char buffer[8];
  char *buf_ptr = buffer;

  // Integer part
  if(integer_part >= 10)
  {
    *buf_ptr++ = '0' + (integer_part / 10);
  }
  *buf_ptr++ = '0' + (integer_part % 10);

  // Decimal point
  *buf_ptr++ = '.';

  // Decimal part (tenths)
  *buf_ptr++ = '0' + decimal_part;
  *buf_ptr = '\0';

  // Copy to output
  char *s = buffer;
  while(*s)
    *(*ptr)++ = *s++;
}

// Helper: for proper table formatting in UART
static void print_fixed_width(float value, uint8_t width, uint8_t decimals)
{
  char buffer[12];
  ftoa(value, buffer, decimals);

  // Calculate current length
  uint8_t len = 0;
  while(buffer[len])
    len++;

  // Print spaces for alignment
  for(uint8_t i = len; i < width; i++)
  {
    USART2_SendChar(' ');
  }

  // Print the number
  USART2_SendString(buffer);
}

// Helper: String to float converter
static float simple_atof(char *str)
{
  float result = 0.0f;
  float fraction = 0.0f;
  int sign = 1;
  int i = 0;

  // Handle sign
  if(str[0] == '-')
  {
    sign = -1;
    i = 1;
  }

  // Integer part
  while(str[i] >= '0' && str[i] <= '9')
  {
    result = result * 10.0f + (str[i] - '0');
    i++;
  }

  // Fraction part
  if(str[i] == '.')
  {
    i++;
    float divider = 10.0f;
    while(str[i] >= '0' && str[i] <= '9')
    {
      fraction = fraction + (str[i] - '0') / divider;
      divider *= 10.0f;
      i++;
    }
  }

  return sign * (result + fraction);
}

static char* csv_tokenize(char *str, char delimiter, char **next)
{
  if(str == NULL || *str == '\0')
    return NULL;

  // Skip leading delimiters
  while(*str == delimiter)
    str++;

  if(*str == '\0')
    return NULL;

  char *token_start = str;

  // Find end of token
  while(*str && *str != delimiter && *str != '\r' && *str != '\n')
    str++;

  // If we found a delimiter or end of line
  if(*str)
  {
    *str = '\0';  // Terminate the token
    *next = str + 1;  // Point to next character
  }
  else
  {
    *next = NULL;  // End of string
  }

  return token_start;
}

// Initialize SD data logger
uint8_t SD_DataLogger_Init(void)
{
  USART2_SendString("Initializing SD Data Logger...\r\n");

  // First, mount the SD card
  if(sd_mount() != FR_OK)
  {
    USART2_SendString("SD Card mount failed!\r\n");
    return SD_LOGGER_ERROR;
  }

  // Check if CSV file exists and count entries
  FIL file;
  uint32_t lines = 0;

  FRESULT res = f_open(&file, CSV_FILENAME, FA_READ);
  if(res == FR_OK)
  {
    // File exists - count lines by reading characters until EOF
    UINT bytes_read;
    uint8_t temp_buf[32];
    uint8_t last_char = 0;

    while(1)
    {
      res = f_read(&file, temp_buf, 1, &bytes_read);
      if(res != FR_OK || bytes_read == 0)
        break;

      if(temp_buf[0] == '\n')
      {
        lines++;
      }
      last_char = temp_buf[0];
    }

    // If file doesn't end with newline, add one to line count
    if(last_char != '\n' && lines > 0)
    {
      lines++;
    }

    f_close(&file);

    // Subtract header line
    if(lines > 0)
    {
      entry_count = lines - 1;
      sd_logger.file_exists = 1;
    }

    USART2_SendString("Found existing CSV with ");
    USART2_SendNumber(entry_count);
    USART2_SendString(" data entries\r\n");
  }
  else
  {
    // File doesn't exist - create with header
    USART2_SendString("No CSV file found. Creating new file...\r\n");

    if(sd_write_file(CSV_FILENAME, CSV_HEADER) == FR_OK)
    {
      USART2_SendString("CSV header created successfully\r\n");
      entry_count = 0;
      sd_logger.file_exists = 1;
    }
    else
    {
      USART2_SendString("Failed to create CSV file!\r\n");
      sd_unmount();
      return SD_LOGGER_ERROR;
    }
  }

  sd_logger.total_entries = entry_count;
  initialized = 1;

  USART2_SendString("SD Data Logger ready. Total entries: ");
  USART2_SendNumber(entry_count);
  USART2_SendString("\r\n");

  return SD_LOGGER_OK;
}

// Save current sensor data to CSV
uint8_t SD_DataLogger_SaveEntry(void)
{
  if(!initialized)
  {
    return SD_LOGGER_UNINIT;
  }

  char csv_line[MAX_LINE_LENGTH];
  char *ptr = csv_line;

  // Entry number
  print_csv_int(&ptr, entry_count + 1);  // Start from 1
  *ptr++ = ',';

  // DHT11 temperature with decimal point
  format_dht11_data(&ptr, dht11_temperature1, dht11_temperature2);
  *ptr++ = ',';

  // DHT11 humidity with decimal point
  format_dht11_data(&ptr, dht11_humidity1, dht11_humidity2);
  *ptr++ = ',';

  // Accelerometer X (float with 3 decimals for higher precision)
  print_csv_float(&ptr, mpu6050_scaled.accel_x, 3);
  *ptr++ = ',';

  // Accelerometer Y
  print_csv_float(&ptr, mpu6050_scaled.accel_y, 3);
  *ptr++ = ',';

  // Accelerometer Z
  print_csv_float(&ptr, mpu6050_scaled.accel_z, 3);
  *ptr++ = ',';

  // Gyro X (float with 2 decimals)
  print_csv_float(&ptr, mpu6050_scaled.gyro_x, 2);
  *ptr++ = ',';

  // Gyro Y
  print_csv_float(&ptr, mpu6050_scaled.gyro_y, 2);
  *ptr++ = ',';

  // Gyro Z
  print_csv_float(&ptr, mpu6050_scaled.gyro_z, 2);

  // End line
  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';

  // Append to CSV file
  if(sd_append_file(CSV_FILENAME, csv_line) != FR_OK)
  {
    USART2_SendString("Failed to append to CSV!\r\n");
    return SD_LOGGER_ERROR;
  }

  entry_count++;
  sd_logger.total_entries = entry_count;

  return SD_LOGGER_OK;
}

// Get entry count
uint32_t SD_DataLogger_GetEntryCount(void)
{
  return entry_count;
}
