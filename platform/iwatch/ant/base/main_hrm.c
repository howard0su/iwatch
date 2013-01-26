/////////////////////////////////////////////////////////////////////////////////////////
// THE FOLLOWING EXAMPLE CODE IS INTENDED FOR LIMITED CIRCULATION ONLY.
// 
// Please forward all questions regarding this code to ANT Technical Support.
// 
// Dynastream Innovations Inc.
// 228 River Avenue
// Cochrane, Alberta, Canada
// T4C 2C1
// 
// (P) (403) 932-9292
// (F) (403) 932-6521
// (TF) 1-866-932-9292
// (E) support@thisisant.com
// 
// www.thisisant.com
//
// Reference Design Disclaimer
//
// The references designs and codes provided may be used with ANT devices only and remain the copyrighted property of 
// Dynastream Innovations Inc. The reference designs and codes are being provided on an "as-is" basis and as an accommodation, 
// and therefore all warranties, representations, or guarantees of any kind (whether express, implied or statutory) including, 
// without limitation, warranties of merchantability, non-infringement,
// or fitness for a particular purpose, are specifically disclaimed.
//
// ©2008 Dynastream Innovations Inc. All Rights Reserved
// This software may not be reproduced by
// any means without express written approval of Dynastream
// Innovations Inc.
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "types.h"
#include "antinterface.h"
#include "antdefines.h"
#include "timer.h"
#include "printf.h"
#include "hrm_rx.h"

#include "contiki.h"


// ANT Channel settings
#define ANT_CHANNEL_HRMRX                          ((UCHAR) 0)        // Default ANT Channel

// other defines
#define HRM_PRECISION                              ((ULONG)1000)
 
 
static const UCHAR aucNetworkKey[] = ANTPLUS_NETWORK_KEY;
  
static void ProcessANTHRMRXEvents(ANTPLUS_EVENT_RETURN* pstEvent_);
static void ProcessAntEvents(UCHAR* pucEventBuffer_);

PROCESS(ant_process, "ANT process");

void ant_process_poll()
{
  process_poll(&ant_process);
}

void ant_init()
{
  ANTInterface_Init();
  
  process_start(&ant_process, NULL);	   
}

//----------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////
// main
//
// main function  
//
// Configures device simulator and HRM TX channel.
//
// \return: This function does not return. 
////////////////////////////////////////////////////////////////////////////
PROCESS_THREAD(ant_process, ev, data)
{
   static UCHAR* pucRxBuffer;
   static ANTPLUS_EVENT_RETURN stEventStruct;

   PROCESS_BEGIN();
   // Main loop
   ANT_Reset();
   
   while(TRUE)
   {
      pucRxBuffer = ANTInterface_Transaction();                // Check if any data has been recieved from serial
      
      if(pucRxBuffer)
      {
		
	HRMRX_ChannelEvent(pucRxBuffer, &stEventStruct);
	ProcessANTHRMRXEvents(&stEventStruct);
	
	ProcessAntEvents(pucRxBuffer);
	ANTInterface_Complete();                              // Release the serial buffer
	
	continue;
      }  
	  
      PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
   }
   
   PROCESS_END();
} 


