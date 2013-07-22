/*
 * Algorithm is uWave coming from 
 *
 */
#include "contiki.h"
#include <string.h>
#include <assert.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

// must be 2^n. 1, 2, 4, 8, 16
#define MOVE_WINDOW 8
#define MOVE_STEP 4

static int16_t data[MOVE_WINDOW][3];
static uint8_t datap = 0;
static int32_t currentsum[3];

#define NUM_GESTURES 3
#define MAX_GESTURES 25

static const int8_t Gesture1[] = {0,0,16, 0,0,15, 0,0,15, -1,0,10, -2,2,10, -2,2,10, -4,1,10, -3,1,10, 0,2,10, 6,5,16, 10,8,16, 10,10,16, 10,9,10, 10,9,10
, 7,10,8, 6,10,8, 5,10,10, -2,10,10, -9,10,16, -9,10,16, -6,10,16, -4,10,16, -3,10,16};
static const int8_t Gesture2[] = {5,1,10, 3,0,10, 1,0,15, 0,0,15, -1,-1,16, -2,-1,16, -4,-1,16, -7,-2,16, -10,-2,16, -10,0,16, -10,0,15, -10,1,16, -7,3,15
, 4,9,10, 15,10,10, 16,10,10, 16,6,10, 10,1,10, 9,0,10, 7,0,10, 6,0,10, 5,-1,10};
static const int8_t Gesture3[] = {0, 0, 0};
static const int8_t *GestureData[NUM_GESTURES] =
{Gesture1, Gesture2, Gesture3};
static const int8_t GestureDataLength[NUM_GESTURES] =
{sizeof(Gesture1)/3, sizeof(Gesture2)/3, sizeof(Gesture3)/3};

static uint16_t distances[NUM_GESTURES][MAX_GESTURES];
static uint16_t count;
static uint8_t recoding;

void gesture_init(int8_t _recording)
{
	// init gesture structure
	memset(distances, 0, sizeof(distances));
	memset(data, 0, sizeof(data));
	memset(currentsum, 0, sizeof(currentsum));
	datap = 0;
	count = 0;
	recoding = _recording;
}

static uint16_t Dist(const int8_t* p1, const int8_t *p2)
{
	uint16_t t = 
	      (p1[0] - p2[0]) * (p1[0] - p2[0])
		+ (p1[1] - p2[1]) * (p1[1] - p2[1]) 
		+ (p1[2] - p2[2]) * (p1[2] - p2[2]);

	return t;
}

#define SCALE_2G 20
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
		ret = value / SCALE_1G * 10;
	}
	else
	{
		ret = 10 + (value - SCALE_1G) / (SCALE_1G) * 5;
	}

	if (data > 0)
		return ret;
	else
		return -ret;
}

static void gesture_caculate(int index, const int8_t* point)
{
	const int8_t *gestureData = GestureData[index];
	int8_t gestureLength = GestureDataLength[index];
	uint16_t *distance = distances[index];
	// caculate with template
	uint16_t lastvalue = Dist(&gestureData[0], point);

	if (count == 0)
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
	if (count > gestureLength)
	{
		uint16_t weight = lastvalue / (gestureLength + count);
		if (weight < 10)
		{
			PRINTF("Match Gesture %d\n", index);
			gesture_init();
		}
	}

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
}

void gesture_processdata(int16_t *input)
{
	int16_t current[3];
	int8_t result[3];
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
		if (recoding)
		{
			printf("%d,%d,%d ", result[0], result[1], result[2]);
		}
		else
			for(int k = 0; k < 2; k++)
				gesture_caculate(k, result);
		count++;
	}
}
