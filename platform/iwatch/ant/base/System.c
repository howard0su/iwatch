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
#include "Config.h"

//------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
/// System_Init
///
/// This function is used initialize the clocks and watchdog parameters
///
/////////////////////////////////////////////////////////////////////////////
void System_Init(void)
{
	// Setup the Main System Clock
   SYSTEM_DCOCTL = CALDCO_8MHZ;              // Set DCO to highest frequency (max speed)
   SYSTEM_BCSCTL1 = CALBC1_8MHZ | XT2OFF;  
   SYSTEM_BCSCTL1 &= ~SYSTEM_XTS;             // selecte low frequency mode for external clock source
   SYSTEM_BCSCTL1 &= ~(SYSTEM_DIVA0  | SYSTEM_DIVA1);          // set ACLK divider to 1
 
   SYSTEM_BCSCTL3 |= XCAP0 | XCAP1;          // Set LFXT1 cap at 12.5 pF  
 
//   SYSTEM_BCSCTL2 |= DIVS_3;                // set SMCLK divider to 8
   
   SYSTEM_IE1  &= ~SYSTEM_OFIE;              // Disable Oscillator Fault Interrupt
   SYSTEM_IFG1 &= ~SYSTEM_OFIFG;             // Clear the Oscillator Fault Interrupt
	
   SYSTEM_IE1  &= ~SYSTEM_WATCHDOG_WDTIE;    // disable watchdog interrupt
   SYSTEM_IFG1 &= ~SYSTEM_WATCHDOG_WDTIFG;   // clear the watchdog interrupt flag
}

//------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
/// System_DisableWatchdog
///
/// This function disables the watchdog
///
/////////////////////////////////////////////////////////////////////////////
void System_DisableWatchdog(void)
{
   SYSTEM_WATCHDOG_TCTL = SYSTEM_WATCHDOG_PW | SYSTEM_WATCHDOG_HOLD;
}
