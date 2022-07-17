/*
  ForceComputer.h - Force parser, intensity over 2 axis from 12-effect reports

  Copyright (c) 2020, Colin Constans

  This library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FORCECOMPUTER_h
#define FORCECOMPUTER_h
#include <Arduino.h>

#define MAX_EFFECT_NUMBER 14
#define EFFECT_SIZE sizeof(Effect_t)
#define MEMORY_SIZE (uint16_t)(MAX_EFFECT_NUMBER*EFFECT_SIZE)

#define TOTAL_GAIN 100
#define CONSTANT_GAIN 100
#define RAMP_GAIN 100
#define SQUARE_GAIN 100
#define SINE_GAIN 100
#define TRIANGLE_GAIN 100
#define SAWTOOTHUP_GAIN 100
#define SAWTOOTHDOWN_GAIN 100
#define CUSTOM_GAIN 100

#define SPRING_GAIN 100
#define SPRING_MAX_POS 255
#define DAMPER_GAIN 100
#define DAMPER_MAX_VEL 255
#define INERTIA_GAIN 100
#define INERTIA_MAX_ACC 255
#define FRICTION_GAIN 100
#define FRICTION_MAX_POS 255


//////////////// ABSTRACT REPORTS ////////////////

typedef struct
{
	int16_t cpOffset;
	int16_t positiveCoefficient;
	int16_t negativeCoefficient;
	int16_t positiveSaturation;
	int16_t negativeSaturation;
	uint16_t deadBand;
} Condition_t;

typedef struct
{
	volatile uint8_t state;
	uint8_t effectType;
	int16_t offset;
	uint8_t gain;
	int16_t attackLevel;
	int16_t fadeLevel;
	uint16_t fadeTime;
	uint16_t attackTime;
	int16_t magnitude;
	uint8_t enableAxis;
	uint8_t directionX;
	uint8_t directionY;
	uint8_t conditionBlocksCount;
	Condition_t conditions[2];
	uint16_t phase;
	int16_t startMagnitude;
	int16_t endMagnitude;
	uint16_t period;
	uint16_t duration;
    uint16_t elapsedTime;
	uint64_t startTime;
} Effect_t;



///////////////// MEMORY HANDLING REPORTS ////////////////

typedef struct
{
	uint8_t	reportId;
	uint8_t effectBlockIndex;
	uint8_t	loadStatus;
	uint16_t ramPoolAvailable;
} BlockLoadReport_t;

typedef struct
{
	uint8_t	reportId;
	uint16_t ramPoolSize;
	uint8_t	maxSimultaneousEffects;
	uint8_t	memoryManagement;
} PoolReport_t;

typedef struct
{
	uint8_t	reportId;
	uint8_t	effectType;
	uint16_t byteCount;
} CreateNewEffectReport_t;



///////////////// EFFECT REPORTS ////////////////

typedef struct //Effect (1)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	uint8_t	effectType;
	uint16_t duration;
	uint16_t triggerRepeatInterval;
	uint16_t samplePeriod;
	uint8_t	gain;
	uint8_t	triggerButton;
	uint8_t	enableAxis;
	uint8_t	directionX;
	uint8_t	directionY;
} SetEffectReport_t;

typedef struct //Enveloppe (2)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	uint16_t attackLevel;
	uint16_t fadeLevel;
	uint32_t attackTime;
	uint32_t fadeTime;
} SetEnvelopeReport_t;

typedef struct //Condition (3)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	uint8_t	parameterBlockOffset;
	int16_t cpOffset;
	int16_t	positiveCoefficient;
	int16_t	negativeCoefficient;
	uint16_t positiveSaturation;
	uint16_t negativeSaturation;
	uint16_t deadBand;
} SetConditionReport_t;

typedef struct //Periodic (4)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	uint16_t magnitude;
	int16_t	offset;
	uint16_t phase;
	uint32_t period;
} SetPeriodicReport_t;

typedef struct //Constant (5)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	int16_t magnitude;
} SetConstantForceReport_t;

typedef struct //Ramp (6)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	int16_t startMagnitude;
	int16_t	endMagnitude;
} SetRampForceReport_t;

typedef struct //Customreport (7)
{
	uint8_t	reportId;
	uint8_t	effectBlockIndex;
	uint16_t reportOffset;
	int8_t	report[12];
} SetCustomForcereportReport_t;

typedef struct //DownloadSample (8)
{
	uint8_t	reportId;
	int8_t	x;
	int8_t	y;
} SetDownloadForceSampleReport_t;

typedef struct //EffectOperation (10)
{
	uint8_t	reportId;
	uint8_t effectBlockIndex;
	uint8_t operation;
	uint8_t	loopCount;
} EffectOperationReport_t;

typedef struct //BlockFree (11)
{
	uint8_t	reportId;
	uint8_t effectBlockIndex;
} BlockFreeReport_t;

typedef struct //DeviceControl (12)
{
	uint8_t	reportId;
	uint8_t control;
} DeviceControlReport_t;

typedef struct //DeviceGain (13)
{
	uint8_t	reportId;
	uint8_t gain;
} DeviceGainReport_t;

typedef struct //Custom (14)
{
	uint8_t	reportId;
	uint8_t effectBlockIndex;
	uint8_t	sampleCount;
	uint16_t samplePeriod;
} SetCustomForceReport_t;



//////////////// MAIN CLASS ///////////////

class ForceComputer
{
public:

	volatile uint8_t nextFreeEffect = 1; //Id of empty effect slot
	volatile Effect_t effectTable[MAX_EFFECT_NUMBER + 1]; //Running-effects storage

	//Memory/Device handling
	volatile uint8_t devicePaused = 0;
	volatile BlockLoadReport_t blockLoadReport;
	volatile PoolReport_t poolReport;
	void createEffect(CreateNewEffectReport_t* newEffectReport);

	//Condition force param
	int16_t springCurPos = 100;
	int16_t damperCurVel = 100;
	int16_t inertiaCurAcc = 100;
	int16_t frictionCurPos = 100;

	//Interfacing methods
	void castReport(uint8_t* report, uint16_t len);
	void ComputeFinalForces(int32_t* forces);

private:

	//Running-effects table handling
	uint8_t getNextFreeEffect();
	void startEffect(uint8_t index);
	void stopEffect(uint8_t index);
	void stopAll();
	void freeEffect(uint8_t index);
	void freeAll();

	//Memory/Device handling
	volatile DeviceGainReport_t deviceGain;
	void EffectOperation(EffectOperationReport_t* report);
	void BlockFree(BlockFreeReport_t* report);
	void DeviceControl(DeviceControlReport_t* report);
	void DeviceGain(DeviceGainReport_t* report);

	//Forces registering
	void SetEffect(SetEffectReport_t* report);
	void SetEnvelope(SetEnvelopeReport_t* report, volatile Effect_t* effect);
	void SetConstantForce(SetConstantForceReport_t* report, volatile Effect_t* effect);
	void SetRampForce(SetRampForceReport_t* report, volatile Effect_t* effect);
	void SetPeriodic(SetPeriodicReport_t* report, volatile Effect_t* effect);
	void SetCondition(SetConditionReport_t* report, volatile Effect_t* effect);
	void SetCustomForce(SetCustomForceReport_t* report);
	void SetCustomForceReport(SetCustomForcereportReport_t* report);
	void SetDownloadForceSample(SetDownloadForceSampleReport_t* report);

	//Forces computing
	int32_t ComputeEnvelope(volatile Effect_t& effect, int32_t value);
	int32_t ComputeConstantForce(volatile Effect_t& effect);
	int32_t ComputeRampForce(volatile Effect_t& effect);
	int32_t ComputeSquareForce(volatile Effect_t& effect);
	int32_t ComputeSinForce(volatile Effect_t& effect);
	int32_t ComputeTriangleForce(volatile Effect_t& effect);
	int32_t ComputeSawtoothDownForce(volatile Effect_t& effect);
	int32_t ComputeSawtoothUpForce(volatile Effect_t& effect);
	int32_t ComputeConditionForce(volatile Effect_t& effect, int16_t value, int16_t maxValue, uint8_t axis);
};

#endif
