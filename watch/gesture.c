/*
 * Algorithm is uWave coming from 
 *
 */
#include "contiki.h"
#include "window.h"
#include <string.h>
#include <assert.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define MAX_DATAPOINTS 100
// must be 2^n. 1, 2, 4, 8, 16
#define MOVE_WINDOW 4
#define MOVE_STEP 2

static int16_t data[MOVE_WINDOW][3];
static uint8_t datap = 0;
static int32_t currentsum[3];

#define NUM_GESTURES 6
#define MAX_GESTURES 25

static const int8_t Gesture1[] = {
0,0,16, 0,0,15, 0,0,15, -1,0,10, -2,2,10, -2,2,10, -4,1,10, -3,1,10, 0,2,10, 6,5,16, 
10,8,16, 10,10,16, 10,9,10, 10,9,10, 7,10,8, 6,10,8, 5,10,10, -2,10,10, -9,10,16, 
-9,10,16, -6,10,16, -4,10,16, -3,10,16};
static const int8_t Gesture2[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t Gesture3[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 5,10,10, -2,10,10, -9,10,16, -9,10,16, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t Gesture4[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 5,10,10, -2,10,10, -9,10,16, -9,10,16, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t Gesture5[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 5,10,10, -2,10,10, -9,10,16, -9,10,16, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t Gesture6[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 5,10,10, -2,10,10, -9,10,16, -9,10,16, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t *GestureData[NUM_GESTURES] =
{Gesture1, Gesture2, Gesture3, Gesture4, Gesture5, Gesture6};
static const int8_t GestureDataLength[NUM_GESTURES] =
{
	sizeof(Gesture1)/3, sizeof(Gesture2)/3, sizeof(Gesture3)/3,
	sizeof(Gesture4)/3, sizeof(Gesture5)/3, sizeof(Gesture6)/3
};

static uint16_t distances[NUM_GESTURES][MAX_GESTURES];
static uint16_t count;
static enum{STATE_NONE, STATE_RECORDING, STATE_RECON} state;

void gesture_init(int8_t _recording)
{
	// init gesture structure
	memset(distances, 0, sizeof(distances));
	memset(data, 0, sizeof(data));
	memset(currentsum, 0, sizeof(currentsum));
	datap = 0;
	count = 0;
	if (_recording)
		state = STATE_RECORDING;
	else
		state = STATE_RECON;
}

static uint16_t Dist(const int8_t* p1, const int8_t *p2)
{
	uint16_t t = 
	      (p1[0] - p2[0]) * (p1[0] - p2[0])
		+ (p1[1] - p2[1]) * (p1[1] - p2[1]) 
		+ (p1[2] - p2[2]) * (p1[2] - p2[2]);

	return t;
}

#define SCALE_2G 128 //16384
#define SCALE_1G (SCALE_2G/2)

static int8_t Normalize(int16_t data)
{
	int16_t value;
	int8_t ret;
	if (data == 0)
		return 0;
	else if (data > 0) 
		value = data;
	else
		value = -data;

	if (value > SCALE_2G)
	{
		ret = 20;
	}
	else if (value < SCALE_1G)
	{
		ret = value * 10 / SCALE_1G;
	}
	else
	{
		ret = 10 + (value - SCALE_1G) * 5/ (SCALE_1G);
	}

	if (data > 0)
		return ret;
	else
		return -ret;
}

extern int spp_send(char* buf, int count);
static uint16_t gesture_caculate(int index, const int8_t* point)
{
	const int8_t *gestureData = GestureData[index];
	int8_t gestureLength = GestureDataLength[index];
	uint16_t *distance = distances[index];
	// caculate with template
	uint16_t lastvalue = Dist(&gestureData[0], point);

	if (count == MOVE_WINDOW)
	{
		distance[0] = lastvalue;
		for(int tindex = 1; tindex < gestureLength; tindex++)
		{
			distance[tindex] = distance[tindex - 1] + Dist(&gestureData[tindex * 3], point);
		}
	}
	else
	{
		if (lastvalue > distance[0])
			lastvalue = distance[0];
		for(int tindex = 1; tindex < gestureLength; tindex++)
		{
			uint16_t local = Dist(&gestureData[tindex * 3], point);
			uint16_t min = lastvalue;
			if (min > distance[tindex])
				min = distance[tindex];
			if (min > distance[tindex - 1])
				min = distance[tindex - 1];
			if (min > lastvalue)
				min = lastvalue;
			distance[tindex - 1] = lastvalue;
			lastvalue = local + min;
		}
		distance[gestureLength - 1] = lastvalue;
	}
#if 0
	for(int i = 0; i < gestureLength; i++)
	{
		PRINTF("%d ",distance[i]);
	}
	PRINTF("\n  ");
	for(int i = 1; i < gestureLength; i++)
	{
		PRINTF("%d ",distance[i] - distance[i-1]);
	}
	PRINTF("\n");
#endif
	return distance[gestureLength - 1]/(gestureLength + count/MOVE_STEP - 1);
}

void gesture_processdata(int16_t *input)
{
	int16_t current[3];
	int8_t result[3];

	if (state == STATE_NONE)
		return;

	if (state == STATE_RECON && count > MAX_DATAPOINTS)
	{
		PRINTF("No MATCH\n");
		state = STATE_NONE;
		process_post(ui_process, EVENT_GESTURE_MATCHED, (void*)0);
		return;
	}

	//printf("%d %d %d\n", input[0], input[1], input[2]);

	count++;

	// integrate into move window average
	// get rid of oldest
	for(int i = 2; i >=0; i--)
	{
		currentsum[i] -= data[datap][i];
		currentsum[i] += input[i];
		data[datap][i] = input[i];

		if ((datap & (MOVE_STEP - 1)) != (MOVE_STEP -1))
			continue;

		//		if (count < MOVE_WINDOW)
		//			continue;

		current[i] = currentsum[i] / MOVE_WINDOW;
		result[i] = Normalize(current[i]);
	}  

	datap++;
	datap &= (MOVE_WINDOW - 1);
	if ((datap % MOVE_STEP == 0) && (count >=  MOVE_WINDOW))
	{
		if (state == STATE_RECORDING)
		{
			char buf[30];
			int length = sprintf(buf, "%d %d %d\n", result[0], result[1], result[2]);
			//spp_send(buf, length);
			printf(buf);
			if (count > MAX_DATAPOINTS)
			{
				printf("===\n");
				state = STATE_NONE;
			}
		}
		else
		{
			PRINTF("%d %d %d\n", result[0], result[1], result[2]);
			uint16_t shortestDistance = 0xffff;
  			uint16_t longestDistance = 0;
  			uint8_t bestMatch;
  			uint32_t totalDistance = 0;
			for(int k = 0; k < 2; k++)
			{
				uint16_t distance = gesture_caculate(k, result);

			    if (distance < shortestDistance) {
			      shortestDistance = distance;
			      bestMatch = k;
			    }

			    if (distance > longestDistance) {
			      longestDistance = distance;
			    }
 				totalDistance += distance;
			}

			uint16_t averageDistance = totalDistance/NUM_GESTURES;
			PRINTF("averageDistance: %d, shortestDistance: %d, longestDistance: %d, variance: %d\n", 
				averageDistance, shortestDistance, longestDistance, longestDistance - shortestDistance);

			if (shortestDistance > averageDistance / 2)
			{
				PRINTF("almost matched %d\n", bestMatch+1);
				return;
			}
			// matched
			PRINTF("Matched %d\n", bestMatch+1);
			process_post(ui_process, EVENT_GESTURE_MATCHED, (void*)(bestMatch + 1));
			state = STATE_NONE;
			return;
		}
	}
}
