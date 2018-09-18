/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/


#ifndef __EVENT_HANDLER_H
#define __EVENT_HANDLER_H


#include "stdint.h"


#define       EVENTS_PER_DAY               32
#define       EVENT_EXISTS                 1

#define       RELAY_OFF                    0


typedef struct
{
  uint8_t Hours;       //часы события
  uint8_t Minutes;     //минуты события
  uint8_t RelayState;  //состояние реле
  
} EventType;

typedef struct
{
  uint8_t EventsCount; //количество событий в дне
  uint8_t PendingEvent; //номер ожидаемого события
    
} DayType;



void FindPendingEvent(uint8_t HoursNow, uint8_t MinutesNow);

void SetEventState(uint8_t EventState);

uint8_t SetPrevTodayEvent();

void SetDayEventsCount(uint8_t NewCount);

void SetEventData(uint8_t EventNum, EventType *EventData);

void ExecuteEvent();

uint8_t PendingEventTime(uint8_t *EventData);

void Events_Init();


#endif //__EVENT_HANDLER_H


