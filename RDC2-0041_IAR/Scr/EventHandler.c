/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/


#include "EventHandler.h"
#include "EEPROM_MAP.h"
#include "LED.h"


static EventType Events[EVENTS_PER_DAY];
static EventType* EventsPtr[EVENTS_PER_DAY];
static DayType DaySettings;
static DayType* DaySettingsPtr = &DaySettings;



void FindPendingEvent(uint8_t HoursNow, uint8_t MinutesNow)
{
  DaySettingsPtr->PendingEvent = EVENTS_PER_DAY;
  
  if (DaySettingsPtr->EventsCount != 0)
  {
    if ((HoursNow == 0) && (MinutesNow == 0) && (EventsPtr[0]->Hours == 0) && (EventsPtr[0]->Minutes == 0))
      DaySettingsPtr->PendingEvent = 0;
    else
    {
      for (uint8_t DayEvent = 0; DayEvent < DaySettingsPtr->EventsCount; DayEvent++)
      {
        if (((EventsPtr[DayEvent]->Hours) > HoursNow) || (((EventsPtr[DayEvent]->Hours) == HoursNow) && ((EventsPtr[DayEvent]->Minutes) > MinutesNow)))
        {
          DaySettingsPtr->PendingEvent = DayEvent;
          break;
        }
      }
    }
  }
}
//------------------------------------------------------------------------------
void SetEventState(uint8_t EventState)
{
  if (EventState == RELAY_OFF)
  {
    RELAY_GPIO->ODR &= ~(1 << RELAY_PIN);
    LED_OFF();
  }
  else
  {
    RELAY_GPIO->ODR |= 1 << RELAY_PIN;
    LED_ON();
  }
}
//------------------------------------------------------------------------------
uint8_t SetPrevTodayEvent()
{  
  if (DaySettingsPtr->EventsCount == 0)
    return 0;
  
  if ((DaySettingsPtr->PendingEvent != EVENTS_PER_DAY)
   && (DaySettingsPtr->PendingEvent != 0))
  {
    SetEventState(EventsPtr[DaySettingsPtr->PendingEvent - 1]->RelayState);
    return EVENT_EXISTS;
  }
  else if (DaySettingsPtr->PendingEvent == EVENTS_PER_DAY)
  {
    SetEventState(EventsPtr[DaySettingsPtr->EventsCount - 1]->RelayState);
    return EVENT_EXISTS;
  }
  
  return 0;
  
/*  
  if (DaySettingsPtr->EventsCount != 0)
  {
    if ((DaySettingsPtr->PendingEvent != EVENTS_PER_DAY)
     && (DaySettingsPtr->PendingEvent != 0))
      SetEventState(DaySettingsPtr->PendingEvent - 1);
    else if (DaySettingsPtr->PendingEvent == EVENTS_PER_DAY)
      SetEventState(DaySettingsPtr->EventsCount - 1);
  }
*/  
}
//------------------------------------------------------------------------------
void ExecuteEvent()
{
  if (DaySettingsPtr->PendingEvent < EVENTS_PER_DAY)
  {
    SetEventState(EventsPtr[DaySettingsPtr->PendingEvent]->RelayState);
    DaySettingsPtr->PendingEvent++;
    if (DaySettingsPtr->PendingEvent == DaySettingsPtr->EventsCount)
      DaySettingsPtr->PendingEvent = EVENTS_PER_DAY;
  }
}
//------------------------------------------------------------------------------
void SetDayEventsCount(uint8_t NewCount)
{
  DaySettingsPtr->EventsCount = NewCount;
}
//------------------------------------------------------------------------------
void SetEventData(uint8_t EventNum, EventType *EventData)
{
  EventsPtr[EventNum]->Hours = EventData->Hours;
  EventsPtr[EventNum]->Minutes = EventData->Minutes;
  EventsPtr[EventNum]->RelayState = EventData->RelayState;     
}
//------------------------------------------------------------------------------
uint8_t PendingEventTime(uint8_t *EventData)
{
  if (DaySettingsPtr->PendingEvent == EVENTS_PER_DAY)
    return 0;
  
  (*EventData) = EventsPtr[DaySettingsPtr->PendingEvent]->Minutes;
  (*(EventData + 1)) = EventsPtr[DaySettingsPtr->PendingEvent]->Hours;
  
  return EVENT_EXISTS;
}
//------------------------------------------------------------------------------
void Events_Init()
{
  for (uint8_t i = 0; i < EVENTS_PER_DAY; i++)
    EventsPtr[i] = &Events[i];
}  
  
  
  