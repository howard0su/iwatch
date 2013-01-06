#include "lib/sensors.h"
#include "gyro-sensor.h"

const struct sensors_sensor gyro_sensor;
static int status(int type);

static int
value(int type)
{
 return 0;
}

static int
configure(int type, int c)
{
	switch (type) {
	case SENSORS_ACTIVE:
		if (c) {
			if(!status(SENSORS_ACTIVE)) {
			}
		} else {
		}
		return 1;
	}
	return 0;
}

static int
status(int type)
{
	switch (type) {
	case SENSORS_ACTIVE:
	case SENSORS_READY:
		return 1;
	}
	return 0;
}

SENSORS_SENSOR(gyro_sensor, GYRO_SENSOR,
	       value, configure, status);