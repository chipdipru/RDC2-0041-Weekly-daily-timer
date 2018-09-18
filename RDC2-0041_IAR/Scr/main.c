/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/


#include "RDC2_0041_board.h"
#include "RDC2_0041_USB.h"
#include "SPI_EEPROM.h"
#include "Display.h"
#include "Key.h"
#include "EEPROM_MAP.h"
#include "RTC.h"
#include "LED.h"


void (*DataForDisplay)(uint8_t *DisData);
static volatile uint8_t State = 0;
EventType TimeLeft;
EventType *TimeLeftPtr = &TimeLeft;

int main()
{
  RDC2_0041_Init();
  
  //SCB->SCR = SCB_SCR_SLEEPONEXIT_Msk;
  
  for(;;)
  {
    //__WFI();
    
    if ((*RDC2_0041_USB_GetStatus()) != RDC2_0041_USB_IDLE)
    {
      uint8_t *NewData = (uint8_t *)RDC2_0041_USB_GetPacket();
      uint8_t USBFlagToClear = 0;
      
      if (((*RDC2_0041_USB_GetStatus()) & RDC2_0041_USB_EEPROM_READ) == RDC2_0041_USB_EEPROM_READ)
      {      
        ReadSettings(NewData);      
        RDC2_0041_USB_WhileNotReadyToSend();
        RDC2_0041_USB_SendData(NewData);        
        USBFlagToClear = RDC2_0041_USB_EEPROM_READ;
      }
      
      else if (((*RDC2_0041_USB_GetStatus()) & RDC2_0041_USB_EEPROM_WRITE) == RDC2_0041_USB_EEPROM_WRITE)
      {           
        if ((*(NewData + USB_START_STOP_FLAG_POS)) == USB_CMD_START_FLAG)
        {
          RTC_DisableAlarm1();
          RTC_ClearIntStatus();
        }
        
        WriteSettings(NewData);
        
        if ((*(NewData + USB_START_STOP_FLAG_POS)) == USB_CMD_STOP_FLAG)
        {
          uint8_t RTCtime[BYTES_FOR_TIME + BYTES_FOR_DISPLAY];
          RTC_ReadTime(RTCtime);
          SyncEvents(RTCtime);
          
          if (DataForDisplay == NextEventTimeLeft)
            State |= DISPLAY_UPDATE_PENDING;
        }
        
        USBFlagToClear = RDC2_0041_USB_EEPROM_WRITE;
      }
      
      else if (((*RDC2_0041_USB_GetStatus()) & RDC2_0041_USB_TIME) == RDC2_0041_USB_TIME)
      {
        if ((*(NewData + USB_TIME_SUBCMD_POS)) == TIME_SUBCMD_SET)
        {
          SyncTime(&NewData[USB_TIME_DATA_POS]);
          RTC_ReadTime(NewData);
          SyncEvents(NewData);  
          State |= DISPLAY_UPDATE_PENDING;
        }
        
        else if ((*(NewData + USB_TIME_SUBCMD_POS)) == TIME_SUBCMD_GET)
        {
          GetTime(&NewData[USB_TIME_DATA_POS]);         
          RDC2_0041_USB_WhileNotReadyToSend();
          RDC2_0041_USB_SendData(NewData);
        }
        
        USBFlagToClear = RDC2_0041_USB_TIME;
      }
      
      RDC2_0041_USB_ClearStatus(USBFlagToClear);
    }
    
    if (RTC_IsIntActive() == RTC_INT_ACTIVE)
    {
      RTCIntHandler();
      RTC_ClearIntStatus();
    }
    
    if ((State & DISPLAY_UPDATE_PENDING) == DISPLAY_UPDATE_PENDING)
    {
      State &= ~DISPLAY_UPDATE_PENDING;
      
      uint8_t RTCtime[BYTES_FOR_TIME + BYTES_FOR_DISPLAY];
      RTC_ReadTime(RTCtime);
      DataForDisplay(RTCtime);
      Display_SetValue(&RTCtime[DISPLAY_HOURS]);  
    }
  }
}
//------------------------------------------------------------------------------
void ReadSettings(uint8_t *DataBuffer)
{
  EEPROM_WhileNotReady();
        
  if ((*(DataBuffer + USB_EEPROM_SUBCMD_POS)) == EEPROM_SUBCMD_EVENTS_COUNT)
  {
    EEPROM_Read(7, DAYS_DATA_START_ADDRESS, &DataBuffer[USB_EEPROM_SUBCMD_POS + 1]);
  }
               
  else if ((*(DataBuffer + USB_EEPROM_SUBCMD_POS)) == EEPROM_SUBCMD_WEEK_DAY)
  {
    uint8_t CurDay = *(DataBuffer + USB_EEPROM_SUBCMD_POS + USB_WEEK_DAY_OFFSET);
          
    for (uint8_t i = 0; i < USB_EVENT_IN_PACKET_MAX; i++)
    {
      uint8_t CurPos = USB_EEPROM_SUBCMD_POS + i * USB_EVENT_SIZE;           
      uint8_t CurEvent = *(DataBuffer + CurPos + USB_EVENT_NUM_OFFSET);
      if (CurEvent == EVENTS_PER_DAY)
        break;

      EEPROM_Read(EVENT_SIZE - EVENT_RESERVED_SIZE, DAY_EVENTS_BASE(CurDay) + EVENT_DATA_OFFSET(CurEvent), &DataBuffer[CurPos + USB_EVENT_DATA_OFFSET]);
    }
  }
}
//------------------------------------------------------------------------------
void WriteSettings(uint8_t *DataBuffer)
{
  if ((*(DataBuffer + USB_EEPROM_SUBCMD_POS)) == EEPROM_SUBCMD_EVENTS_COUNT)
  {
    EEPROM_WhileNotReady();
    EEPROM_Write(7, DAYS_DATA_START_ADDRESS, &DataBuffer[USB_EEPROM_SUBCMD_POS + 1]);
  }
        
  else if ((*(DataBuffer + USB_EEPROM_SUBCMD_POS)) == EEPROM_SUBCMD_WEEK_DAY)
  {
    uint8_t DayCode = *(DataBuffer + USB_EEPROM_SUBCMD_POS + USB_WEEK_DAY_CODE_OFFSET);
    uint8_t CurPos = USB_EEPROM_SUBCMD_POS;
          
    while (DayCode != 0)
    {
      uint8_t CurDay = *(DataBuffer + CurPos + USB_WEEK_DAY_OFFSET);
      uint8_t CurEvent = *(DataBuffer + CurPos + USB_EVENT_NUM_OFFSET);

      EEPROM_WhileNotReady();
      EEPROM_Write(EVENT_SIZE - EVENT_RESERVED_SIZE, DAY_EVENTS_BASE(CurDay) + EVENT_DATA_OFFSET(CurEvent), &DataBuffer[CurPos + USB_EVENT_DATA_OFFSET]);
            
      CurPos += USB_EVENT_SIZE;
      DayCode = *(DataBuffer + CurPos + USB_WEEK_DAY_CODE_OFFSET);
            
      if (CurPos > USB_MESSAGE_LENGTH)
        break;
    }
  }
}
//------------------------------------------------------------------------------
void SyncTime(uint8_t *DataBuffer)
{
  RTC_DisableAlarm1();
  RTC_SetTime(DataBuffer);
  EEPROM_WhileNotReady();
  EEPROM_Write(1, TIME_ZONE_ADDRESS, &DataBuffer[BYTES_FOR_TIME]);  
}
//------------------------------------------------------------------------------
void GetTime(uint8_t *DataBuffer)
{
  RTC_ReadTime(DataBuffer);
  EEPROM_WhileNotReady();
  EEPROM_Read(1, TIME_ZONE_ADDRESS, &DataBuffer[BYTES_FOR_TIME]);
}
//------------------------------------------------------------------------------
void RDC2_0041_Init()
{
  //HSI, PLL, 48 MHz
  FLASH->ACR = FLASH_ACR_PRFTBE | (uint32_t)FLASH_ACR_LATENCY;  
  // частота PLL: (HSI / 2) * 12 = (8 / 2) * 12 = 48 (МГц)
  RCC->CFGR |= (uint32_t)(RCC_CFGR_PLLSRC_HSI_DIV2 | RCC_CFGR_PLLMUL12);

  RCC->CR |= RCC_CR_PLLON;
  while((RCC->CR & RCC_CR_PLLRDY) == 0);
 
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL);
  
  RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN  | RCC_AHBENR_GPIOFEN;
  
  RELAY_GPIO->MODER |= (1 << (2 * RELAY_PIN));
  
  //LED
  LED_Init();  
  //EEPROM  
  if (EEPROM_Init() != EEPROM_PRESENT)
    ErrorHandler(ERROR_EEPROM);
  //RTC
  if (RTC_Init() != RTC_IS_READY)
    ErrorHandler(ERROR_RTC);
  
  uint8_t Alarm2Time[] = {DS3231_ALARM_MASK, DS3231_ALARM_MASK, DS3231_ALARM_MASK};
  RTC_SetAlarm2(Alarm2Time); //каждую минуту
  RTC_EnableAlarm2();
  
  //события
  Events_Init();
  //Display
  Display_Init(CLOCK_POINT_POSITION, CLOCK_POINT_DURATION);
  DataForDisplay = EmptyFunction;
  
  uint8_t RTCtime[BYTES_FOR_TIME + BYTES_FOR_DISPLAY];
  RTC_ReadTime(RTCtime);
  SyncEvents(RTCtime);
  DataForDisplay(RTCtime);
  Display_SetValue(&RTCtime[DISPLAY_HOURS]);

  //USB
  RDC2_0041_USB_Init();
  //кнопка
  Key_Init(&ChangeDisplayState);
  
  Display_ON();
  Display_SetState(DISPLAY_IS_ON_TIME);
}
//------------------------------------------------------------------------------
void LoadDayEvents(uint8_t WeekDay)
{  
  uint8_t EventsInDay = ReadDayEventsCount(WeekDay);
  
  if (EventsInDay <= EVENTS_PER_DAY)
  {
    SetDayEventsCount(EventsInDay);
    
    for (uint8_t i = 0; i < EventsInDay; i++)
    {
      EventType NewEvent;
      
      ReadEventData(WeekDay, i, &NewEvent);
      SetEventData(i, &NewEvent);      
    }
  }
  else //если память чистая
    SetDayEventsCount(0);
}
//------------------------------------------------------------------------------
uint8_t ReadDayEventsCount(uint8_t DayOfWeek)
{
  uint8_t DayEventsCount;
  
  EEPROM_WhileNotReady();
  EEPROM_Read(1, DAY_EVENTS_COUNT(DayOfWeek), &DayEventsCount);
  
  return DayEventsCount;
}
//------------------------------------------------------------------------------
void ReadEventData(uint8_t DayOfWeek, uint8_t EventNum, EventType* EventPtr)
{
  uint8_t DataToRead[EVENT_SIZE];
  
  EEPROM_Read(EVENT_SIZE - EVENT_RESERVED_SIZE, DAY_EVENTS_BASE(DayOfWeek) + EVENT_DATA_OFFSET(EventNum), DataToRead);
  EventPtr->Hours = *(DataToRead + EVENT_HOURS_OFFSET);
  EventPtr->Minutes = *(DataToRead + EVENT_MINUTES_OFFSET);
  EventPtr->RelayState = *(DataToRead + EVENT_RELAY_STATE_OFFSET);
}
//------------------------------------------------------------------------------
void RTCIntHandler()
{
  uint8_t RTCstatus;
  uint8_t NewAlarm = 0;
  
  RTC_ReadStatus(&RTCstatus);
  
  if (TimeLeftPtr->Hours != NO_EVENT_HOURS)
  {
    if (TimeLeftPtr->Minutes != 0)
      TimeLeftPtr->Minutes--;
    else
    {
      TimeLeftPtr->Minutes = 59;
      if (TimeLeftPtr->Hours != 0)
        TimeLeftPtr->Hours--;
    }
  }
  
  if ((RTCstatus & DS3231_A1F) == DS3231_A1F)//настпуило событие
  {
    RTC_DisableAlarm1();
    ExecuteEvent();
    NewAlarm = 1;    
  }
  
  uint8_t RTCtime[BYTES_FOR_TIME + BYTES_FOR_DISPLAY];
  RTC_ReadTime(RTCtime);
  
  if ((RTCtime[DS3231_HOURS_OFFSET] == 0) && (RTCtime[DS3231_MINUTES_OFFSET] == 0))//новый день
  {
    LoadDayEvents(RTCtime[DS3231_DAYS_OFFSET] - 1);  
    FindPendingEvent(RTCtime[DS3231_HOURS_OFFSET], RTCtime[DS3231_MINUTES_OFFSET]);
    NewAlarm = 1;
    
    uint8_t NewEventTime[2];
    if (PendingEventTime(NewEventTime) == EVENT_EXISTS)
    {
      if ((NewEventTime[0] == 0) && (NewEventTime[1] == 0))
      {
        ExecuteEvent();
        //NewAlarm = 1;
      }
    }    
  }
  
  if (NewAlarm == 1)
    ActivateNextEvent(RTCtime);
  
  DataForDisplay(RTCtime);
  Display_SetValue(&RTCtime[DISPLAY_HOURS]);
}
//------------------------------------------------------------------------------
void ActivateNextEvent(uint8_t *TimeNow)
{
  uint8_t AlarmTime[ALARM_1_SIZE];
  uint16_t MinBeforeEvent;
  uint16_t MinutesNow = TimeNow[DS3231_MINUTES_OFFSET] + TimeNow[DS3231_HOURS_OFFSET] * 60;
  
  //если следующее событие сегодня
  if (PendingEventTime(&AlarmTime[DS3231_ALARM_MIN_OFFSET]) == EVENT_EXISTS)
  {
    AlarmTime[DS3231_ALARM_SEC_OFFSET] = 0;
    AlarmTime[DS3231_ALARM_DAY_OFFSET] = DS3231_ALARM_MASK;
    MinBeforeEvent = AlarmTime[DS3231_ALARM_MIN_OFFSET] + AlarmTime[DS3231_ALARM_HOUR_OFFSET] * 60 - MinutesNow;
    
    RTC_SetAlarm1(AlarmTime);
    RTC_EnableAlarm1();
  }
  //поиск события в следующих днях с переходом на понедельник
  else
  {
    uint8_t NextEventDay = 7;
    
    for (uint8_t DayNum = (TimeNow[DS3231_DAYS_OFFSET] - 1) + 1; DayNum < 7; DayNum++)
    {
      uint8_t EventsInDay = ReadDayEventsCount(DayNum);
  
      if ((EventsInDay != 0) && (EventsInDay <= EVENTS_PER_DAY))
      {
        NextEventDay = DayNum;
        break;
      }
    }
    
    if (NextEventDay == 7)
    {
      for (uint8_t DayNum = 0; DayNum <= (TimeNow[DS3231_DAYS_OFFSET] - 1); DayNum++)
      {
        uint8_t EventsInDay = ReadDayEventsCount(DayNum);
  
        if ((EventsInDay != 0) && (EventsInDay <= EVENTS_PER_DAY))
        {
          NextEventDay = DayNum;
          break;
        }
      }
    }
    
    if (NextEventDay != 7) //событие найдено в каком-то дне
    {
/*      
      AlarmTime[DS3231_ALARM_SEC_OFFSET] = 0;
      AlarmTime[DS3231_ALARM_MIN_OFFSET] = 0;
      AlarmTime[DS3231_ALARM_HOUR_OFFSET] = 0;
      AlarmTime[DS3231_ALARM_DAY_OFFSET] = DS3231_ALARM_MASK;
      RTC_SetAlarm1(AlarmTime);
      RTC_EnableAlarm1();
*/      
      EventType NextEvent;
      ReadEventData(NextEventDay, 0, &NextEvent);
    
      if (NextEventDay <= (TimeNow[DS3231_DAYS_OFFSET] - 1))
        NextEventDay += 7;
        
      MinBeforeEvent = (NextEventDay - (TimeNow[DS3231_DAYS_OFFSET] - 1)) * 24 * 60 + NextEvent.Minutes + 60 * NextEvent.Hours - MinutesNow;    
    }
    else //событий нет
      MinBeforeEvent = NO_EVENT_HOURS * 60;
  }
  
  TimeLeftPtr->Hours = MinBeforeEvent / 60;
  TimeLeftPtr->Minutes = MinBeforeEvent % 60;
}
//------------------------------------------------------------------------------
void SyncEvents(uint8_t *Time)
{
  LoadDayEvents(Time[DS3231_DAYS_OFFSET] - 1); //-1, т.к. в DS3231 нумерация дней с 1 до 7  
  FindPendingEvent(Time[DS3231_HOURS_OFFSET], Time[DS3231_MINUTES_OFFSET]);
  SetPreviousEvent(Time[DS3231_DAYS_OFFSET] - 1);
  ActivateNextEvent(Time);
}
//------------------------------------------------------------------------------
void SetPreviousEvent(uint8_t DayNow)
{
  if (SetPrevTodayEvent() != EVENT_EXISTS)
  {
    uint8_t PrevEventDay = 7;
    uint8_t EventsInDay = 0;
    
    for (int8_t DayNum = (DayNow - 1); DayNum >= 0; DayNum--)
    {
      EventsInDay = ReadDayEventsCount(DayNum);
  
      if ((EventsInDay != 0) && (EventsInDay <= EVENTS_PER_DAY))
      {
        PrevEventDay = DayNum;
        break;
      }
    }
    
    if (PrevEventDay == 7)
    {
      for (int8_t DayNum = 6; DayNum >= (DayNow - 1); DayNum--)
      {
        EventsInDay = ReadDayEventsCount(DayNum);
  
        if ((EventsInDay != 0) && (EventsInDay <= EVENTS_PER_DAY))
        {
          PrevEventDay = DayNum;
          break;
        }
      }
    }
    
    if (PrevEventDay != 7) //событие найдено в каком-то дне
    {      
      EventType PrevEvent;
      ReadEventData(PrevEventDay, EventsInDay - 1, &PrevEvent);
      SetEventState(PrevEvent.RelayState);
    }
    else
      SetEventState(RELAY_OFF);
  }
}
//------------------------------------------------------------------------------
void ChangeDisplayState()
{
  switch(Display_State())
  {
    case DISPLAY_IS_OFF:
      Display_ON(); //время
      Display_SetState(DISPLAY_IS_ON_TIME);
      Display_PointSet(CLOCK_POINT_POSITION, CLOCK_POINT_DURATION);
      DataForDisplay = EmptyFunction;
      State |= DISPLAY_UPDATE_PENDING;
    break;
    
    case DISPLAY_IS_ON_TIME:
      Display_SetState(DISPLAY_IS_ON_EVENT);      
      DataForDisplay = NextEventTimeLeft;
      State |= DISPLAY_UPDATE_PENDING;
    break;
    
    case DISPLAY_IS_ON_EVENT:
      Display_OFF();
      Display_SetState(DISPLAY_IS_OFF);
      DataForDisplay = EmptyFunction;
    break;
  }  
}
//------------------------------------------------------------------------------
void EmptyFunction(uint8_t *ClockData)
{
  
}
//------------------------------------------------------------------------------
void NextEventTimeLeft(uint8_t *EventData)
{
  uint8_t PointPos = 1;
  
  if (TimeLeftPtr->Hours != NO_EVENT_HOURS)
  {  
    (*(EventData + DISPLAY_HOURS)) = TimeLeftPtr->Hours / 100;
    (*(EventData + DISPLAY_HOURS + 1)) = (TimeLeftPtr->Hours % 100) / 10;
    (*(EventData + DISPLAY_MINUTES)) = (TimeLeftPtr->Hours % 100) % 10;
        
    if ((*(EventData + DISPLAY_HOURS)) != 0)
    {
      PointPos = 2;
      (*(EventData + DISPLAY_MINUTES + 1)) = CHAR_h_INDEX;
    }
    else
    {
      (*(EventData + DISPLAY_HOURS)) = (*(EventData + DISPLAY_HOURS + 1));
      (*(EventData + DISPLAY_HOURS + 1)) = (*(EventData + DISPLAY_MINUTES));
      (*(EventData + DISPLAY_MINUTES)) = TimeLeftPtr->Minutes / 10;
      (*(EventData + DISPLAY_MINUTES + 1)) = TimeLeftPtr->Minutes % 10;
    }
  }
  
  else
  {
    (*(EventData + DISPLAY_HOURS)) = CHAR_LINE_INDEX;
    (*(EventData + DISPLAY_HOURS + 1)) = CHAR_LINE_INDEX;
    (*(EventData + DISPLAY_MINUTES)) = CHAR_LINE_INDEX;
    (*(EventData + DISPLAY_MINUTES + 1)) = CHAR_LINE_INDEX;
    PointPos = DIGITS_NUM;//точка отключена
  }

  Display_PointSet(PointPos, EVENT_POINT_DURATION);
}
//------------------------------------------------------------------------------
void ErrorHandler(uint8_t ErrorType)
{
  RELAY_GPIO->ODR &= ~(1 << RELAY_PIN);
  
  uint8_t LEDfreq = 1;
      
  switch(ErrorType)
  {
    case ERROR_EEPROM:
      LEDfreq = EEPROM_ERROR_LED_FREQ;
    break;
    
    case ERROR_RTC:
      LEDfreq = RTC_ERROR_LED_FREQ;
    break;
    
    default:
    break;
  }
  
  LED_ONwithFreq(LEDfreq);
  for(;;);
}

