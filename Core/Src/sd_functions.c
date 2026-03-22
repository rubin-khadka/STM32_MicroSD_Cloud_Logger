/*
 * sd_functions.c
 *
 *  Created on: Mar 3, 2026
 *      Author: Rubin Khadka
 */

#include "sd_spi.h"
#include "sd_diskio.h"
#include "fatfs.h"
#include "ff.h"
#include "ffconf.h"
#include "usart2.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char sd_path[4];
FATFS fs;

int sd_get_space_kb(void)
{
  FATFS *pfs;
  DWORD fre_clust, tot_sect, fre_sect, total_kb, free_kb;
  FRESULT res = f_getfree(sd_path, &fre_clust, &pfs);
  if(res != FR_OK)
    return res;

  tot_sect = (pfs->n_fatent - 2) * pfs->csize;
  fre_sect = fre_clust * pfs->csize;
  total_kb = tot_sect / 2;
  free_kb = fre_sect / 2;

  char buffer[64];
  char num_buffer[16];
  char *ptr = buffer;

  *ptr++ = 'T';
  *ptr++ = 'o';
  *ptr++ = 't';
  *ptr++ = 'a';
  *ptr++ = 'l';
  *ptr++ = ':';
  *ptr++ = ' ';

  itoa_32(total_kb, num_buffer);
  for(char *s = num_buffer; *s; s++)
    *ptr++ = *s;

  *ptr++ = ' ';
  *ptr++ = 'K';
  *ptr++ = 'B';
  *ptr++ = ',';
  *ptr++ = ' ';
  *ptr++ = 'F';
  *ptr++ = 'r';
  *ptr++ = 'e';
  *ptr++ = 'e';
  *ptr++ = ':';
  *ptr++ = ' ';

  itoa_32(free_kb, num_buffer);
  for(char *s = num_buffer; *s; s++)
    *ptr++ = *s;

  *ptr++ = ' ';
  *ptr++ = 'K';
  *ptr++ = 'B';
  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';

  USART2_SendString(buffer);
  return FR_OK;
}

int sd_mount(void)
{
  FRESULT res;
  extern uint8_t SD_IsSDHC(void);

  if(FATFS_LinkDriver((Diskio_drvTypeDef*) &SD_Driver, sd_path) != 0)
  {
    USART2_SendString("FATFS_LinkDriver failed\n");
    return FR_DISK_ERR;
  }

  DSTATUS stat = disk_initialize(0);
  if(stat != 0)
  {
    USART2_SendString("disk_initialize failed: 0x");
    USART2_SendHex(stat);
    USART2_SendString("\r\n");
    return FR_NOT_READY;
  }

  res = f_mount(&fs, sd_path, 1);
  if(res == FR_OK)
  {
    USART2_SendString("SD card mounted successfully at ");
    USART2_SendString(sd_path);
    USART2_SendString("\r\n");
    USART2_SendString("Card Type: ");
    USART2_SendString(SD_IsSDHC() ? "SDHC/SDXC" : "SDSC");
    USART2_SendString("\r\n");
    sd_get_space_kb();
    return FR_OK;
  }

  // Handle no filesystem
  if(res == FR_NO_FILESYSTEM)
  {
    USART2_SendString("No filesystem found. Attempting format...\r\n");

    // Unmount first
    f_mount(NULL, sd_path, 1);

    // TRY DIFFERENT FORMAT OPTIONS:

    // Option A: Simple format with default parameters
    USART2_SendString("Trying format with default parameters...\r\n");
    res = f_mkfs(sd_path, 0, 0);

    if(res != FR_OK)
    {
      // Option B: Try with explicit sector size (512 bytes is standard)
      USART2_SendString("Default format failed (");
      USART2_SendNumber(res);
      USART2_SendString("). Trying with 512-byte sectors...\r\n");
      res = f_mkfs(sd_path, 0, 512);
    }

    if(res != FR_OK)
    {
      // Option C: Try with 4096-byte sectors (better for large cards)
      USART2_SendString("512-byte format failed (");
      USART2_SendNumber(res);
      USART2_SendString("). Trying with 4096-byte sectors...\r\n");
      res = f_mkfs(sd_path, 0, 4096);
    }

    if(res != FR_OK)
    {
      USART2_SendString("All format attempts failed. Last error: ");
      USART2_SendNumber(res);
      USART2_SendString("\r\n");
      return res;
    }

    USART2_SendString("Format successful! Remounting...\r\n");

    // Try mounting again
    res = f_mount(&fs, sd_path, 1);
    if(res == FR_OK)
    {
      USART2_SendString("SD card formatted and mounted successfully.\r\n");
      USART2_SendString("Card Type: ");
      USART2_SendString(SD_IsSDHC() ? "SDHC/SDXC" : "SDSC");
      USART2_SendString("\r\n");
      sd_get_space_kb();
    }
    else
    {
      USART2_SendString("Mount failed even after format: ");
      USART2_SendNumber(res);
      USART2_SendString("\r\n");
    }
    return res;
  }

  USART2_SendString("Mount failed with code: ");
  USART2_SendNumber(res);
  USART2_SendString("\r\n");
  return res;
}

