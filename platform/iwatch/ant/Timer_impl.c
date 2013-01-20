#include "base/timer.h"


UCHAR                                  // Alarm number (0 if alarm failed to get assigned)
Timer_RegisterAlarm(
   EVENT_FUNC pfCallback_,             // Pointer to function to call on alarm elapsed event
   UCHAR ucFlags_)                    // Alarm function flags
{
  return 1;
}

BOOL                                   // TRUE if alarm unregistered succesfully 
Timer_UnRegisterAlarm(
   UCHAR ucAlarmNumber_)              // Alarm number
{
}

BOOL                                   // Alarm started successfully
Timer_Start(
   UCHAR ucAlarmNumber_,               // Alarm number
   USHORT ucCount_)                   // Alarm elapsed count
{
  
}

USHORT                                 // 1024 tick count
Timer_GetTickCount()
{
}

void 
Timer_Stop(
   UCHAR ucAlarmNumber_)              // Alarm number
{
}
