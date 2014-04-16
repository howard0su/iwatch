#include <time.h>
#include <stdint.h>

#include "contiki.h"
#include "rtc.h"
#include "window.h"
#include "btstack/include/btstack/utils.h"
#include "contiki-conf.h"
#include "system.h"


PROCESS(rtc_process, "RTC Driver");
PROCESS_NAME(system_process);

#define __no_init    __attribute__ ((section (".noinit")))
__no_init static struct datetime now;
__no_init static uint16_t checksum;

static uint8_t source;
////////////////////////////////////////////////////////////
/* Calendar struct */
static struct tm calendar;
static time_t startTime;

/* Declare variables for time keeping */
static uint32_t  burtcCount = 0;
static uint32_t  burtcOverflowCounter = 0;
static uint32_t  burtcOverflowIntervalRem;
// static uint32_t  burtcOverflowInterval;
static uint32_t  burtcTimestamp;
static time_t    currentTime;
/* Time defines */
static uint32_t rtcOverflowCounter = 0;
static uint32_t overflow_interval;
static uint32_t overflow_interval_r;

static uint8_t   alarmEnable;

/* Clock Defines */
static time_t rtcStartTime = 0;

/* Clock defines */
#define LFXO_FREQUENCY 32768
#define BURTC_PRESCALING 128
#define UPDATE_INTERVAL 60
//#define UPDATE_INTERVAL 1
#define COUNTS_PER_SEC (LFXO_FREQUENCY/BURTC_PRESCALING)
#define COUNTS_BETWEEN_UPDATE (UPDATE_INTERVAL*COUNTS_PER_SEC)


/* Declare variables */
static uint32_t resetcause = 0;

/* Declare BURTC variables */
static uint32_t burtcCountAtWakeup = 0;

/* Calendar struct for initial date setting */
struct tm initialCalendar;



void budSetup(void);
void burtcSetup(void);

void clockRestore(uint32_t burtcCountAtWakeup);
uint32_t clockGetOverflowCounter(void);
time_t clockGetStartTime(void);
void clockSetStartTime(time_t offset);
void clockSetOverflowCounter(uint32_t of);
void clockSetCal(struct tm * timeptr);

void clockInit(struct tm * timeptr);
void clockBackup(void);

static uint16_t getChecksum()
{
	return 0;  	
}

void rtc_init()
{
  	/* Read and clear RMU->RSTCAUSE as early as possible */
  	resetcause = RMU->RSTCAUSE;
  	RMU_ResetCauseClear();
  	
  	/* Read Backup Real Time Counter value */
  	burtcCountAtWakeup = BURTC_CounterGet();  	
  	
  	/* Configure Backup Domain */
  	budSetup();  	
  	
  	/* Setting up a structure to initialize the calendar
     	   for January 1 2012 12:00:00
     	   The struct tm is declared in time.h
     	   More information for time.h library in http://en.wikipedia.org/wiki/Time.h */
  	initialCalendar.tm_sec    = 0;   /* 0 seconds (0-60, 60 = leap second)*/
  	initialCalendar.tm_min    = 0;   /* 0 minutes (0-59) */
  	initialCalendar.tm_hour   = 12;  /* 6 hours (0-23) */
  	initialCalendar.tm_mday   = 1;   /* 1st day of the month (1 - 31) */
  	initialCalendar.tm_mon    = 0;   /* January (0 - 11, 0 = January) */
  	initialCalendar.tm_year   = 114; /* Year 2012 (year since 1900) */
  	initialCalendar.tm_wday   = 0;   /* Sunday (0 - 6, 0 = Sunday) */
  	initialCalendar.tm_yday   = 0;   /* 1st day of the year (0-365) */
  	initialCalendar.tm_isdst  = -1;  /* Daylight saving time; enabled (>0), disabled (=0) or unknown (<0) */
  	
	/* Set the calendar */
  	clockInit(&initialCalendar);  	
  	
  	/* Compute overflow interval (integer) and remainder */
  	// burtcOverflowInterval  =  ((uint64_t)UINT32_MAX+1)/COUNTS_BETWEEN_UPDATE; /* in seconds */
  	burtcOverflowIntervalRem = ((uint64_t)UINT32_MAX+1)%COUNTS_BETWEEN_UPDATE;

  	// burtcSetComp( COUNTS_BETWEEN_UPDATE );
  	BURTC_CompareSet(0, COUNTS_BETWEEN_UPDATE );  	
  	alarmEnable = 0;
  	/* If waking from backup mode, restore time from retention registers */
  	if ( !(resetcause & RMU_RSTCAUSE_BUBODBUVIN) && (resetcause & RMU_RSTCAUSE_BUMODERST) )
  	{
    		/* Check if retention registers were being written to when backup mode was entered */
    		if ( (BURTC->STATUS & BURTC_STATUS_RAMWERR) >> _BURTC_STATUS_RAMWERR_SHIFT )
		{
		}

    		/* Check if timestamp is written */
    		if (! ((BURTC->STATUS & BURTC_STATUS_BUMODETS) >> _BURTC_STATUS_BUMODETS_SHIFT) )
    		{
    		}

    		/* Restore time from backup RTC + retention memory and print backup info*/
    		clockRestore( burtcCountAtWakeup );
    		
    		/*    		
    		clockAppRestore( burtcCountAtWakeup );
    		*/

    		/* Reset timestamp */
    		BURTC_StatusClear();
  	}

  	/* If normal startup, initialize BURTC */
  	else
  	{
    		/* Setup BURTC */
    		burtcSetup();

    		/* Backup initial calendar (also to initialize retention registers) */
    		clockBackup();

    		/* Update display if necessary */
    		//clockAppDisplay();
  	}  	
  	
  	/* Enable BURTC interrupts */
  	NVIC_ClearPendingIRQ(BURTC_IRQn);
  	NVIC_EnableIRQ(BURTC_IRQn);  	

  	process_start(&rtc_process, NULL);

}

