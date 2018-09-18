/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/




#ifndef __RDC2_0041_BOARD_H
#define __RDC2_0041_BOARD_H

#include "stm32f042x6.h"
#include "EventHandler.h"


//сегменты индикатора
#define       SEGMENTS_GPIO                GPIOA
#define       SEGMENT_1_PIN                7
#define       SEGMENT_2_PIN                6
#define       SEGMENT_3_PIN                3
#define       SEGMENT_4_PIN                1
#define       SEGMENT_5_PIN                5
#define       SEGMENT_6_PIN                8
#define       SEGMENT_7_PIN                9
#define       SEGMENT_8_PIN                4
#define       SEGMENTS_MASK                ((1 << SEGMENT_1_PIN) | (1 << SEGMENT_2_PIN) | (1 << SEGMENT_3_PIN) | (1 << SEGMENT_4_PIN) | \
                                            (1 << SEGMENT_5_PIN) | (1 << SEGMENT_6_PIN) | (1 << SEGMENT_7_PIN) | (1 << SEGMENT_8_PIN))

//разряды
#define       DIGITS_GPIO                  GPIOB
#define       DIGIT_1_PIN                  0
#define       DIGIT_2_PIN                  1
#define       DIGIT_3_PIN                  6
#define       DIGIT_4_PIN                  7
#define       DIGITS_MASK                  ((1 << DIGIT_1_PIN) | (1 << DIGIT_2_PIN) | (1 << DIGIT_3_PIN) | (1 << DIGIT_4_PIN))
#define       DIGITS_NUM                   4

//реле
#define       RELAY_GPIO                   GPIOA
#define       RELAY_PIN                    10

//светодиод
#define       LED_GPIO                     GPIOB
#define       LED_PIN                      8

//кнопка
#define       KEY_GPIO                     GPIOA
#define       KEY_PIN                      2
#define       KEY_IRQ                      EXTI2_3_IRQn


//RTC interrupt pin
#define       RTC_GPIO                     GPIOA
#define       RTC_PIN                      0
#define       RTC_IRQ                      EXTI0_1_IRQn

//SPI
#define       SPI_GPIO                     GPIOB
#define       SPI_PORT                     SPI1
#define       SPI_SCK_PIN                  3
#define       SPI_SCK_AF                   0
#define       SPI_MISO_PIN                 4
#define       SPI_MISO_AF                  0
#define       SPI_MOSI_PIN                 5
#define       SPI_MOSI_AF                  0
#define       SPI_NSS_GPIO                 GPIOA
#define       SPI_NSS_PIN                  15

//I2C
#define       I2C_GPIO                     GPIOF
#define       I2C_PORT                     I2C1
#define       I2C_SCL_PIN                  1
#define       I2C_SCL_AF                   1
#define       I2C_SDA_PIN                  0
#define       I2C_SDA_AF                   1


//приоритеты прерываний
#define       RTC_PRIORITY                 0
#define       DISPLAY_PRIORITY             1
#define       KEY_PRIORITY                 2

#define       DISPLAY_UPDATE_PENDING       (1 << 0)


#define       RTC_ERROR_LED_FREQ           2//Hz
#define       EEPROM_ERROR_LED_FREQ        4

#define       ERROR_EEPROM                 0
#define       ERROR_RTC                    1

#define       CLOCK_POINT_POSITION         1
#define       CLOCK_POINT_DURATION         60
#define       EVENT_POINT_DURATION         15

#define       NO_EVENT_HOURS               0xFF



void ReadSettings(uint8_t *DataBuffer);

void WriteSettings(uint8_t *DataBuffer);

void SyncTime(uint8_t *DataBuffer);

void GetTime(uint8_t *DataBuffer);

void RDC2_0041_Init();

void LoadDayEvents(uint8_t WeekDay);

uint8_t ReadDayEventsCount(uint8_t DayOfWeek);

void ReadEventData(uint8_t DayOfWeek, uint8_t EventNum, EventType* EventPtr);

void RTCIntHandler();

void ActivateNextEvent(uint8_t *TimeNow);

void SyncEvents(uint8_t *Time);

void SetPreviousEvent(uint8_t DayNow);

void ChangeDisplayState();

void EmptyFunction(uint8_t *ClockData);

void NextEventTimeLeft(uint8_t *EventData);

void ErrorHandler(uint8_t ErrorType);


#endif //__RDC2_0041_BOARD_H

