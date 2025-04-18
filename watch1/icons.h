#ifndef _ICONS_H_
#define _ICONS_H_

#define ICON_RUN        'h'
#define ICON_ALARM      'i'
#define ICON_BT         'j'
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
#define ICON_STEPS	'a'
#define ICON_TIME       'b'
#define ICON_CALORIES 	'c'
#define ICON_DISTANCE   'd'
#else
#define ICON_STEPS		'y'
#define ICON_TIME       'z'
#define ICON_CALORIES 	'z'+1
#define ICON_DISTANCE   'z'+2
#endif


#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
#define TYPE_CLOUDY   		'o'
#define TYPE_RAIN		'p'
#define TYPE_MOSTLY_CLOUDY	'q'
#define TYPE_SUNNY		'r'
#define TYPE_MOSTLY_SUNNYY	's'
#define TYPE_SNOW		't'  
#define TYPE_SHOWER		'u'  
#define TYPE_THUNDER_STORN 	'v' 
#endif

#define ICON_BATTERY_FULL 'm'
#define ICON_BATTERY_MORE 'n'
#define ICON_BATTERY_LESS 'o'
#define ICON_BATTERY_EMPTY 'p'
#define ICON_BATTERY_CHARGE_LESS 'q'
#define ICON_BATTERY_CHARGE_MORE 'r'

#define ICON_CHARGING ('z' + 3)

#define ICON_LARGE_CYCLE 'a'
#define ICON_LARGE_RUN   'b'
#define ICON_LARGE_ALARM 'c'
#define ICON_LARGE_BT    'd'
#define ICON_LARGE_NOBT  'e'
#define ICON_LARGE_WAIT1 'f'
#define ICON_LARGE_WAIT2 'g'
#define ICON_LARGET_WATCH 'h'
#define ICON_LARGET_PHONE 'i'
#define ICON_LARGE_WARNING 'j'
#define ICON_LARGE_LOWBATTERY 'k'
#define ICON_LARGE_BATTERY_LEVEL1 'l'
#define ICON_LARGE_BATTERY_LEVEL2 'm'
#define ICON_LARGE_BATTERY_LEVEL3 'n'
#define ICON_LARGE_BATTERY_LEVEL4 'o'

#endif