////////////////////////////////////////////////////////////////////////////
// ProcessANTHRMRXEvents
//
// HRM Reciever event processor  
//
// Processes events recieved from HRM module.
//
// \return: N/A 
///////////////////////////////////////////////////////////////////////////
void ProcessANTHRMRXEvents(ANTPLUS_EVENT_RETURN* pstEvent_)
{
   static UCHAR ucPreviousBeatCount = 0;
   
   switch (pstEvent_->eEvent)
   {

      case ANTPLUS_EVENT_CHANNEL_ID:
      {
         // Can store this device number for future pairings so that 
         // wild carding is not necessary.
         printf("Device Number is %d\n", pstEvent_->usParam1);
         printf("Transmission type is %d\n\n", pstEvent_->usParam2);
         break;
      }
      case ANTPLUS_EVENT_PAGE:
      {
         HRMPage0_Data* pstPage0Data = HRMRX_GetPage0(); //common data
         BOOL bCommonPage = FALSE;

         //print formulated page identifier
         if (pstEvent_->usParam1 <= HRM_PAGE_4)
            printf("Heart Rate Monitor Page %d\n", pstEvent_->usParam1);

         // Get data correspinding to the page. Only get the data you 
         // care about.
         switch(pstEvent_->usParam1)
         {
            case HRM_PAGE_0:
            {
               bCommonPage = TRUE;
               break;
            }
            case HRM_PAGE_1:
            {
               HRMPage1_Data* pstPage1Data = HRMRX_GetPage1();
               ULONG ulMinutes, ulHours, ulDays, ulSeconds;

               ulDays = (ULONG)((pstPage1Data->ulOperatingTime) / 86400);  //1 day == 86400s
               ulHours = (ULONG)((pstPage1Data->ulOperatingTime) % 86400); // half the calculation so far
               ulMinutes = ulHours % (ULONG)3600;
               ulSeconds = ulMinutes % (ULONG)60;
               ulHours /= (ULONG)3600; //finish the calculations: hours = 1hr == 3600s
               ulMinutes /= (ULONG)60; //finish the calculations: minutes = 1min == 60s

               printf("Cumulative operating time: %dd ", ulDays);
               printf("%dh ", ulHours);
               printf("%dm ", ulMinutes);
               printf("%ds\n\n", ulSeconds);
               bCommonPage = TRUE;
               break;
            }
            case HRM_PAGE_2:
            {
               HRMPage2_Data* pstPage2Data = HRMRX_GetPage2();

               printf("Manufacturer ID: %u\n", pstPage2Data->ucManId);
               printf("Serial No (upper 16-bits): 0x%X\n", pstPage2Data->ulSerialNumber);               
               bCommonPage = TRUE;
               break;
            }
            case HRM_PAGE_3:
            {
               HRMPage3_Data* pstPage3Data = HRMRX_GetPage3();

               printf("Hardware Rev ID %u ", pstPage3Data->ucHwVersion);
               printf("Model %u\n", pstPage3Data->ucModelNumber);
               printf("Software Ver ID %u\n", pstPage3Data->ucSwVersion);
               bCommonPage = TRUE;
               break;
            }
            case HRM_PAGE_4:
            {
               HRMPage4_Data* pstPage4Data = HRMRX_GetPage4();
               
               printf("Previous heart beat event: %u.", (ULONG)(pstPage4Data->usPreviousBeat/1024));
               printf("%03u s\n", (ULONG)((((pstPage4Data->usPreviousBeat % 1024) * HRM_PRECISION) + 512) / 1024));
               
               if((pstPage0Data->ucBeatCount - ucPreviousBeatCount) == 1)	// ensure that there is only one beat between time intervals
               {
                  USHORT usR_RInterval = pstPage0Data->usBeatTime - pstPage4Data->usPreviousBeat;	// subtracting the event time gives the R-R interval
                  printf("R-R Interval: %u.", (ULONG)(usR_RInterval/1024));
                  printf("%03u s\n", (ULONG)((((usR_RInterval % 1024) * HRM_PRECISION) + 512) / 1024));
               }
               ucPreviousBeatCount = pstPage0Data->ucBeatCount;
                              
               bCommonPage = TRUE;
               break;
            }
           default:
            {
               // ASSUME PAGE 0
               printf("Unknown format\n\n");
               break; 
            }
         }
         if(bCommonPage)
         {
            printf("Time of last heart beat event: %u.", (ULONG)(pstPage0Data->usBeatTime/1024));
            printf("%03u s\n", (ULONG)((((pstPage0Data->usBeatTime % 1024) * HRM_PRECISION) + 512) / 1024));
            printf("Heart beat count: %u\n", pstPage0Data->ucBeatCount);
            printf("Instantaneous heart rate: %u bpm\n\n", pstPage0Data->ucComputedHeartRate);
         }
         break;
      }

      case ANTPLUS_EVENT_UNKNOWN_PAGE:  // Decode unknown page manually
      case ANTPLUS_EVENT_NONE:
      default:
      {
     	 break;
      }  
   }
}

void ProcessAntEvents(UCHAR* pucEventBuffer_)
{
   
   if(pucEventBuffer_)
   {
      UCHAR ucANTEvent = pucEventBuffer_[BUFFER_INDEX_MESG_ID];   
      switch( ucANTEvent )
      {
         case MESG_RESPONSE_EVENT_ID:
         {
            switch(pucEventBuffer_[BUFFER_INDEX_RESPONSE_CODE])
            {
               case EVENT_RX_SEARCH_TIMEOUT:
               {
                  break;
               }
               case EVENT_TX:
               {
                  break;
               }
               
               case RESPONSE_NO_ERROR:
               {   
               	  if (pucEventBuffer_[3] == MESG_OPEN_CHANNEL_ID)
                  {
                     printf("initialization is complete.\n");
                  }
                  else if (pucEventBuffer_[3] == MESG_CLOSE_CHANNEL_ID)
                  {
                  }
                  else if (pucEventBuffer_[3] == MESG_NETWORK_KEY_ID)
                  {
                     //Once we get a response to the set network key
                     //command, start opening the HRM channel
                     HRMRX_Open(ANT_CHANNEL_HRMRX, 0, 0); 
                  }
                  break;
               }
            }
         	break;
		 }
	  case MESG_STARTUP_ID:
		{
		  if (pucEventBuffer_[2] == 0)
		  {
			printf("Reset is complete.\n");
		  }
		  else 
		  {
			if (pucEventBuffer_[2] & BIT0)
		  	{
				printf("HARDWARE_RESET_LINE ");
		  	}
			if (pucEventBuffer_[2] & BIT1)
		  	{
				printf("WATCH_DOG_RESET ");
		  	}
			if (pucEventBuffer_[2] & BIT5)
		  	{
				printf("COMMAND_RESET ");
		  	}
			if (pucEventBuffer_[2] & BIT6)
		  	{
				printf("SYNCHRONOUS_RESET ");
		  	}
			if (pucEventBuffer_[2] & BIT7)
		  	{
				printf(" SUSPEND_RESET ");
		  	}
			
			printf("\n");
		  }
		  ANT_NetworkKey(ANTPLUS_NETWORK_NUMBER, aucNetworkKey);
          break;
		}
      }
   }      
}
