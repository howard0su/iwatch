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
#ifndef __IOBOARD_H__
#define __IOBOARD_H__

#include "types.h"

#define BUTTON_STATE_MASK    ((UCHAR) 0x03) // Button 1 bit mask
#define BUTTON_STATE_PRESSED ((UCHAR) 0x01) // Button has been pressed
#define BUTTON_STATE_WFC     ((UCHAR) 0x02) // Waiting for button to be released



#define BUTTON1_STATE_MASK    ((UCHAR) 0x03) // Button 1 bit mask
#define BUTTON1_STATE_OFFSET  ((UCHAR) 0x00) // Button 1 bit mask
#define BUTTON1_STATE_PRESSED ((UCHAR) 0x01) // Button has been pressed
#define BUTTON1_STATE_WFC     ((UCHAR) 0x02) // Waiting for button to be released

#define BUTTON2_STATE_MASK    ((UCHAR) 0x0C) // Button 2 bit mask
#define BUTTON2_STATE_OFFSET  ((UCHAR) 0x02) // Button 2 bit mask
#define BUTTON2_STATE_PRESSED ((UCHAR) 0x04) // Button has been pressed
#define BUTTON2_STATE_WFC     ((UCHAR) 0x08) // Waiting for button to be released

#define BUTTON3_STATE_MASK    ((UCHAR) 0x30) // Button 3 bit mask
#define BUTTON3_STATE_OFFSET  ((UCHAR) 0x04) // Button 3 bit mask
#define BUTTON3_STATE_PRESSED ((UCHAR) 0x10) // Button has been pressed
#define BUTTON3_STATE_WFC     ((UCHAR) 0x20) // Waiting for button to be released

#define BUTTON0_STATE_MASK    ((UCHAR) 0xC0) // Button 3 bit mask
#define BUTTON0_STATE_OFFSET  ((UCHAR) 0x06) // Button 3 bit mask
#define BUTTON0_STATE_PRESSED ((UCHAR) 0x40) // Button has been pressed
#define BUTTON0_STATE_WFC     ((UCHAR) 0x80) // Waiting for button to be released




typedef enum
{
   LED_0,
//   LED_1, 
//   LED_2,
   LED_3
}  LedEnum;


void IOBoard_Init();

UCHAR IOBoard_Button_Pressed();
void IOBoard_Button_Clear(UCHAR ucButtonWFC_);
void IOBoard_Check_Button();
//void IOBoard_Led_Set(LedEnum eLed_, BOOL bOn_);


#endif




























//
























