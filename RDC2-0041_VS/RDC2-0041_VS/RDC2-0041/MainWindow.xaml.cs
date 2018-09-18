/*
********************************************************************************
* COPYRIGHT(c) ЗАО «ЧИП и ДИП», 2018
* 
* Программное обеспечение предоставляется на условиях «как есть» (as is).
* При распространении указание автора обязательно.
********************************************************************************
*/

using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace RDC2_0041
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public const byte DAYS_NUM = 7;
        private const byte EVENTS_MAX = 32;
        public const byte TEMP_EVENTS_MAX = 8;

        private const Int32 USB_PACKET_SIZE = 64;

        private const byte USB_REPORT_ID = 1;
        
        private const byte USB_CMD_POS = 1;
        
        private const byte USB_CMD_EEPROM_WRITE = 0;
        private const byte USB_CMD_EEPROM_READ = 1;
        private const byte USB_CMD_TIME = 2;

        private const byte EEPROM_SUBCMD_EVENTS_COUNT = 0xC0;
        private const byte EEPROM_SUBCMD_WEEK_DAY = 0xC1;        
        private const byte TIME_SUBCMD_SET = 0xC5;
        private const byte TIME_SUBCMD_GET = 0xC6;

        private const byte USB_EEPROM_SUBCMD_POS = 2;
        private const byte USB_WEEK_DAY_CODE_OFFSET = 0;
        private const byte USB_WEEK_DAY_OFFSET = 1;
        private const byte USB_EVENT_NUM_OFFSET = 2;
        private const byte USB_EVENT_DATA_OFFSET = 3;
        private const byte USB_EVENT_SIZE = 8;

        private const byte USB_TIME_SUBCMD_POS = USB_EEPROM_SUBCMD_POS;
        private const byte USB_TIME_SECONDS_POS = (USB_TIME_SUBCMD_POS + 1);
        private const byte USB_TIME_MINUTES_POS = (USB_TIME_SECONDS_POS + 1);
        private const byte USB_TIME_HOURS_POS = (USB_TIME_MINUTES_POS + 1);
        private const byte USB_TIME_WEEKDAY_POS = (USB_TIME_HOURS_POS + 1);
        private const byte USB_TIME_DATE_POS = (USB_TIME_WEEKDAY_POS + 1);
        private const byte USB_TIME_MONTH_POS = (USB_TIME_DATE_POS + 1);
        private const byte USB_TIME_YEAR_POS = (USB_TIME_MONTH_POS + 1);
        private const byte USB_TIME_ZONE_POS = (USB_TIME_YEAR_POS + 1);

        private const byte USB_START_STOP_FLAG_POS = 63;
        private const byte USB_CMD_STOP_FLAG = 0xA0;
        private const byte USB_CMD_START_FLAG = 0x55;


        private const byte USB_EVENT_IN_PACKET_MAX = 7;
        
        private const byte USB_START_HOUR_OFFSET = 9;
        
        private const byte THREAD_SET_EVENTS = 0;
        

        public static readonly string[] WeekDaysStrings =
        {
            "ПОНЕДЕЛЬНИК",
            "ВТОРНИК",
            "СРЕДА",
            "ЧЕТВЕРГ",
            "ПЯТНИЦА",
            "СУББОТА",
            "ВОСКРЕСЕНЬЕ",
        };

        private static readonly string DeviceConnectedString = "Таймер RDC2-0041 подключен.";
        private static readonly string DeviceNotConnectedString = "Таймер RDC2-0041 не подключен. Подключите устройство и перезапустите программу.";


        private ObservableCollection<Event>[] Days = new ObservableCollection<Event>[DAYS_NUM];        
        private ObservableCollection<Event> CopyBuffer = new ObservableCollection<Event>();

        private byte CurrentDay = 0;
        private DispatcherTimer ThreadTimer = new DispatcherTimer();
        private ProgressWindow ProgressBar = new ProgressWindow();
        private bool ThreadWrite = false;
        private byte ThreadDay = 0;
        private byte ThreadEventInDay = 0;
        private byte ThreadSetting = THREAD_SET_EVENTS;
               
        public MainWindow()
        {
            InitializeComponent();
            
            for (byte day = 0; day < DAYS_NUM; day++)
            {
                Days[day] = new ObservableCollection<Event>();                
            }

            Events.ItemsSource = Days[0];

            Boolean USBDevDetected = USB_device.Open();
            
            if (USBDevDetected)
            {                                
                ThreadTimer.Interval = TimeSpan.FromMilliseconds(10); //период опроса устройства
                ThreadTimer.Tick += ThreadTimer_Tick;
            }

            ShowConnectionState(USBDevDetected);
        }

        private void NewEventButton_Click(object sender, RoutedEventArgs e)
        {
            Event EventToAdd = new Event();
            EventWindow NewEventWindow = new EventWindow(EventToAdd);
            NewEventWindow.Owner = this;
            NewEventWindow.ShowDialog();
            
            if (EventToAdd.IsActive == true)
            {
                CopyBuffer.Clear();
                CopyBuffer.Add(new Event() { Hours = EventToAdd.Hours, Minutes = EventToAdd.Minutes, RelayState = EventToAdd.RelayState, IsActive = true });
                PasteEvent();
                CopyBuffer.Clear();
            }
        }
        
        private void DeleteEventButton_Click(object sender, RoutedEventArgs e)
        {
            if (Events.SelectedItems.Count != 0)
            {
                ObservableCollection<Event> DelBuffer = new ObservableCollection<Event>();

                foreach (Event ItemToCopy in Events.SelectedItems)
                    DelBuffer.Add(ItemToCopy);

                foreach (Event ItemToDel in DelBuffer)
                    Days[CurrentDay].Remove(ItemToDel);
            }
        }
        
        private void WeekDayChanged(object sender, SelectionChangedEventArgs e)
        {
            if ((Events != null) && (WeekDays.SelectedItem != null))
            {
                CurrentDay = (byte)WeekDays.SelectedIndex;
                Events.ItemsSource = Days[CurrentDay];
            }
        }

        private void CopyEventButton_Click(object sender, RoutedEventArgs e)
        {
            if (Events.SelectedItems.Count != 0)
            {
                CopyBuffer.Clear();

                foreach (Event ItemToCopy in Events.SelectedItems)
                    CopyBuffer.Add(new Event() { Hours = ItemToCopy.Hours, Minutes = ItemToCopy.Minutes, RelayState = ItemToCopy.RelayState, IsActive = true });
            }
        }

        private void PasteEventButton_Click(object sender, RoutedEventArgs e)
        {
            PasteEvent();
        }

        private void EditEventButton_Click(object sender, RoutedEventArgs e)
        {
            if (Events.SelectedItems.Count == 1)
            {
                Event EventToEdit = new Event();
                Event EventLastSet = new Event();
                EventToEdit.Hours = Days[CurrentDay][Events.SelectedIndex].Hours;
                EventToEdit.Minutes = Days[CurrentDay][Events.SelectedIndex].Minutes;
                EventToEdit.RelayState = Days[CurrentDay][Events.SelectedIndex].RelayState;
                EventToEdit.IsActive = true;
                EventLastSet.Hours = Days[CurrentDay][Events.SelectedIndex].Hours;
                EventLastSet.Minutes = Days[CurrentDay][Events.SelectedIndex].Minutes;
                EventLastSet.RelayState = Days[CurrentDay][Events.SelectedIndex].RelayState;
                EventLastSet.IsActive = true;
                EventWindow NewEventWindow = new EventWindow(EventToEdit);
                NewEventWindow.Owner = this;
                NewEventWindow.ShowDialog();

                if (EventToEdit.IsActive == true)
                {
                    if ((Days[CurrentDay][Events.SelectedIndex].Hours == EventToEdit.Hours)
                     && (Days[CurrentDay][Events.SelectedIndex].Minutes == EventToEdit.Minutes))
                    {
                        Days[CurrentDay][Events.SelectedIndex].RelayState = EventToEdit.RelayState;
                    }

                    else
                    {
                        int NewItemIndex = -1;
                        int ItemIndex = Events.SelectedIndex;

                        Days[CurrentDay].RemoveAt(Events.SelectedIndex);

                        foreach (Event EventToComp in Days[CurrentDay])
                        {
                            if ((EventToEdit.Hours < EventToComp.Hours)
                             || ((EventToEdit.Hours == EventToComp.Hours) && (EventToEdit.Minutes < EventToComp.Minutes)))
                            {
                                NewItemIndex = Days[CurrentDay].IndexOf(EventToComp);
                                break;
                            }

                            else if ((EventToEdit.Hours == EventToComp.Hours) && (EventToEdit.Minutes == EventToComp.Minutes))
                            {
                                NewItemIndex = EVENTS_MAX;
                                break;
                            }
                        }

                        if ((NewItemIndex >= 0) && (NewItemIndex < EVENTS_MAX))
                            Days[CurrentDay].Insert(NewItemIndex, new Event() { Hours = EventToEdit.Hours, Minutes = EventToEdit.Minutes, RelayState = EventToEdit.RelayState, IsActive = true });

                        else if (NewItemIndex == EVENTS_MAX)
                        {
                            CopyBuffer.Clear();
                            CopyBuffer.Add(new Event() { Hours = EventToEdit.Hours, Minutes = EventToEdit.Minutes, RelayState = EventToEdit.RelayState, IsActive = true });
                            if (PasteEvent() == -1)
                                Days[CurrentDay].Insert(ItemIndex, new Event() { Hours = EventLastSet.Hours, Minutes = EventLastSet.Minutes, RelayState = EventLastSet.RelayState, IsActive = true });
                            CopyBuffer.Clear();
                        }
                            
                        else                            
                            Days[CurrentDay].Add(new Event() { Hours = EventToEdit.Hours, Minutes = EventToEdit.Minutes, RelayState = EventToEdit.RelayState, IsActive = true });
                    }
                }
            }
        }

        private void OpenFileButton_Click(object sender, ExecutedRoutedEventArgs e)
        {
            bool? WinRes = true;

            foreach (ObservableCollection<Event> DayItem in Days)
            {
                if (DayItem.Count != 0)
                {
                    WarningWindow Warning = new WarningWindow();
                    Warning.Owner = this;
                    WinRes = Warning.ShowDialog();
                    break;
                }
            }
            
            if (WinRes == true)
            {
                OpenFileDialog ChooseFile = new OpenFileDialog();
                ChooseFile.Filter = "Text files (*.txt)|*.txt";
                if (ChooseFile.ShowDialog() == true)
                {
                    OpenFile(ChooseFile.FileName);
                }
            }
        }

        private void SaveFileButton_Click(object sender, ExecutedRoutedEventArgs e)
        {
            SaveFileDialog ChooseFile = new SaveFileDialog();
            ChooseFile.Filter = "Text files (*.txt)|*.txt";
            if (ChooseFile.ShowDialog() == true)
            {
                using (StreamWriter FileToWriteTo = File.CreateText(ChooseFile.FileName))
                {
                    byte DayNum = 0;
                    //события по дням для реле
                    foreach (ObservableCollection<Event> DayItem in Days)
                    {
                        FileToWriteTo.WriteLine("---------{0}", WeekDaysStrings[DayNum]);

                        foreach (Event EventItem in DayItem)
                        {
                            FileToWriteTo.WriteLine("{0}:{1} Реле: {2};", EventItem.Hours, EventItem.Minutes, EventItem.RelayState);
                        }

                        DayNum++;
                    }                    
                }
            }
        }
        
        private void DownloadButton_Click(object sender, RoutedEventArgs e)
        {
            DownloadStart();            
        }

        private void UpLoadButton_Click(object sender, RoutedEventArgs e)
        {
            bool? WinRes = true;

            foreach (ObservableCollection<Event> DayItem in Days)
            {
                if (DayItem.Count != 0)
                {
                    WarningWindow Warning = new WarningWindow();
                    WinRes = Warning.ShowDialog();
                    break;
                }
            }

            if (WinRes == true)
                ReadSettings();
        }
        
        private void ThreadTimer_Tick(object sender, EventArgs e)
        {
            if (ThreadWrite)//запись
            {
                byte[] USBPacket = new byte[USB_PACKET_SIZE];
                USBPacket[0] = USB_REPORT_ID;
                USBPacket[USB_CMD_POS] = USB_CMD_EEPROM_WRITE;

                if (ThreadSetting == THREAD_SET_EVENTS)
                {
                    ProgressBar.Owner = this;
                    ProgressBar.Show();

                    for (byte EventInPacket = 0; EventInPacket < USB_EVENT_IN_PACKET_MAX; EventInPacket++)
                    {
                        if (ThreadEventInDay != Days[ThreadDay].Count)
                        {
                            int EventStartPos = USB_EEPROM_SUBCMD_POS + EventInPacket * USB_EVENT_SIZE;
                            USBPacket[EventStartPos + USB_WEEK_DAY_CODE_OFFSET] = EEPROM_SUBCMD_WEEK_DAY;
                            USBPacket[EventStartPos + USB_WEEK_DAY_OFFSET] = ThreadDay;
                            USBPacket[EventStartPos + USB_EVENT_NUM_OFFSET] = ThreadEventInDay;
                            USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET] = Days[ThreadDay][ThreadEventInDay].Hours;
                            USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET + 1] = Days[ThreadDay][ThreadEventInDay].Minutes;
                            USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET + 2] = (byte)Array.IndexOf(EventWindow.RelayStatesStrings, Days[ThreadDay][ThreadEventInDay].RelayState);
                            
                            ThreadEventInDay++;
                        }

                        else
                        {
                            byte NextDay = DAYS_NUM;
                            for (byte i = (byte)(ThreadDay + 1); i < DAYS_NUM; i++)
                            {
                                if (Days[i].Count != 0)
                                {
                                    NextDay = i;
                                    ThreadDay = NextDay;
                                    ThreadEventInDay = 0;
                                    break;
                                }
                            }

                            if (NextDay == DAYS_NUM)
                            {
                                ThreadDay = 0;
                                ThreadTimer.Stop();
                                ProgressBar.Hide();

                                USBPacket[USB_START_STOP_FLAG_POS] = USB_CMD_STOP_FLAG;
                            }

                            break;
                        }
                    }
                }
                
                Boolean USBSuccess = USB_device.Write(USBPacket);
            }

            else //чтение
            {
                byte[] USBPacket = new byte[USB_PACKET_SIZE];
                USBPacket[0] = USB_REPORT_ID;
                USBPacket[USB_CMD_POS] = USB_CMD_EEPROM_READ;

                if (ThreadSetting == THREAD_SET_EVENTS)
                {
                    ProgressBar.Owner = this;
                    ProgressBar.Show();
                    byte NextDay = ThreadDay;
                    USBPacket[USB_EEPROM_SUBCMD_POS] = EEPROM_SUBCMD_WEEK_DAY;
                    USBPacket[USB_EEPROM_SUBCMD_POS + USB_WEEK_DAY_OFFSET] = ThreadDay;
                    for (byte EventInPacket = 0; EventInPacket < USB_EVENT_IN_PACKET_MAX; EventInPacket++)
                    {
                        if (ThreadEventInDay != Days[ThreadDay][0].Hours)
                        {
                            USBPacket[USB_EEPROM_SUBCMD_POS + EventInPacket * USB_EVENT_SIZE + USB_EVENT_NUM_OFFSET] = ThreadEventInDay;
                            ThreadEventInDay++;
                        }

                        else
                        {
                            USBPacket[USB_EEPROM_SUBCMD_POS + EventInPacket * USB_EVENT_SIZE + USB_EVENT_NUM_OFFSET] = EVENTS_MAX;

                            NextDay = DAYS_NUM;
                            for (byte i = (byte)(ThreadDay + 1); i < DAYS_NUM; i++)
                            {
                                if (Days[i][0].Hours != 0)
                                {
                                    NextDay = i;
                                    break;
                                }
                            }

                            break;
                        }
                    }

                    Boolean USBSuccess = USB_device.Write(USBPacket);
                    USBSuccess = USB_device.Read(USBPacket);
                    for (byte EventInPacket = 0; EventInPacket < USB_EVENT_IN_PACKET_MAX; EventInPacket++)
                    {
                        int EventStartPos = USB_EEPROM_SUBCMD_POS + EventInPacket * USB_EVENT_SIZE;

                        if (USBPacket[EventStartPos + USB_EVENT_NUM_OFFSET] != EVENTS_MAX)
                            Days[ThreadDay].Add(new Event()
                            {
                                Hours = USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET],
                                Minutes = USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET + 1],
                                RelayState = EventWindow.RelayStatesStrings[USBPacket[EventStartPos + USB_EVENT_DATA_OFFSET + 2]],
                            });

                        else
                            break;
                    }

                    if ((NextDay != DAYS_NUM) && (ThreadDay != NextDay))
                    {
                        ThreadDay = NextDay;
                        ThreadEventInDay = 0;
                    }
                    else if (NextDay == DAYS_NUM)
                    {
                        foreach (ObservableCollection<Event> day in Days)
                            day.RemoveAt(0);
                        //ThreadDay = 0;
                        ThreadTimer.Stop();
                        ProgressBar.Hide();
                    }
                }                
            }
        }

        private void ReadSettings()
        {
            bool EventsExist = false;
            byte[] USBPacket = new byte[USB_PACKET_SIZE];
            USBPacket[0] = USB_REPORT_ID;
            USBPacket[USB_CMD_POS] = USB_CMD_EEPROM_READ;
            USBPacket[USB_EEPROM_SUBCMD_POS] = EEPROM_SUBCMD_EVENTS_COUNT;
            Boolean USBSuccess = USB_device.Write(USBPacket);

            ShowConnectionState(USBSuccess);
            if (USBSuccess == false)
                return;

            USBSuccess = USB_device.Read(USBPacket);

            for(byte i = 0; i < DAYS_NUM; i++)
            {
                Days[i].Clear();
                //создается один элемент, в поле Hours кол-во событий в дне
                //этот элемент потом нужно удалить
                byte EventsInThatDay = USBPacket[USB_EEPROM_SUBCMD_POS + 1 + i];
                if (EventsInThatDay > EVENTS_MAX)
                    EventsInThatDay = 0;
                Days[i].Add(new Event() {Hours = EventsInThatDay });
            }

            for (byte i = 0; i < DAYS_NUM; i++)
            {
                if (Days[i][0].Hours != 0)
                {
                    ThreadDay = i;
                    ThreadEventInDay = 0;
                    EventsExist = true;
                    break;
                }
            }

            if (EventsExist)
            {
                ThreadWrite = false;
                ThreadSetting = THREAD_SET_EVENTS;
                ThreadTimer.Start();
            }

            else
            {
                DeleteAllEvents();                
            }
        }
        
        private int PasteEvent()
        {            
            int SameEventsAction = 0;

            foreach (Event ItemToCopy in CopyBuffer)
            {
                bool SameTimeEvents = false;

                foreach (Event EventToComp in Days[CurrentDay])
                {
                    if ((ItemToCopy.Hours == EventToComp.Hours) && (ItemToCopy.Minutes == EventToComp.Minutes))
                    {
                        SameTimeEvents = true;
                        break;
                    }
                }

                if (SameTimeEvents)
                {
                    bool? WindowRes;
                    PasteActionWindow PasteDialog = new PasteActionWindow();
                    PasteDialog.Owner = this;
                    WindowRes = PasteDialog.ShowDialog();
                    if (WindowRes == true)
                    {
                        if (PasteDialog.Action == PasteActionWindow.Skip)
                        {
                            SameEventsAction = 1;
                        }

                        else if (PasteDialog.Action == PasteActionWindow.Replace)
                        {
                            SameEventsAction = 2;
                        }
                    }
                    else
                        SameEventsAction = -1;

                    break;
                }
            }

            if (SameEventsAction != -1)
            {
                foreach (Event ItemToCopy in CopyBuffer)
                {
                    int NewItemIndex = -1;
                    bool SameTimeEvents = false;

                    foreach (Event EventToComp in Days[CurrentDay])
                    {
                        if ((ItemToCopy.Hours < EventToComp.Hours)
                         || ((ItemToCopy.Hours == EventToComp.Hours) && (ItemToCopy.Minutes < EventToComp.Minutes))
                         || ((ItemToCopy.Hours == EventToComp.Hours) && (ItemToCopy.Minutes == EventToComp.Minutes)))
                        {
                            NewItemIndex = Days[CurrentDay].IndexOf(EventToComp);

                            if (((ItemToCopy.Hours == EventToComp.Hours) && (ItemToCopy.Minutes == EventToComp.Minutes)))
                                SameTimeEvents = true;
                            break;
                        }
                    }

                    if ((SameEventsAction == 0) || ((SameEventsAction != 0) && (!SameTimeEvents)))
                    {
                        if (NewItemIndex != -1)
                            Days[CurrentDay].Insert(NewItemIndex, new Event() { Hours = ItemToCopy.Hours, Minutes = ItemToCopy.Minutes, RelayState = ItemToCopy.RelayState, IsActive = true });
                        else
                            Days[CurrentDay].Add(new Event() { Hours = ItemToCopy.Hours, Minutes = ItemToCopy.Minutes, RelayState = ItemToCopy.RelayState, IsActive = true });
                    }
                    else if (SameTimeEvents)
                    {
                        if (SameEventsAction == 1)
                        {

                        }
                        else if (SameEventsAction == 2)
                        {
                            if (NewItemIndex != -1)
                            {
                                Days[CurrentDay].RemoveAt(NewItemIndex);
                                Days[CurrentDay].Insert(NewItemIndex, new Event() { Hours = ItemToCopy.Hours, Minutes = ItemToCopy.Minutes, RelayState = ItemToCopy.RelayState, IsActive = true });
                            }
                        }
                    }
                }
            }

            return SameEventsAction;
        }

        private void TimeSetButton_Click(object sender, RoutedEventArgs e)
        {
            TimeSettings TimeSet = new TimeSettings();
            
            byte[] USBPacket = new byte[USB_PACKET_SIZE];
            USBPacket[0] = USB_REPORT_ID;
            USBPacket[USB_CMD_POS] = USB_CMD_TIME;
            USBPacket[USB_TIME_SUBCMD_POS] = TIME_SUBCMD_GET;

            Boolean USBSuccess = USB_device.Write(USBPacket);

            ShowConnectionState(USBSuccess);
            if (USBSuccess == false)
                return;

            USBSuccess = USB_device.Read(USBPacket);
            
            TimeSet.Seconds = USBPacket[USB_TIME_SECONDS_POS];
            TimeSet.Minutes = USBPacket[USB_TIME_MINUTES_POS];
            TimeSet.Hours = USBPacket[USB_TIME_HOURS_POS];
            TimeSet.WeekDay = USBPacket[USB_TIME_WEEKDAY_POS];
            TimeSet.Date = USBPacket[USB_TIME_DATE_POS];
            TimeSet.Month = USBPacket[USB_TIME_MONTH_POS];
            TimeSet.Year = USBPacket[USB_TIME_YEAR_POS];
            TimeSet.TimeZoneId = USBPacket[USB_TIME_ZONE_POS];

            TimeSetWindow TimeSettings = new TimeSetWindow(TimeSet);
            TimeSettings.Owner = this;
            bool? SetResult = TimeSettings.ShowDialog();

            if (SetResult == true)
            {
                USBPacket = new byte[USB_PACKET_SIZE];
                USBPacket[0] = USB_REPORT_ID;
                USBPacket[USB_CMD_POS] = USB_CMD_TIME;
                USBPacket[USB_TIME_SUBCMD_POS] = TIME_SUBCMD_SET;
                USBPacket[USB_TIME_SECONDS_POS] = TimeSet.Seconds;
                USBPacket[USB_TIME_MINUTES_POS] = TimeSet.Minutes;
                USBPacket[USB_TIME_HOURS_POS] = TimeSet.Hours;
                USBPacket[USB_TIME_WEEKDAY_POS] = TimeSet.WeekDay;
                USBPacket[USB_TIME_DATE_POS] = TimeSet.Date;
                USBPacket[USB_TIME_MONTH_POS] = TimeSet.Month;
                USBPacket[USB_TIME_YEAR_POS] = TimeSet.Year;
                USBPacket[USB_TIME_ZONE_POS] = TimeSet.TimeZoneId;

                USBSuccess = USB_device.Write(USBPacket);

                ShowConnectionState(USBSuccess);
                if (USBSuccess == false)
                    return;
            }
        }
        
        private void OpenFile(string FileToOpen)
        {
            for (byte day = 0; day < DAYS_NUM; day++)
                Days[day].Clear();

            using (StreamReader FileToReadFrom = File.OpenText(FileToOpen))
            {
                byte DayIndex = 0;
                bool IsLineAWeekDay;                
                bool ReadFile = true;

                while ((FileToReadFrom.Peek() >= 0) && (ReadFile))
                {
                    StringBuilder NewLine = new StringBuilder(FileToReadFrom.ReadLine());

                    NewLine.Replace(" ", "");

                    IsLineAWeekDay = false;

                    for (byte i = 0; i < DAYS_NUM; i++)
                    {
                        if (NewLine.ToString().IndexOf(WeekDaysStrings[i]) != -1)
                        {
                            DayIndex = i;
                            IsLineAWeekDay = true;
                            break;
                        }
                    }

                    if (IsLineAWeekDay == false)
                    {
                        Event DayEvent = new Event();
                        DayEvent.IsActive = true;

                        for (byte j = 0; j < 1; j++)
                        {
                            byte NumValue;
                            int StringIndex;

                            StringIndex = NewLine.ToString().IndexOf(":");
                            if (StringIndex == -1)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            if (byte.TryParse(NewLine.ToString().Substring(0, StringIndex), out NumValue) == false)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            DayEvent.Hours = NumValue;
                            NewLine.Remove(0, StringIndex + 1);

                            StringIndex = NewLine.ToString().IndexOf("Реле:");
                            if (StringIndex == -1)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            if (byte.TryParse(NewLine.ToString().Substring(0, StringIndex), out NumValue) == false)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            DayEvent.Minutes = NumValue;
                            NewLine.Remove(0, StringIndex + 5);

                            StringIndex = NewLine.ToString().IndexOf(";");
                            if (StringIndex == -1)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            int IndexInArray = Array.IndexOf(EventWindow.RelayStatesStrings, NewLine.ToString().Substring(0, StringIndex));
                            if (IndexInArray == -1)
                            {
                                DayEvent.IsActive = false;
                                break;
                            }

                            DayEvent.RelayState = EventWindow.RelayStatesStrings[IndexInArray];
                        }

                        if (DayEvent.IsActive == true)
                            Days[DayIndex].Add(DayEvent);
                    }
                }
            }
        }

        private void WindowIsClosing(object sender, CancelEventArgs e)
        {            
            ProgressBar.Close();
            USB_device.Close();
        }

        private void DropFile(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

                OpenFile(files[0]);
            }
        }

        private void FilePreviewDragOver(object sender, DragEventArgs e)
        {
            bool dropEnabled = true;
            if (e.Data.GetDataPresent(DataFormats.FileDrop, true))
            {
                string[] filenames = e.Data.GetData(DataFormats.FileDrop, true) as string[];

                foreach (string filename in filenames)
                {
                    if (System.IO.Path.GetExtension(filename).ToUpperInvariant() != ".TXT")
                    {
                        dropEnabled = false;
                        break;
                    }
                }
            }
            else
            {
                dropEnabled = false;
            }

            if (!dropEnabled)
            {
                e.Effects = DragDropEffects.None;
                e.Handled = true;
            }
        }

        private void FilePreviewDragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effects = DragDropEffects.Copy;
            }
            else
            {
                e.Effects = DragDropEffects.None;
            }
        }
        
        private void DeleteAllButton_Click(object sender, RoutedEventArgs e)
        {
            DeleteAllEvents();            
        }

        private void DeleteAllEvents()
        {
            foreach (ObservableCollection<Event> day in Days)
                day.Clear();
        }

        private void DownloadStart()
        {
            ThreadDay = 0;
            ThreadEventInDay = 0;
            ThreadSetting = THREAD_SET_EVENTS;

            byte[] USBPacket = new byte[USB_PACKET_SIZE];
            USBPacket[0] = USB_REPORT_ID;
            USBPacket[USB_CMD_POS] = USB_CMD_EEPROM_WRITE;
            USBPacket[USB_EEPROM_SUBCMD_POS] = EEPROM_SUBCMD_EVENTS_COUNT;
            USBPacket[USB_EEPROM_SUBCMD_POS + 1] = (byte)Days[0].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 2] = (byte)Days[1].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 3] = (byte)Days[2].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 4] = (byte)Days[3].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 5] = (byte)Days[4].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 6] = (byte)Days[5].Count;
            USBPacket[USB_EEPROM_SUBCMD_POS + 7] = (byte)Days[6].Count;
            USBPacket[USB_START_STOP_FLAG_POS] = USB_CMD_START_FLAG;
            Boolean USBSuccess = USB_device.Write(USBPacket);

            ShowConnectionState(USBSuccess);
            if (USBSuccess == false)
                return;

            ThreadWrite = true;
            ThreadTimer.Start();
        }

        private void ClearConfigButton_Click(object sender, RoutedEventArgs e)
        {
            DeleteAllEvents();
            DownloadStart();
        }

        private void ShowConnectionState(bool State)
        {
            if (State)
            {
                StateImage.Source = new BitmapImage(new Uri("images/apply_32x32.png", UriKind.Relative));
                StateTextBlock.Text = DeviceConnectedString;
            }

            else
            {
                StateImage.Source = new BitmapImage(new Uri("images/warning_32x32.png", UriKind.Relative));
                StateTextBlock.Text = DeviceNotConnectedString;
            }

            DownloadMenu.IsEnabled = State;
            UploadMenu.IsEnabled = State;
            ClearMenu.IsEnabled = State;
            TimeMenu.IsEnabled = State;

        }
    }
}