PROCESS_THREAD(rtc_process, ev, data)
{
  	PROCESS_BEGIN();

  	while(1)
  	{
    		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
    		if (source == 0)
    		{
      			checksum = getChecksum();
      			process_post(ui_process, EVENT_TIME_CHANGED, &now);
    		}
    		else
    		{
      			// notification of alarm
      			window_notify("Alarm", "Alarm triggered.", NOTIFY_OK, 0);
    		}
  	}
  	PROCESS_END();
}

uint8_t rtc_getweekday(uint16_t year, uint8_t month, uint8_t day)
{
  	if( month == 1 || month == 2 )
  	{
    		month += 12;
    		year--;
  	}

  	return 1 + (( day + 2*month + 3*(month+1)/5 + year + year/4 ) %7);
}

void rtc_save()
{
  	/* Write overflow counter to retention memory */
  	BURTC_RetRegSet(0, clockGetOverflowCounter() );

  	/* Write local epoch offset to retention memory */
  	BURTC_RetRegSet(1, clockGetStartTime());	
}

void rtc_setdate(uint16_t year, uint8_t month, uint8_t day)
{
  	uint8_t weekday;

	startTime = clockGetStartTime( );
	calendar = *localtime( &startTime );
	now.year = calendar.tm_year = year;
	now.month = calendar.tm_mon = month;
	now.day = calendar.tm_mday = day;
	
	/* Set new epoch offset */
  	startTime = mktime(&calendar);
  	clockSetStartTime( startTime );
  	clockBackup( );	
  	process_post(ui_process, EVENT_TIME_CHANGED, &now);
}

void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec)
{
	startTime = clockGetStartTime( );
	calendar = *localtime( &startTime );
	now.hour = calendar.tm_hour = hour;
	now.minute = calendar.tm_min = min;
	now.second = calendar.tm_sec = sec;
	
	/* Set new epoch offset */
  	startTime = mktime(&calendar);
  	clockSetStartTime( startTime );
  	clockBackup( );		
  	process_post(ui_process, EVENT_TIME_CHANGED, &now);
}

uint8_t rtc_getmaxday(uint16_t year, uint8_t month)
{
  	if (month == 2)
  	{
    		if ((year%4==0) && (year%100!=0 || year%400==0) )
      			return 29;
    		else
      			return 28;
  	}
  	if(month==8)
  	{
    		return 31;
  	}

  	if (month % 2 == 0)
  	{
    		return 30;
  	}
  	else
  	{
    		return 31;
  	}
}

