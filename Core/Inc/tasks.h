/*
 * tasks.h
 *
 *  Created on: Mar 8, 2026
 *      Author: Rubin Khadka
 */

#ifndef INC_TASKS_H_
#define INC_TASKS_H_

extern volatile float dht11_humidity;
extern volatile float dht11_temperature;

void Task_DHT11_Read(void);
void Task_LCD_Update(void);
void Task_MPU6050_Read(void);
void Task_MQTT_Publish_DHT11(void);
void Task_MQTT_Publish_MPU6050(void);

#endif /* INC_TASKS_H_ */
