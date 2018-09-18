/*
********************************************************************************
* COPYRIGHT(c) ��� ���� � ��ϻ, 2018
* 
* ����������� ����������� ��������������� �� �������� ���� ����� (as is).
* ��� ��������������� �������� ������ �����������.
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
  uint8_t Hours;       //���� �������
  uint8_t Minutes;     //������ �������
  uint8_t RelayState;  //��������� ����
  
} EventType;

typedef struct
{
  uint8_t EventsCount; //���������� ������� � ���
  uint8_t PendingEvent; //����� ���������� �������
    
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