void rtc_setalarm(uint8_t aday, uint8_t adow, uint8_t ahour, uint8_t aminute)
{
  	int enable = 0;

  	if (adow & 0x80) 
  	{
    		now.adow = adow;
    		alarmEnable |= 0x04;
  	}

  	if (aday & 0x80)
  	{
   		now.aday = aday;
   		alarmEnable |= 0x08;
  	}

  	if (aminute & 0x80)
  	{
    		now.aminute = aminute;
    		enable = 1;
  	}
  
  	if (ahour & 0x80)
  	{
    		now.ahour = ahour;
    		enable = 1;
  	}
  	if (enable)
  	{
  	}	

}

void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
	
   	currentTime = time( NULL );
    	calendar = *localtime( &currentTime );	

  	if (hour) 
  		*hour = calendar.tm_hour;
  	if (min) 
  		*min = calendar.tm_min;
  	if (sec) 
  		*sec = calendar.tm_sec;
  		    	
  		
}

void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday)
{
    	currentTime = time( NULL );
    	calendar = *localtime( &currentTime );	
    	
  	if (year) 
  		*year = calendar.tm_year;
  	if (month) 
  		*month = calendar.tm_mon;
  	if (day) 
  		*day = calendar.tm_mday;
  	if (weekday) 
  		*weekday = calendar.tm_wday;
  		
}