int sd_unmount(void)
{
  FRESULT res = f_mount(NULL, sd_path, 1);
  USART2_SendString("SD card unmounted: ");
  USART2_SendString((res == FR_OK) ? "OK" : "Failed");
  USART2_SendString("\r\n");
  return res;
}

int sd_write_file(const char *filename, const char *text)
{
  FIL file;
  UINT bw;
  FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if(res != FR_OK)
    return res;

  res = f_write(&file, text, strlen(text), &bw);

  if(res == FR_OK)
  {
    f_sync(&file);
  }

  f_close(&file);

  char buffer[64];
  char num_buffer[16];
  char *ptr = buffer;

  *ptr++ = 'W';
  *ptr++ = 'r';
  *ptr++ = 'i';
  *ptr++ = 't';
  *ptr++ = 'e';
  *ptr++ = ' ';

  itoa_32(bw, num_buffer);
  for(char *s = num_buffer; *s; s++)
    *ptr++ = *s;

  *ptr++ = ' ';
  *ptr++ = 'b';
  *ptr++ = 'y';
  *ptr++ = 't';
  *ptr++ = 'e';
  *ptr++ = 's';
  *ptr++ = ' ';
  *ptr++ = 't';
  *ptr++ = 'o';
  *ptr++ = ' ';

  while(*filename)
    *ptr++ = *filename++;

  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';

  USART2_SendString(buffer);
  return (res == FR_OK && bw == strlen(text)) ? FR_OK : FR_DISK_ERR;
}

int sd_append_file(const char *filename, const char *text)
{
  FIL file;
  UINT bw;
  FRESULT res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
  if(res != FR_OK)
    return res;

  res = f_lseek(&file, f_size(&file));
  if(res != FR_OK)
  {
    f_close(&file);
    return res;
  }

  res = f_write(&file, text, strlen(text), &bw);
  f_close(&file);

  char buffer[64];
  char num_buffer[16];
  char *ptr = buffer;

  *ptr++ = 'A';
  *ptr++ = 'p';
  *ptr++ = 'p';
  *ptr++ = 'e';
  *ptr++ = 'n';
  *ptr++ = 'd';
  *ptr++ = 'e';
  *ptr++ = 'd';
  *ptr++ = ' ';

  itoa_32(bw, num_buffer);
  for(char *s = num_buffer; *s; s++)
    *ptr++ = *s;

  *ptr++ = ' ';
  *ptr++ = 'b';
  *ptr++ = 'y';
  *ptr++ = 't';
  *ptr++ = 'e';
  *ptr++ = 's';
  *ptr++ = ' ';
  *ptr++ = 't';
  *ptr++ = 'o';
  *ptr++ = ' ';

  while(*filename)
    *ptr++ = *filename++;

  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';

  USART2_SendString(buffer);
  return (res == FR_OK && bw == strlen(text)) ? FR_OK : FR_DISK_ERR;
}

int sd_read_file(const char *filename, char *buffer, UINT bufsize, UINT *bytes_read)
{
  FIL file;
  *bytes_read = 0;

  FRESULT res = f_open(&file, filename, FA_READ);
  if(res != FR_OK)
  {
    USART2_SendString("f_open failed with code: ");
    USART2_SendNumber(res);
    USART2_SendString("\r\n");
    return res;
  }

  USART2_SendString("File Opened: ");
  USART2_SendString(filename);
  USART2_SendString("\r\n");

  res = f_read(&file, buffer, bufsize - 1, bytes_read);
  if(res != FR_OK)
  {
    USART2_SendString("f_read failed with code: ");
    USART2_SendNumber(res);
    USART2_SendString("\r\n");
    f_close(&file);
    return res;
  }

  buffer[*bytes_read] = '\0';

  res = f_close(&file);
  if(res != FR_OK)
  {
    USART2_SendString("f_close failed with code: ");
    USART2_SendNumber(res);
    USART2_SendString("\r\n");
    return res;
  }

  char output[64];
  char num_buffer[16];
  char *ptr = output;

  *ptr++ = 'R';
  *ptr++ = 'e';
  *ptr++ = 'a';
  *ptr++ = 'd';
  *ptr++ = ' ';

  itoa_32(*bytes_read, num_buffer);
  for(char *s = num_buffer; *s; s++)
    *ptr++ = *s;

  *ptr++ = ' ';
  *ptr++ = 'b';
  *ptr++ = 'y';
  *ptr++ = 't';
  *ptr++ = 'e';
  *ptr++ = 's';
  *ptr++ = ' ';
  *ptr++ = 'f';
  *ptr++ = 'r';
  *ptr++ = 'o';
  *ptr++ = 'm';
  *ptr++ = ' ';

  while(*filename)
    *ptr++ = *filename++;

  *ptr++ = '\r';
  *ptr++ = '\n';
  *ptr = '\0';

  return FR_OK;
}

typedef struct CsvRecord
{
  char field1[32];
  char field2[32];
  int value;
} CsvRecord;

