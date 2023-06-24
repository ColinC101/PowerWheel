/*
  ForceComputer.cpp - Force parser, intensity over 2 axis from 12-effect reports

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

#include "ForceComputer.h"


///////////////// RUNNING-EFFECTS TABLE HANDLING ////////////////

void ForceComputer::createEffect(CreateNewEffectReport_t* newEffectReport)
{
	blockLoadReport.reportId = 6;
	blockLoadReport.effectBlockIndex = getNextFreeEffect();

	if (blockLoadReport.effectBlockIndex == 0) //Effect Table is full
	{
		blockLoadReport.loadStatus = 2;
	}
	else //Effect can be added
	{
		blockLoadReport.loadStatus = 1;

		volatile Effect_t* effect = &effectTable[blockLoadReport.effectBlockIndex];

		memset((void*)effect, 0, sizeof(Effect_t));
		effect->state = 0x01; //Memory allocated successfully
		blockLoadReport.ramPoolAvailable -= EFFECT_SIZE;
	}
}

uint8_t ForceComputer::getNextFreeEffect()
{
	if (nextFreeEffect == MAX_EFFECT_NUMBER)
		return 0;

	//Allocate next free effect
	uint8_t index = nextFreeEffect;
	effectTable[index].state = 0x01;

	//Update nextFreeEffect by finding the new one
	while (effectTable[nextFreeEffect].state != 0)
	{
		if (nextFreeEffect >= MAX_EFFECT_NUMBER)
			break;
		nextFreeEffect++;
	}

	return index;
}

void ForceComputer::startEffect(uint8_t index)
{
	if (index > MAX_EFFECT_NUMBER) return;
	effectTable[index].state = 0x02; //Effect playing
	effectTable[index].elapsedTime = 0;
	effectTable[index].startTime = (uint64_t) millis();
}

void ForceComputer::stopEffect(uint8_t index)
{
	if (index > MAX_EFFECT_NUMBER) return;
	effectTable[index].state &= ~0x02; //Effect not playing
	blockLoadReport.ramPoolAvailable += EFFECT_SIZE;
}

void ForceComputer::stopAll()
{
	for (uint8_t i = 0 ; i < MAX_EFFECT_NUMBER + 1 ; i++)
		stopEffect(i);
}

void ForceComputer::freeEffect(uint8_t index)
{
	if (index > MAX_EFFECT_NUMBER) return;
	effectTable[index].state = 0;
	if (index < nextFreeEffect)
		nextFreeEffect = index; //Update nextFreeEffect
}

void ForceComputer::freeAll(void)
{
	nextFreeEffect = 1;
	memset((void*)& effectTable, 0, sizeof(effectTable));
	blockLoadReport.ramPoolAvailable = MEMORY_SIZE;
}



///////////////// FORCE REGISTERING FROM MAIN REPORT ////////////////

void ForceComputer::SetEffect(SetEffectReport_t* report) //Effect (1)
{
	volatile Effect_t* effect = &effectTable[report->effectBlockIndex];

	effect->duration = report->duration;
	effect->directionX = report->directionX;
	effect->directionY = report->directionY;
	effect->effectType = report->effectType;
	effect->gain = report->gain;
	effect->enableAxis = report->enableAxis;
}

void ForceComputer::SetEnvelope(SetEnvelopeReport_t* report, volatile Effect_t* effect) //Enveloppe (2)
{
	effect->attackLevel = report->attackLevel;
	effect->fadeLevel = report->fadeLevel;
	effect->attackTime = report->attackTime;
	effect->fadeTime = report->fadeTime;
}

void ForceComputer::SetCondition(SetConditionReport_t* report, volatile Effect_t* effect) //Condition (3)
{
	uint8_t axis = report->parameterBlockOffset;

    effect->conditions[axis].cpOffset = report->cpOffset;
    effect->conditions[axis].positiveCoefficient = report->positiveCoefficient;
    effect->conditions[axis].negativeCoefficient = report->negativeCoefficient;
    effect->conditions[axis].positiveSaturation = report->positiveSaturation;
    effect->conditions[axis].negativeSaturation = report->negativeSaturation;
    effect->conditions[axis].deadBand = report->deadBand;
	effect->conditionBlocksCount++;
}

void ForceComputer::SetPeriodic(SetPeriodicReport_t* report, volatile Effect_t* effect) //Periodic (4)
{
	effect->magnitude = report->magnitude;
	effect->offset = report->offset;
	effect->phase = report->phase;
	effect->period = report->period;
}

void ForceComputer::SetConstantForce(SetConstantForceReport_t* report, volatile Effect_t* effect) //Constant (5)
{
	effect->magnitude = report->magnitude;
}

void ForceComputer::SetRampForce(SetRampForceReport_t* report, volatile Effect_t* effect) //Ramp (6)
{
	effect->startMagnitude = report->startMagnitude;
	effect->endMagnitude = report->endMagnitude;
}

void ForceComputer::SetCustomForceReport(SetCustomForcereportReport_t* report) //Customreport (7)
{
}

void ForceComputer::SetDownloadForceSample(SetDownloadForceSampleReport_t* report) //DownloadSample (8)
{
}

void ForceComputer::EffectOperation(EffectOperationReport_t* report) //EffectOperation (10)
{
	switch (report->operation)
	{
		case 1: //Start effect
			//Looped execution
			if (report->loopCount > 0) effectTable[report->effectBlockIndex].duration *= report->loopCount;
			if (report->loopCount == 0xFF) effectTable[report->effectBlockIndex].duration = 0x7FFF;
			startEffect(report->effectBlockIndex);
			break;
		case 2: //Start with reset
			stopAll();
			startEffect(report->effectBlockIndex);
			break;
		case 3: //Stop effect
			stopEffect(report->effectBlockIndex);
			break;
		default:
			break;
	}
}

void ForceComputer::BlockFree(BlockFreeReport_t* report) //BlockFree (11)
{
	if (report->effectBlockIndex == 255) freeAll();
	else freeEffect(report->effectBlockIndex);
}

void ForceComputer::DeviceControl(DeviceControlReport_t* report) //DeviceControl (12)
{
	switch (report->control)
	{
		case 1://Enable actuators
			break;
		case 2: //Disable actuators
			break;
		case 3: //Stop effects
			stopAll();
			break;
		case 4: //Reset effects
			freeAll();
			break;
		case 5: //Pause
			devicePaused = 1;
			break;
		case 6: //Continue
			devicePaused = 0;
			break;
		default:
			break;
	}
}

void ForceComputer::DeviceGain(DeviceGainReport_t* report) //DeviceGain (13)
{
	deviceGain.gain = report->gain;
}

void ForceComputer::SetCustomForce(SetCustomForceReport_t* report) //Custom (14)
{
}



//////////////// FORCE COMPUTING ////////////////

int32_t ForceComputer::ComputeEnvelope(volatile Effect_t& effect, int32_t value)
{
	int32_t magnitude = (((int32_t) effect.magnitude) * effect.gain) / 255;
	int32_t attackLevel = (((int32_t) effect.attackLevel) * effect.gain) / 255;
	int32_t fadeLevel = (((int32_t) effect.fadeLevel) * effect.gain) / 255;
	int32_t newValue = magnitude;
	int32_t attackTime = effect.attackTime;
	int32_t fadeTime = effect.fadeTime;
	int32_t elapsedTime = effect.elapsedTime;
	int32_t duration = effect.duration;

	if (elapsedTime < attackTime)
	{
		newValue = (magnitude - attackLevel) * elapsedTime / attackTime;
		newValue += attackLevel;
	}
	if (elapsedTime > (duration - fadeTime))
	{
		newValue = (magnitude - fadeLevel) * (duration - elapsedTime);
		newValue /= fadeTime;
		newValue += fadeLevel;
	}
	newValue = newValue * value / magnitude;

	return newValue;
}

int32_t ForceComputer::ComputeConstantForce(volatile Effect_t& effect)
{
	return ComputeEnvelope(effect, (int32_t)effect.magnitude);
}

int32_t ForceComputer::ComputeRampForce(volatile Effect_t& effect)
{
	int32_t tempforce = (int32_t)(effect.startMagnitude + effect.elapsedTime * 1.0 * (effect.endMagnitude - effect.startMagnitude) / effect.duration);
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeSquareForce(volatile Effect_t& effect)
{
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t phase = effect.phase;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t period = effect.period;

	int32_t maxMagnitude = offset + magnitude;
	int32_t minMagnitude = offset - magnitude;
	uint32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	uint32_t reminder = timeTemp % period;
	int32_t tempforce;
	if (reminder > (period / 2)) tempforce = minMagnitude;
	else tempforce = maxMagnitude;
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeSinForce(volatile Effect_t& effect)
{
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t phase = effect.phase;
	uint16_t timeTemp = effect.elapsedTime;
	uint16_t period = effect.period;
	float angle = 0.0;
	if(period != 0)
		angle = ((timeTemp * 1.0 / period) * 2 * PI + (phase / 36000.0));
	float sine = sin(angle);
	int32_t tempforce = (int32_t)(sine * magnitude);
	tempforce += offset;
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeTriangleForce(volatile Effect_t& effect)
{
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = ((maxMagnitude - minMagnitude) * 2) / periodF;
	int32_t tempforce = 0;
	if (reminder > (periodF / 2)) tempforce = slope * (periodF - reminder);
	else tempforce = slope * reminder;
	tempforce += minMagnitude;
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeSawtoothDownForce(volatile Effect_t& effect)
{
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = (maxMagnitude - minMagnitude) / periodF;
	int32_t tempforce = 0;
	tempforce = slope * (period - reminder);
	tempforce += minMagnitude;
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeSawtoothUpForce(volatile Effect_t& effect)
{
	int16_t offset = effect.offset * 2;
	int16_t magnitude = effect.magnitude;
	uint16_t elapsedTime = effect.elapsedTime;
	uint16_t phase = effect.phase;
	uint16_t period = effect.period;
	uint16_t periodF = effect.period;

	int16_t maxMagnitude = offset + magnitude;
	int16_t minMagnitude = offset - magnitude;
	int32_t phasetime = (phase * period) / 255;
	uint32_t timeTemp = elapsedTime + phasetime;
	int32_t reminder = timeTemp % period;
	int32_t slope = (maxMagnitude - minMagnitude) / periodF;
	int32_t tempforce = 0;
	tempforce = slope * reminder;
	tempforce += minMagnitude;
	return ComputeEnvelope(effect, tempforce);
}

int32_t ForceComputer::ComputeConditionForce(volatile Effect_t& effect, int16_t value, int16_t maxValue, uint8_t axis)
{
	float deadBand;
	float cpOffset;
	float positiveCoefficient;
	float negativeCoefficient;
	float positiveSaturation;
	float negativeSaturation;

    deadBand = effect.conditions[axis].deadBand;
    cpOffset = effect.conditions[axis].cpOffset;
    negativeCoefficient = effect.conditions[axis].negativeCoefficient;
    negativeSaturation = effect.conditions[axis].negativeSaturation;
    positiveSaturation = effect.conditions[axis].positiveSaturation;
    positiveCoefficient = effect.conditions[axis].positiveCoefficient;

	float normalizedValue = (float)value * 1.00 / maxValue;

	float  tempForce = 0;
	if (normalizedValue < (cpOffset - deadBand))
	{
		tempForce = (normalizedValue - (float)1.00*(cpOffset - deadBand)/10000) * negativeCoefficient;
		tempForce = (tempForce < -negativeSaturation ? -negativeSaturation : tempForce);
	}
	else if (normalizedValue > (cpOffset + deadBand))
	{
		tempForce = (normalizedValue - (float)1.00 * (cpOffset + deadBand) / 10000) * positiveCoefficient;
		tempForce = (tempForce > positiveSaturation ? positiveSaturation : tempForce);
	}
	else return 0;
	tempForce = -tempForce * effect.gain / 255;

	return (int32_t)tempForce;
}



///////////////// MAIN INTERFFACING METHODS ////////////////

//Casts the report in the right format, and calls the associated command
void ForceComputer::castReport(uint8_t* report, uint16_t len)
{
	uint8_t effectId = report[1];

	switch (report[0])
	{
	case 1:
		SetEffect((SetEffectReport_t*) report);
		break;
	case 2:
		SetEnvelope((SetEnvelopeReport_t*) report, &effectTable[effectId]);
		break;
	case 3:
		SetCondition((SetConditionReport_t*) report, &effectTable[effectId]);
		break;
	case 4:
		SetPeriodic((SetPeriodicReport_t*) report, &effectTable[effectId]);
		break;
	case 5:
		SetConstantForce((SetConstantForceReport_t*) report, &effectTable[effectId]);
		break;
	case 6:
		SetRampForce((SetRampForceReport_t*) report, &effectTable[effectId]);
		break;
	case 7:
		SetCustomForceReport((SetCustomForcereportReport_t*) report);
		break;
	case 8:
		SetDownloadForceSample((SetDownloadForceSampleReport_t*) report);
		break;
	case 10:
		EffectOperation((EffectOperationReport_t*) report);
		break;
	case 11:
		BlockFree((BlockFreeReport_t*) report);
		break;
	case 12:
		DeviceControl((DeviceControlReport_t*) report);
		break;
	case 13:
		DeviceGain((DeviceGainReport_t*) report);
		break;
	case 14:
		SetCustomForce((SetCustomForceReport_t*) report);
		break;
	default:
		break;
	}
}

void ForceComputer::ComputeFinalForces(int32_t* forces) {
	forces[0] = 0;
    forces[1] = 0;
	for (int i = 0; i < MAX_EFFECT_NUMBER; i++)
	{
		if ((effectTable[i].state == 0x02) && //Effect playing
			((effectTable[i].elapsedTime <= effectTable[i].duration) ||
			(effectTable[i].duration == 0x7FFF)) &&
			!devicePaused)
		{
			for (int j = 0; j < 2; j++)
			{
				uint8_t axis;

				if (effectTable[i].conditionBlocksCount > 1) axis = j;
				else axis = 0;

				switch (effectTable[i].effectType)
				{
					case 1: //Constant
						forces[j] += ComputeConstantForce(effectTable[i]) * CONSTANT_GAIN;
						break;
					case 2: //Ramp
						forces[j] += ComputeRampForce(effectTable[i]) * RAMP_GAIN;
						break;
					case 3: //Periodic_Square
						forces[j] += ComputeSquareForce(effectTable[i]) * SQUARE_GAIN;
						break;
					case 4: //Periodic_Sine
						forces[j] += ComputeSinForce(effectTable[i]) * SINE_GAIN;
						break;
					case 5: //Periodic_Triangle
						forces[j] += ComputeTriangleForce(effectTable[i]) * TRIANGLE_GAIN;
						break;
					case 6: //Periodic_SawtoothDown
						forces[j] += ComputeSawtoothDownForce(effectTable[i]) * SAWTOOTHDOWN_GAIN;
						break;
					case 7: //Periodic_SawtoothUp
						forces[j] += ComputeSawtoothUpForce(effectTable[i]) * SAWTOOTHUP_GAIN;
						break;
					case 8: //Condition_Spring
						forces[j] += ComputeConditionForce(effectTable[i], springCurPos, SPRING_MAX_POS, axis) * SPRING_GAIN;
						break;
					case 9: //Condition_Damper
						forces[j] += ComputeConditionForce(effectTable[i], damperCurVel, DAMPER_MAX_VEL, axis) * DAMPER_GAIN;
						break;
					case 10: //Condition_Inertia
						if (inertiaCurAcc < 0 && frictionCurPos < 0) {
							forces[j] += ComputeConditionForce(effectTable[i], abs(inertiaCurAcc), INERTIA_MAX_ACC, axis) * INERTIA_GAIN;
						}
						else if (inertiaCurAcc < 0 && frictionCurPos > 0) {
							forces[j] += -1 * ComputeConditionForce(effectTable[i], abs(inertiaCurAcc), INERTIA_MAX_ACC, axis) * INERTIA_GAIN;
						}
						break;
					case 11: //Condition_Friction
						forces[j] += ComputeConditionForce(effectTable[i], frictionCurPos, FRICTION_MAX_POS, axis) * FRICTION_GAIN;
						break;
					case 12: //Custom
						break;
				}

                effectTable[i].elapsedTime = (uint64_t)millis() - effectTable[i].startTime;

			}
		}
	}
	for (int j = 0; j < 2; j++)
	{
		forces[j] = (int32_t)((float)1.0 * forces[j] * TOTAL_GAIN / 10000);
		forces[j] = map(forces[j], -10000, 10000, -255, 255);
	}
}