static const uint8_t month_day_map[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

uint32_t calc_timestamp(uint8_t year, uint8_t month, uint8_t day, uint8_t hh, uint8_t mm, uint8_t ss)
{
    	uint8_t leap_years = year / 4 + 1;
    	if (year % 4 == 0 && month < 2)
        	leap_years -= 1;

    	uint32_t days = year * 365 + leap_years;
    	for (uint8_t i = 0; i < month - 1; ++i)
		days += month_day_map[i];
    	days += day;
    	return ((days * 24 + hh) * 60 + mm) * 60 + ss;

}

void parse_timestamp(uint32_t time, uint8_t* year, uint8_t* month, uint8_t* day, uint8_t* hh, uint8_t* mm, uint8_t* ss)
{
    	uint32_t temp = 0;

    	*ss = time % 60;
    	temp = time / 60;

    	*mm = temp % 60;
    	temp = temp / 60;

    	*hh = temp % 24;
    	temp = temp / 24;

    	uint16_t total_day = temp;
    	uint8_t tyear = 0;
    	while(1)
    	{
        	if (tyear % 4 == 0)
        	{
            		if (total_day >= 366)
                		total_day -= 366;
            		else
                		break;
        	}
        	else
        	{
            		if (total_day >= 365)
                		total_day -= 365;
            		else
                		break;
        	}
        	tyear++;
    	}

    	uint8_t i = 0;
    	for (; i < count_elem(month_day_map); ++i)
    	{
        	if (tyear % 4 == 0 && i == 1)
        	{
            		if (total_day < month_day_map[i] + 1)
                		break;
            		total_day -= month_day_map[i] + 1;
        	}
        	else
        	{
            		if (total_day < month_day_map[i])
                		break;
            		total_day -= month_day_map[i];
        	}
    	}

    	*year  = tyear;
    	*month = i + 1;
    	*day   = total_day;

}

uint32_t rtc_readtime32()
{
	uint16_t year  = 0;
	uint8_t  month = 0;
	uint8_t  day   = 0;
	uint8_t  wday  = 0;
	rtc_readdate(&year, &month, &day, &wday);

    	uint8_t  hour  = 0;
	uint8_t  min   = 0;
	uint8_t  sec   = 0;
	rtc_readtime(&hour, &min, &sec);
	return calc_timestamp(year - 2000, month, day, hour, min, sec);
}

void rtc_enablechange(uint8_t changes)
{

/*  	
  	if (changes & MINUTE_CHANGE)
  	{
    		RTCCTL01 |= RTCTEV__MIN + RTCTEVIFG + RTCTEVIE;
  	}
  	else
  	{
    		RTCCTL01 &= ~(RTCTEV__MIN + RTCTEVIE);
  	}

  	if (changes & SECOND_CHANGE)
  	{
    		RTCCTL01 |= RTCRDYIE + RTCRDYIFG;
  	}
  	else
  	{
    		RTCCTL01 &= ~RTCRDYIE;
  	}

  	if (changes & TENMSECOND_CHANGE)
  	{
    		RTCPS1CTL = RT1PSIE | RT1IP1;
  	}
  	else
  	{
    		RTCPS1CTL &= ~RT1PSIE;
  	}
*/  	
}

/**************************************************************************//**
 * @brief  Restore CALENDAR from retention registers
 *
 * @param[in] burtcCountAtWakeup BURTC value at power up. Only used for printout
 *****************************************************************************/
void clockRestore(uint32_t burtcCountAtWakeup)
{
  	uint32_t burtcStart;
  	uint32_t nextUpdate;
  	(void)burtcCountAtWakeup;                       /* Unused parameter */


  	/* Store BURTC->CNT for consistency in display output within this function */
  	burtcCount = BURTC_CounterGet();

  	/* Timestamp is BURTC value at time of main power loss */
  	burtcTimestamp = BURTC_TimestampGet();

  	/* Read overflow counter from retention memory */
  	burtcOverflowCounter = BURTC_RetRegGet(0);

  	/* Check for overflow while in backup mode
     	Assume that overflow interval >> backup source capacity
     	i.e. that overflow has only occured once during main power loss */
  	if ( burtcCount < burtcTimestamp )
  	{
    		burtcOverflowCounter++;
  	}

  	/* Restore epoch offset from retention memory */
  	clockSetStartTime(BURTC_RetRegGet(1));

  	/* Restore clock overflow counter */
  	clockSetOverflowCounter(burtcOverflowCounter);

  	/* Calculate start point for current BURTC count cycle
     	If (COUNTS_BETWEEN_UPDATE/burtcOverflowInterval) is not an integer,
     	BURTC value at first update is different between each count cycle */
  	burtcStart = (burtcOverflowCounter * (COUNTS_BETWEEN_UPDATE - burtcOverflowIntervalRem)) % COUNTS_BETWEEN_UPDATE;

  	/*  Calculate next update compare value
      	Add 1 extra UPDATE_INTERVAL to be sure that counter doesn't
      	pass COMP value before interrupts are enabled */
  	nextUpdate = burtcStart + ((burtcCount / COUNTS_BETWEEN_UPDATE) +1 ) * COUNTS_BETWEEN_UPDATE ;

  	BURTC_CompareSet(0, nextUpdate);
}


void clockBackup(void)
{
  	/* Write overflow counter to retention memory */
  	BURTC_RetRegSet(0, clockGetOverflowCounter() );

  	/* Write local epoch offset to retention memory */
  	BURTC_RetRegSet(1, clockGetStartTime());
}

/***************************************************************************//**
 * @brief Initialize system CLOCK
 *
 * @param[in] timeptr
 *   Calendar struct that is used to set the start time of the counter.
 *
 ******************************************************************************/
void clockInit(struct tm * timeptr)
{
  	/* Reset variables */
  	rtcOverflowCounter = 0;

  	/* Set overflow interval based on counter width and frequency */
  	overflow_interval  =  ((uint64_t)UINT32_MAX+1) / COUNTS_PER_SEC; /* in seconds */
  	overflow_interval_r = ((uint64_t)UINT32_MAX+1) % COUNTS_PER_SEC; /* division remainder */

  	/* Set epoch offset */
  	clockSetCal(timeptr);
}


/***************************************************************************//**
 * @brief Set the epoch offset
 *
 * @param[in] timeptr
 *   Calendar struct which is converted to unix time and used as new epoch
 *   offset
 *
 ******************************************************************************/
void clockSetCal(struct tm * timeptr)
{
  	rtcStartTime = mktime(timeptr);
}

/***************************************************************************//**
 * @brief Set the epoch offset
 *
 * @param[in] offset
 *   unix time when the counter was started
 *
 ******************************************************************************/
void clockSetStartTime(time_t offset)
{
  	rtcStartTime = offset;
}



/***************************************************************************//**
 * @brief Get the epoch offset
 *
 * @return
 *   unix time when the counter was started
 *
 ******************************************************************************/
time_t clockGetStartTime(void)
{
  	return rtcStartTime;
}

/***************************************************************************//**
 * @brief Call this function on counter overflow to let CLOCK know how many
 *        overflows has occured since start time
 *
 ******************************************************************************/
uint32_t clockOverflow(void)
{
  	rtcOverflowCounter++;
  	return rtcOverflowCounter;
}



/***************************************************************************//**
 * @brief Call this function on counter overflow to let CLOCK know how many
 *        overflows has occured since start time
 *
 ******************************************************************************/
void clockSetOverflowCounter(uint32_t of)
{
  	rtcOverflowCounter = of;
}



/***************************************************************************//**
 * @brief Call this function on counter overflow to let CLOCK know how many
 *        overflows has occured since start time
 *
 ******************************************************************************/
uint32_t clockGetOverflowCounter(void)
{
  	return rtcOverflowCounter;
}

time_t time( time_t * timer )
{
  	time_t t;

  	/* Add the time offset */
  	t = rtcStartTime;

  	/* Add time based on number of counter overflows*/
  	t += rtcOverflowCounter * overflow_interval;

  	/* Correct if overflow interval is not an integer*/
  	if ( overflow_interval_r != 0 )	
  	{
    		t += rtcOverflowCounter * overflow_interval_r / COUNTS_PER_SEC;
  	}

  	/* Add the number of seconds for BURTC */
  	t += (BURTC->CNT / COUNTS_PER_SEC);

  	/* Copy system time to timer if not NULL*/
  	if ( !timer )
  	{
    		timer = &t;
  	}

  	return t;	
}

/**************************************************************************//**
 * @brief   Configure backup RTC
 *****************************************************************************/
void burtcSetup(void)
{
  	BURTC_Init_TypeDef burtcInit = BURTC_INIT_DEFAULT;

  	/* Select LFXO as clock source for BURTC */
  	burtcInit.clkSel = burtcClkSelLFXO;
  	/* Enable BURTC operation in EM0-EM4 */
  	burtcInit.mode = burtcModeEM4;
  	/* Set prescaler to max. Resolution is not that important here */
  	burtcInit.clkDiv = 128;
  	/* Enable BURTC timestamp upon backup mode entry*/
  	burtcInit.timeStamp = true;
  	/* Counter doesn't wrap around when CNT == COMP0 */
  	burtcInit.compare0Top = false;
  	burtcInit.enable = true;

  	/* Enable interrupt on compare match */
  	BURTC_IntClear(BURTC_IEN_COMP0);
  	BURTC_IntEnable(BURTC_IEN_COMP0);
  	BURTC_Init(&burtcInit);
}

/***************************************************************************//**
 * @brief Set up backup domain.
 ******************************************************************************/
void budSetup(void)
{
	EMU_EM4Init_TypeDef em4Init = EMU_EM4INIT_DEFAULT;
	EMU_BUPDInit_TypeDef bupdInit = EMU_BUPDINIT_DEFAULT;
	
	/* Unlock configuration */
	EMU_EM4Lock(false);
	
	/* Enable backup status pin */
	bupdInit.statusPinEnable = false;
	
	/* Enable backup power domain */
	bupdInit.enable = true;
	
	/* Normal operation: Connect main power to backup power through diode */
	bupdInit.inactivePower = emuPower_MainBU;
	
	/* Backup mode: No connection between backup power and main power */
	bupdInit.activePower = emuPower_None;
	
	/* Set backup "charging" resistor */
	bupdInit.resistor = emuRes_Res3;
	
	EMU_BUPDInit(&bupdInit);
	
	/* Wait until backup power functionality is ready */
	EMU_BUReady();
	
	/* Release reset for backup domain */
	RMU_ResetControl(rmuResetBU, false);
	
	/* Enable BU_VIN pin */
	bupdInit.enable = true;
	
	/* Enable voltage regulator in backup mode */
	em4Init.vreg = true;
	
	/* Configure oscillators in EM4 */
	em4Init.osc = emuEM4Osc_LFXO;
	
	/* Lock configuration in case of brown out */
	em4Init.lockConfig = true;
	
	EMU_EM4Init(&em4Init);
}

void BURTC_IRQHandler(void)
{
  	uint32_t irq;

  	irq = BURTC_IntGet();
  	BURTC_IntClear(irq);	
  	
  	/* Interrupt source: compare match 	*/
  	/*   Increment compare value and	*/
   	/*   update TFT display            	*/
  	if ( irq & BURTC_IF_COMP0 )
  	{
    		BURTC_CompareSet(0, BURTC->COMP0 + COUNTS_BETWEEN_UPDATE );
    		process_poll(&rtc_process);
  	}

  	/* Interrupt source: counter overflow 	*/
  	/*   Increase overflow counter		*/
   	/*   and backup calendar              	*/
  	if ( irq & BURTC_IF_OF )
  	{
    		clockOverflow( );
    		clockBackup();
  	}  	
}