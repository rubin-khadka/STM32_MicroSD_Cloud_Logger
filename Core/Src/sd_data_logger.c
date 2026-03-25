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
#include "ds3231.h"

#define CSV_FILENAME        "sensor_data.csv"
#define CSV_HEADER          "DateTime,DHT11_C,DHT11_%,AX_g,AY_g,AZ_g,GX_dps,GY_dps,GZ_dps\r\n"
#define MAX_LINE_LENGTH     128

static uint8_t initialized = 0;
volatile uint32_t entry_count = 0;

// Structure to track logger state
static struct
{
  uint32_t total_entries;
  uint8_t file_exists;
} sd_logger;

// Helper: Format DHT11 data with decimal point (integer part and decimal part)
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
  ftoa(value, buffer, decimals);  // Use ftoa from utils.c

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

// CSV tokenizer
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

// Save current sensor data to CSV with timestamp
uint8_t SD_DataLogger_SaveEntry(void)
{
  if(!initialized)
  {
    return SD_LOGGER_UNINIT;
  }

  DS3231_Time_t current_time;
  char csv_line[MAX_LINE_LENGTH];
  char *ptr = csv_line;
  char timestamp[20];
  char temp_buffer[16];

  // Get current timestamp from DS3231
  if(DS3231_GetTime(&current_time) != DS3231_OK)
  {
    USART2_SendString("Failed to get current time!\r\n");
    return SD_LOGGER_ERROR;
  }

  // Use FormatTimestamp from utils
  FormatTimestamp(&current_time, timestamp, sizeof(timestamp));

  // Copy timestamp to CSV line
  char *ts = timestamp;
  while(*ts)
    *ptr++ = *ts++;
  *ptr++ = ',';

  // DHT11 temperature with decimal point
  format_dht11_data(&ptr, dht11_temperature1, dht11_temperature2);
  *ptr++ = ',';

  // DHT11 humidity with decimal point
  format_dht11_data(&ptr, dht11_humidity1, dht11_humidity2);
  *ptr++ = ',';

  // Use ftoa from utils for accelerometer data
  ftoa(mpu6050_scaled.accel_x, temp_buffer, 3);
  char *s = temp_buffer;
  while(*s)
    *ptr++ = *s++;
  *ptr++ = ',';

  ftoa(mpu6050_scaled.accel_y, temp_buffer, 3);
  s = temp_buffer;
  while(*s)
    *ptr++ = *s++;
  *ptr++ = ',';

  ftoa(mpu6050_scaled.accel_z, temp_buffer, 3);
  s = temp_buffer;
  while(*s)
    *ptr++ = *s++;
  *ptr++ = ',';

  // Use ftoa from utils for gyroscope data
  ftoa(mpu6050_scaled.gyro_x, temp_buffer, 2);
  s = temp_buffer;
  while(*s)
    *ptr++ = *s++;
  *ptr++ = ',';

  ftoa(mpu6050_scaled.gyro_y, temp_buffer, 2);
  s = temp_buffer;
  while(*s)
    *ptr++ = *s++;
  *ptr++ = ',';

  ftoa(mpu6050_scaled.gyro_z, temp_buffer, 2);
  s = temp_buffer;
  while(*s)
    *ptr++ = *s++;

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

// Periodic task to log data
void Task_SD_DataLogger(void)
{
  USART2_SendString("Task_SD_DataLogger called\r\n");
  if(!initialized)
  {
    USART2_SendString("Not initialized!\r\n");
    return;
  }

  if(SD_DataLogger_SaveEntry() == SD_LOGGER_OK)
  {
    USART2_SendString("Logged entry #");
    USART2_SendNumber(entry_count);
    USART2_SendString("\r\n");
  }
  else
  {
    USART2_SendString("Save FAILED!\r\n");
  }
}

// Read all data from SD card and display with timestamp
uint32_t SD_DataLogger_ReadAll(void)
{
  if(!initialized)
  {
    USART2_SendString("Logger not initialized!\r\n");
    return 0;
  }

  FIL file;
  char line[MAX_LINE_LENGTH];
  uint32_t lines_read = 0;
  uint32_t data_line = 0;

  FRESULT res = f_open(&file, CSV_FILENAME, FA_READ);
  if(res != FR_OK)
  {
    USART2_SendString("Failed to open CSV file! Error: ");
    USART2_SendNumber(res);
    USART2_SendString("\r\n");
    return 0;
  }

  // Print beautiful table header with timestamp
  USART2_SendString("\r\n");
  USART2_SendString(
      "+---------------------+----------+----------+----------+----------+----------+----------+----------+----------+\r\n");
  USART2_SendString(
      "|     Timestamp       | DHT11_C  | DHT11_%  |  AX(g)   |  AY(g)   |  AZ(g)   | GX(dps)  | GY(dps)  | GZ(dps)  |\r\n");
  USART2_SendString(
      "+---------------------+----------+----------+----------+----------+----------+----------+----------+----------+\r\n");

  // Skip header line
  f_gets(line, sizeof(line), &file);

  // Read and parse each data line
  while(f_gets(line, sizeof(line), &file))
  {
    // Remove newline characters manually
    int len = 0;
    while(line[len] && line[len] != '\n' && line[len] != '\r')
      len++;
    line[len] = '\0';

    // Parse CSV
    char *next = line;
    char *token;
    char *timestamp_str = NULL;
    float dht_temp = 0, dht_hum = 0;
    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;

    int col = 0;

    // Get timestamp as string (first column)
    timestamp_str = csv_tokenize(next, ',', &next);

    // Get remaining values
    while(col < 8 && (token = csv_tokenize(next, ',', &next)) != NULL)
    {
      float val = simple_atof(token);

      switch(col)
      {
        case 0:
          dht_temp = val;
          break;
        case 1:
          dht_hum = val;
          break;
        case 2:
          ax = val;
          break;
        case 3:
          ay = val;
          break;
        case 4:
          az = val;
          break;
        case 5:
          gx = val;
          break;
        case 6:
          gy = val;
          break;
        case 7:
          gz = val;
          break;
      }
      col++;
    }

    // Only process if we got all 9 columns (timestamp + 8 values)
    if(timestamp_str != NULL && col == 8)
    {
      data_line++;

      // Print row
      USART2_SendString("| ");

      // Timestamp (width 19, fixed format "YYYY-MM-DD HH:MM:SS")
      USART2_SendString(timestamp_str);

      // Pad to exactly 19 characters (timestamp should be exactly that length)
      int timestamp_len = 0;
      while(timestamp_str[timestamp_len])
        timestamp_len++;
      for(int i = timestamp_len; i < 19; i++)
        USART2_SendChar(' ');

      USART2_SendString(" | ");

      // DHT11 temperature (1 decimal)
      print_fixed_width(dht_temp, 8, 1);
      USART2_SendString(" | ");

      // DHT11 humidity (1 decimal)
      print_fixed_width(dht_hum, 8, 1);
      USART2_SendString(" | ");

      // Accelerometer X (3 decimals)
      print_fixed_width(ax, 8, 3);
      USART2_SendString(" | ");

      // Accelerometer Y (3 decimals)
      print_fixed_width(ay, 8, 3);
      USART2_SendString(" | ");

      // Accelerometer Z (3 decimals)
      print_fixed_width(az, 8, 3);
      USART2_SendString(" | ");

      // Gyro X (2 decimals)
      print_fixed_width(gx, 8, 2);
      USART2_SendString(" | ");

      // Gyro Y (2 decimals)
      print_fixed_width(gy, 8, 2);
      USART2_SendString(" | ");

      // Gyro Z (2 decimals)
      print_fixed_width(gz, 8, 2);
      USART2_SendString(" | ");

      USART2_SendString("\r\n");
      lines_read++;
    }
  }

  // Print table footer
  USART2_SendString(
      "+---------------------+----------+----------+----------+----------+----------+----------+----------+----------+\r\n");
  USART2_SendString("Total: ");
  USART2_SendNumber(lines_read);
  USART2_SendString(" entries\r\n\r\n");

  f_close(&file);
  return lines_read;
}
