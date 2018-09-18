/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/


#ifndef __EEPROM_MAP_H
#define __EEPROM_MAP_H



#define       EEPROM_PAGE_SIZE             32 //размер станицы EEPROM

//разметка памяти
#define       SYSTEM_DATA_START_ADDRESS    (2 * EEPROM_PAGE_SIZE)
#define       DAYS_DATA_START_ADDRESS      (3 * EEPROM_PAGE_SIZE) //информация о кол-ве событий в каждом дне
#define       EVENTS_DATA_START_ADDRESS    (32 * EEPROM_PAGE_SIZE)
#define       EVENTS_DATA_PER_DAY_SIZE     (32 * EEPROM_PAGE_SIZE)
#define       TIME_ZONE_ADDRESS            SYSTEM_DATA_START_ADDRESS 

//свойства события
#define       EVENT_HOURS_SIZE             1
#define       EVENT_MINUTES_SIZE           1
#define       EVENT_RELAY_STATE_SIZE       1
#define       EVENT_FREQUENCY_SIZE         1
#define       EVENT_DUTY_CYCLE_SIZE        1
#define       EVENT_RESERVED_SIZE          3
#define       EVENT_SIZE                   (EVENT_HOURS_SIZE + EVENT_MINUTES_SIZE + EVENT_RELAY_STATE_SIZE + \
                                            EVENT_FREQUENCY_SIZE + EVENT_DUTY_CYCLE_SIZE + EVENT_RESERVED_SIZE)

#define       EVENT_HOURS_OFFSET           0
#define       EVENT_MINUTES_OFFSET         (EVENT_HOURS_OFFSET + EVENT_HOURS_SIZE)
#define       EVENT_RELAY_STATE_OFFSET     (EVENT_MINUTES_OFFSET + EVENT_MINUTES_SIZE)
#define       EVENT_FREQUENCY_OFFSET       (EVENT_RELAY_STATE_OFFSET + EVENT_RELAY_STATE_SIZE)
#define       EVENT_DUTY_CYCLE_OFFSET      (EVENT_FREQUENCY_OFFSET + EVENT_FREQUENCY_SIZE)

#define       DAY_EVENTS_COUNT(DayNum)     (DAYS_DATA_START_ADDRESS + DayNum)
#define       DAY_EVENTS_BASE(DayNum)      (EVENTS_DATA_START_ADDRESS + DayNum * EVENTS_DATA_PER_DAY_SIZE)
#define       EVENT_DATA_OFFSET(EventNum)  (EventNum * EVENT_SIZE)

#define       EVENTS_PER_EEPROM_PAGE       (EEPROM_PAGE_SIZE / EVENT_SIZE)


#endif //__EEPROM_MAP_H