int sd_read_csv(const char *filename, CsvRecord *records, int max_records, int *record_count)
{
  FIL file;
  char line[128];
  *record_count = 0;

  FRESULT res = f_open(&file, filename, FA_READ);
  if(res != FR_OK)
  {
    USART2_SendString("Failed to open CSV: ");
    USART2_SendString(filename);
    USART2_SendString(" (");
    USART2_SendNumber(res);
    USART2_SendString(")\r\n");
    return res;
  }

  USART2_SendString("Reading CSV: ");
  USART2_SendString(filename);
  USART2_SendString("\r\n");

  while(f_gets(line, sizeof(line), &file) && *record_count < max_records)
  {
    char *token = strtok(line, ",");
    if(!token)
      continue;
    strncpy(records[*record_count].field1, token, sizeof(records[*record_count].field1));

    token = strtok(NULL, ",");
    if(!token)
      continue;
    strncpy(records[*record_count].field2, token, sizeof(records[*record_count].field2));

    token = strtok(NULL, ",");
    if(token)
      records[*record_count].value = atoi(token);
    else
      records[*record_count].value = 0;

    (*record_count)++;
  }

  f_close(&file);

  // Print parsed data
  for(int i = 0; i < *record_count; i++)
  {
    char buffer[128];
    char num_buffer[16];
    char *ptr = buffer;

    *ptr++ = '[';
    itoa_32(i, num_buffer);
    for(char *s = num_buffer; *s; s++)
      *ptr++ = *s;
    *ptr++ = ']';
    *ptr++ = ' ';

    // Copy field1
    for(char *s = records[i].field1; *s; s++)
      *ptr++ = *s;
    *ptr++ = ' ';
    *ptr++ = '|';
    *ptr++ = ' ';

    // Copy field2
    for(char *s = records[i].field2; *s; s++)
      *ptr++ = *s;
    *ptr++ = ' ';
    *ptr++ = '|';
    *ptr++ = ' ';

    // Copy value
    itoa_32(records[i].value, num_buffer);
    for(char *s = num_buffer; *s; s++)
      *ptr++ = *s;

    *ptr++ = '\r';
    *ptr++ = '\n';
    *ptr = '\0';

    USART2_SendString(buffer);
  }

  return FR_OK;
}

int sd_delete_file(const char *filename)
{
  FRESULT res = f_unlink(filename);
  USART2_SendString("Delete ");
  USART2_SendString(filename);
  USART2_SendString(": ");
  USART2_SendString((res == FR_OK ? "OK" : "Failed"));
  USART2_SendString("\r\n");
  return res;
}

int sd_rename_file(const char *oldname, const char *newname)
{
  FRESULT res = f_rename(oldname, newname);
  USART2_SendString("Rename ");
  USART2_SendString(oldname);
  USART2_SendString(" to ");
  USART2_SendString(newname);
  USART2_SendString(": ");
  USART2_SendString((res == FR_OK ? "OK" : "Failed"));
  USART2_SendString("\r\n");
  return res;
}

FRESULT sd_create_directory(const char *path)
{
  FRESULT res = f_mkdir(path);
  USART2_SendString("Create directory ");
  USART2_SendString(path);
  USART2_SendString(": ");
  USART2_SendString((res == FR_OK ? "OK" : "Failed"));
  USART2_SendString("\r\n");
  return res;
}

void sd_list_directory_recursive(const char *path, int depth)
{
  DIR dir;
  FILINFO fno;
  FRESULT res = f_opendir(&dir, path);
  if(res != FR_OK)
  {
    // Print indentation
    for(int i = 0; i < depth; i++)
      USART2_SendString("  ");
    USART2_SendString("[ERR] Cannot open: ");
    USART2_SendString(path);
    USART2_SendString("\r\n");
    return;
  }

  while(1)
  {
    res = f_readdir(&dir, &fno);
    if(res != FR_OK || fno.fname[0] == 0)
      break;

    // ADD THIS DEBUG LINE
    USART2_SendString("Found entry: ");
    USART2_SendString(fno.fname);
    USART2_SendString("\r\n");

    const char *name = (*fno.fname) ? fno.fname : fno.fname;

    if(fno.fattrib & AM_DIR)
    {
      if(strcmp(name, ".") && strcmp(name, ".."))
      {
        // Print indentation
        for(int i = 0; i < depth; i++)
          USART2_SendString("  ");
        USART2_SendString(name);
        USART2_SendString("\r\n");

        char newpath[128];
        snprintf(newpath, sizeof(newpath), "%s/%s", path, name);
        sd_list_directory_recursive(newpath, depth + 1);
      }
    }
    else
    {
      // Print indentation
      for(int i = 0; i < depth; i++)
        USART2_SendString("  ");
      USART2_SendString(name);
      USART2_SendString(" (");

      char num_buffer[16];
      itoa_32(fno.fsize, num_buffer);
      USART2_SendString(num_buffer);

      USART2_SendString(" bytes)\r\n");
    }
  }
  f_closedir(&dir);
}

void sd_list_files(void)
{
  USART2_SendString("Files on SD Card:\r\n");
  sd_list_directory_recursive(sd_path, 0);
  USART2_SendString("\r\n\r\n");
}
