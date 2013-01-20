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
#include "ioboard.h"
#include "config.h"

static UCHAR ucTheButtonStates;
void Check_Button(volatile UCHAR ucAddress_, UCHAR ucButtonOffset_);

void IOBoard_Init()
{
   // Initialize the LED's 
   IOBOARD_LED0_SEL &= ~IOBOARD_LED0_BIT;   // Set as output
   IOBOARD_LED0_DIR |= IOBOARD_LED0_BIT;    // Set as output
   IOBOARD_LED0_OUT |= IOBOARD_LED0_BIT;    // Port select HI for SYNC serial

// Can't configure LED1 in synchronous serial mode because it conflicts with SFLOW
//   IOBOARD_LED1_SEL &= ~IOBOARD_LED1_BIT;   // Set as output
//   IOBOARD_LED1_DIR |= IOBOARD_LED1_BIT;    // Set as output
//   IOBOARD_LED1_OUT |= IOBOARD_LED1_BIT;    // Port select HI for SYNC serial

#if defined(SERIAL_UART_ASYNC)
   IOBOARD_LED2_SEL &= ~IOBOARD_LED2_BIT;   // Set as output
   IOBOARD_LED2_DIR |= IOBOARD_LED2_BIT;    // Set as output
   IOBOARD_LED2_OUT |= IOBOARD_LED2_BIT;    // Port select HI for SYNC serial
   
#endif
   IOBOARD_LED3_SEL &= ~IOBOARD_LED3_BIT;   // Set as output
   IOBOARD_LED3_DIR |= IOBOARD_LED3_BIT;    // Set as output
   IOBOARD_LED3_OUT |= IOBOARD_LED3_BIT;    // Port select HI for SYNC serial


#if defined(SERIAL_UART_ASYNC)

   IOBOARD_BUTTON0_SEL &= ~IOBOARD_BUTTON0_BIT;   
   IOBOARD_BUTTON0_DIR &= ~IOBOARD_BUTTON0_BIT;     
        

   IOBOARD_BUTTON1_SEL &= ~IOBOARD_BUTTON1_BIT;    
   IOBOARD_BUTTON1_DIR &= ~IOBOARD_BUTTON1_BIT;     
        

   IOBOARD_BUTTON2_SEL &= ~IOBOARD_BUTTON2_BIT;    
   IOBOARD_BUTTON2_DIR &= ~IOBOARD_BUTTON2_BIT;     

#else


   IOBOARD_BUTTON1_SEL &= ~(IOBOARD_BUTTON1_BIT);   // Set as i/o
   IOBOARD_BUTTON1_DIR &= ~(IOBOARD_BUTTON1_BIT);   // Set as input
   
   IOBOARD_BUTTON2_SEL &= ~(IOBOARD_BUTTON2_BIT);   // Set as i/o
   IOBOARD_BUTTON2_DIR &= ~(IOBOARD_BUTTON2_BIT);   // Set as input
   
   IOBOARD_BUTTON3_SEL &= ~(IOBOARD_BUTTON3_BIT);   // Set as i/o
   IOBOARD_BUTTON3_DIR &= ~(IOBOARD_BUTTON3_BIT);   // Set as input
   
   
#endif
//   IOBOARD_BUTTON1_SEL &= ~IOBOARD_BUTTON1_BIT;   // Set as i/o
//   IOBOARD_BUTTON1_DIR &= ~IOBOARD_BUTTON1_BIT;   // Set as input
//   IOBOARD_BUTTON2_SEL &= ~IOBOARD_BUTTON2_BIT;   // Set as i/o
//   IOBOARD_BUTTON2_DIR &= ~IOBOARD_BUTTON2_BIT;   // Set as input
//   IOBOARD_BUTTON3_SEL &= ~IOBOARD_BUTTON3_BIT;   // Set as i/o
//   IOBOARD_BUTTON3_DIR &= ~IOBOARD_BUTTON3_BIT;   // Set as input
//


   ucTheButtonStates = 0;
}

UCHAR IOBoard_Button_Pressed()
{
   return(ucTheButtonStates);
}

void IOBoard_Button_Clear(UCHAR ucButtonOffset_)
{
   UCHAR ucMask = BUTTON_STATE_MASK << ucButtonOffset_;
   UCHAR ucWFC = BUTTON_STATE_WFC << ucButtonOffset_;
   
   ucTheButtonStates = ((ucTheButtonStates & ~ucMask) | ucWFC);
}


void IOBoard_Check_Button()
{


   

#if defined(SERIAL_UART_ASYNC)
   Check_Button((IOBOARD_BUTTON0_IN & IOBOARD_BUTTON0_BIT), BUTTON0_STATE_OFFSET);
#else
   Check_Button((IOBOARD_BUTTON3_IN & IOBOARD_BUTTON3_BIT), BUTTON3_STATE_OFFSET);
#endif

   Check_Button((IOBOARD_BUTTON1_IN & IOBOARD_BUTTON1_BIT), BUTTON1_STATE_OFFSET);
   Check_Button((IOBOARD_BUTTON2_IN & IOBOARD_BUTTON2_BIT), BUTTON2_STATE_OFFSET);

}

//void IOBoard_Led_Set(LedEnum eLed_, BOOL bOn_)
//{
//   switch(eLed_)
//   {
//      case LED_0:
//      {
//         if(bOn_)
//            IOBOARD_LED0_OUT &= ~IOBOARD_LED0_BIT;               
//         else
//            IOBOARD_LED0_OUT |= IOBOARD_LED0_BIT;                
//         break;
//      }
////      case LED_1:
////      {
////         if(bOn_)
////            IOBOARD_LED1_OUT &= ~IOBOARD_LED1_BIT;               
////         else
////            IOBOARD_LED1_OUT |= IOBOARD_LED1_BIT;                
////         break;
////      }
////      case LED_2:
////      {
////         if(bOn_)
////            IOBOARD_LED2_OUT &= ~IOBOARD_LED2_BIT;               
////         else
////            IOBOARD_LED2_OUT |= IOBOARD_LED2_BIT;                
////         break;
////      }
//      case LED_3:
//      {
//         if(bOn_)
//            IOBOARD_LED3_OUT &= ~IOBOARD_LED3_BIT;               
//         else
//            IOBOARD_LED3_OUT |= IOBOARD_LED3_BIT;                
//         break;
//      }
//      default:
//      {
//         break;
//      }
//   }
//}

void Check_Button(UCHAR ucPressed_, UCHAR ucButtonOffset_)
{
   

   UCHAR ucMask = BUTTON_STATE_MASK << ucButtonOffset_;
   UCHAR ucWFC = BUTTON_STATE_WFC << ucButtonOffset_;
   UCHAR ucPressed = BUTTON_STATE_PRESSED << ucButtonOffset_;


   if(!(ucTheButtonStates & ucMask))                     // If button is OFF
   {
      if(!(ucPressed_))             // Button has been pressed
      {
         ucTheButtonStates |= ucPressed;  
      }   
   }
   else if(ucTheButtonStates & ucPressed)
   {
      if(!(ucPressed_))             // Button is still pressed
      {
         ucTheButtonStates = ((ucTheButtonStates & ~ucMask) | ucWFC);
      }   
      else                                               // Button has been released
      {
         ucTheButtonStates &= ~ucMask; 
      }      
   }
   else if(ucTheButtonStates & ucWFC)
   {
      if(ucPressed_)                // Button is released
      {
         ucTheButtonStates &= ~ucMask; 
      }
      
   }



}


