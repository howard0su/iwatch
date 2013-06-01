#ifndef _BATTERY_H_
#define _BATTRY_H_
typedef enum 
  {
    BATTERY_DISCHARGING,
    BATTERY_CHARGING,
    BATTERY_FULL
  }BATTERY_STATE;
void battery_init(void);

// battery charging status
BATTERY_STATE battery_state(void);

// battery level
uint8_t       battery_level(void);
#endif
