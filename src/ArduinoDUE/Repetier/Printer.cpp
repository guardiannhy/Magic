/*
    This file is part of Repetier-Firmware.

    Repetier-Firmware is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Repetier-Firmware is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Repetier-Firmware.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Repetier.h"
#if BED_LEDS
#include "Lighting.h"
#endif

long Printer::PrinterId = 0;
uint8_t Printer::probeType;
uint16_t Printer::bedType;
#if USE_ADVANCE
ufast8_t Printer::maxExtruderSpeed;            ///< Timer delay for end extruder speed
volatile int Printer::extruderStepsNeeded; ///< This many extruder steps are still needed, <0 = reverse steps needed.
//uint8_t Printer::extruderAccelerateDelay;     ///< delay between 2 speec increases
#endif
uint8_t Printer::unitIsInches = 0; ///< 0 = Units are mm, 1 = units are inches.
//Stepper Movement Variables
float Printer::axisStepsPerMM[E_AXIS_ARRAY] = {XAXIS_STEPS_PER_MM, YAXIS_STEPS_PER_MM, ZAXIS_STEPS_PER_MM, 1}; ///< Number of steps per mm needed.
float Printer::invAxisStepsPerMM[E_AXIS_ARRAY]; ///< Inverse of axisStepsPerMM for faster conversion
float Printer::maxFeedrate[E_AXIS_ARRAY] = {MAX_FEEDRATE_X, MAX_FEEDRATE_Y, MAX_FEEDRATE_Z}; ///< Maximum allowed feedrate.
float Printer::homingFeedrate[Z_AXIS_ARRAY] = {HOMING_FEEDRATE_X, HOMING_FEEDRATE_Y, HOMING_FEEDRATE_Z};
#if RAMP_ACCELERATION
//  float max_start_speed_units_per_second[E_AXIS_ARRAY] = MAX_START_SPEED_UNITS_PER_SECOND; ///< Speed we can use, without acceleration.
float Printer::maxAccelerationMMPerSquareSecond[E_AXIS_ARRAY] = {MAX_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z and E max acceleration in mm/s^2 for printing moves or retracts
float Printer::maxTravelAccelerationMMPerSquareSecond[E_AXIS_ARRAY] = {MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_X,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Y,MAX_TRAVEL_ACCELERATION_UNITS_PER_SQ_SECOND_Z}; ///< X, Y, Z max acceleration in mm/s^2 for travel moves
/** Acceleration in steps/s^3 in printing mode.*/
unsigned long Printer::maxPrintAccelerationStepsPerSquareSecond[E_AXIS_ARRAY];
/** Acceleration in steps/s^2 in movement mode.*/
unsigned long Printer::maxTravelAccelerationStepsPerSquareSecond[E_AXIS_ARRAY];
#endif
long Printer::currentDeltaPositionSteps[E_TOWER_ARRAY];
uint8_t lastMoveID = 0; // Last move ID
uint8_t Printer::relativeCoordinateMode = false;  ///< Determines absolute (false) or relative Coordinates (true).
uint8_t Printer::relativeExtruderCoordinateMode = false;  ///< Determines Absolute or Relative E Codes while in Absolute Coordinates mode. E is always relative in Relative Coordinates mode.

bool Printer::retDefAxisDir[Z_AXIS_ARRAY];
long Printer::currentPositionSteps[E_AXIS_ARRAY];
float Printer::currentPosition[Z_AXIS_ARRAY];
float Printer::lastProbeActHeight = 0.0;
float Printer::lastCmdPos[Z_AXIS_ARRAY];
long Printer::destinationSteps[E_AXIS_ARRAY];
float Printer::coordinateOffset[Z_AXIS_ARRAY] = {0,0,0};
uint8_t Printer::flag0 = 0;
uint8_t Printer::flag1 = 0;
uint8_t Printer::flag2 = 0;
uint8_t Printer::flag3 = 0;
uint8_t Printer::debugLevel = 6; ///< Bitfield defining debug output. 1 = echo, 2 = info, 4 = error, 8 = dry run., 16 = Only communication, 32 = No moves
fast8_t Printer::stepsPerTimerCall = 1;
uint8_t Printer::mode = DEFAULT_PRINTER_MODE;
uint8_t Printer::fanSpeed = 0; // Last fan speed set with M106/M107
float Printer::extrudeMultiplyError = 0;
float Printer::extrusionFactor = 1.0;
float oldFeedrate = Printer::feedrate;
uint8_t Printer::ledVal = 0;
bool Printer::allowBelow = true;

#if EEPROM_MODE != 0
float Printer::zBedOffset = HAL::eprGetFloat(EPR_Z_PROBE_Z_OFFSET);
#else
float Printer::zBedOffset = Z_PROBE_Z_OFFSET;
#endif
#if FEATURE_AUTOLEVEL
float Printer::autolevelTransformation[9]; ///< Transformation matrix
#endif
uint32_t Printer::interval = 30000;           ///< Last step duration in ticks.
uint32_t Printer::timer;              ///< used for acceleration/deceleration timing
uint32_t Printer::stepNumber;         ///< Step number in current move.
#if USE_ADVANCE
#if ENABLE_QUADRATIC_ADVANCE
int32_t Printer::advanceExecuted;             ///< Executed advance steps
#endif
int Printer::advanceStepsSet;
#endif
int32_t Printer::maxDeltaPositionSteps;
floatLong Printer::deltaDiagonalStepsSquaredA;
floatLong Printer::deltaDiagonalStepsSquaredB;
floatLong Printer::deltaDiagonalStepsSquaredC;
float Printer::deltaMaxRadiusSquared;
float Printer::radius0;
int32_t Printer::deltaFloorSafetyMarginSteps = 0;
int32_t Printer::deltaAPosXSteps;
int32_t Printer::deltaAPosYSteps;
int32_t Printer::deltaBPosXSteps;
int32_t Printer::deltaBPosYSteps;
int32_t Printer::deltaCPosXSteps;
int32_t Printer::deltaCPosYSteps;
int32_t Printer::realDeltaPositionSteps[TOWER_ARRAY];
int16_t Printer::travelMovesPerSecond;
int16_t Printer::printMovesPerSecond;
int32_t Printer::stepsRemainingAtZHit;
int32_t Printer::stepsRemainingAtXHit;
int32_t Printer::stepsRemainingAtYHit;
#if SOFTWARE_LEVELING
int32_t Printer::levelingP1[3];
int32_t Printer::levelingP2[3];
int32_t Printer::levelingP3[3];
#endif
int16_t Printer::resends = 0;
float Printer::minimumSpeed;               ///< lowest allowed speed to keep integration error small
float Printer::minimumZSpeed;
int32_t Printer::xMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::yMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::zMaxSteps;                   ///< For software endstops, limit of move in positive direction.
int32_t Printer::xMinSteps;                   ///< For software endstops, limit of move in negative direction.
int32_t Printer::yMinSteps;                   ///< For software endstops, limit of move in negative direction.
int32_t Printer::zMinSteps;                   ///< For software endstops, limit of move in negative direction.
float Printer::xLength;
float Printer::xMin;
float Printer::yLength;
float Printer::yMin;
float Printer::zLength;
float Printer::zMin;
float Printer::feedrate;                   ///< Last requested feedrate.
int Printer::feedrateMultiply;             ///< Multiplier for feedrate in percent (factor 1 = 100)
unsigned int Printer::extrudeMultiply;     ///< Flow multiplier in percdent (factor 1 = 100)
float Printer::maxJerk;                    ///< Maximum allowed jerk in mm/s
float Printer::offsetX;                     ///< X-offset for different extruder positions.
float Printer::offsetY;                     ///< Y-offset for different extruder positions.
float Printer::offsetZ;                     ///< Z-offset for different extruder positions.
speed_t Printer::vMaxReached;               ///< Maximumu reached speed
uint32_t Printer::msecondsPrinting;         ///< Milliseconds of printing time (means time with heated extruder)
float Printer::filamentPrinted;             ///< mm of filament printed since counting started
float Printer::memoryX;
float Printer::memoryY;
float Printer::memoryZ;
float Printer::memoryE;
float Printer::memoryF = -1;

#ifdef DEBUG_SEGMENT_LENGTH
float Printer::maxRealSegmentLength = 0;
#endif
#ifdef DEBUG_REAL_JERK
float Printer::maxRealJerk = 0;
#endif

volatile bool Endstops::xMaxHit;
volatile bool Endstops::yMaxHit;
volatile bool Endstops::zMaxHit;
volatile bool Endstops::probeHit;

void Endstops::xMaxRead(void)
{
    xMaxHit = (READ(X_MAX_PIN) != ENDSTOP_X_MAX_INVERTING) ? true : false;
}
void Endstops::yMaxRead(void)
{
    yMaxHit = (READ(Y_MAX_PIN) != ENDSTOP_Y_MAX_INVERTING) ? true : false;
}
void Endstops::zMaxRead(void)
{
    zMaxHit = (READ(Z_MAX_PIN) != ENDSTOP_Z_MAX_INVERTING) ? true : false;
}
void Endstops::probeRead(void)
{
    if (Printer::probeType == 2) probeHit = READ(Z_PROBE_PIN);
    else probeHit = !READ(Z_PROBE_PIN);
}


void Endstops::init(void)
{
#if !(X_MAX_PIN > -1 && Y_MAX_PIN > -1 && Z_MAX_PIN>-1 && Z_PROBE_PIN > -1)
#error Endstop pins missing
#endif /* Pin check */

    SET_INPUT(X_MAX_PIN);
#if ENDSTOP_PULLUP_X_MAX
    PULLUP(X_MAX_PIN, HIGH);
#endif

    SET_INPUT(Y_MAX_PIN);
#if ENDSTOP_PULLUP_Y_MAX
    PULLUP(Y_MAX_PIN, HIGH);
#endif

    SET_INPUT(Z_MAX_PIN);
#if ENDSTOP_PULLUP_Z_MAX
    PULLUP(Z_MAX_PIN, HIGH);
#endif

    SET_INPUT(Z_PROBE_PIN);
#if Z_PROBE_PULLUP
    PULLUP(Z_PROBE_PIN, HIGH);
#endif
    /* Get initial values*/
    xMaxRead();
    yMaxRead();
    zMaxRead();
    probeRead();
    /* Setup intterrupts */
    attachInterrupt(X_MAX_PIN, xMaxRead, CHANGE);
    attachInterrupt(Y_MAX_PIN, yMaxRead, CHANGE);
    attachInterrupt(Z_MAX_PIN, zMaxRead, CHANGE);
    attachInterrupt(Z_PROBE_PIN, probeRead, CHANGE);
}

void Endstops::report(void)
{
    Com::printF("endstops hit: ");
    Com::printF(Com::tXMaxColon);
    Com::printF(xMaxHit ? Com::tHSpace : Com::tLSpace);
    Com::printF(Com::tYMaxColon);
    Com::printF(yMaxHit ? Com::tHSpace : Com::tLSpace);
    Com::printF(Com::tZMaxColon);
    Com::printF(zMaxHit ? Com::tHSpace : Com::tLSpace);
    Com::printF(Com::tZProbeState);
    Com::printF(probeHit ? Com::tHSpace : Com::tLSpace);
    Com::println();
}

void Printer::setDebugLevel(uint8_t newLevel) {
	debugLevel = newLevel;
	Com::printFLN("DebugLevel:",(int)newLevel);
}

bool Printer::isPositionAllowed(float x,float y,float z)
{
    if(isNoDestinationCheck())  return true;
    bool allowed = true;
    allowed &= (z >= 0) && (z <= zLength + 0.05);
    allowed &= (x * x + y * y <= deltaMaxRadiusSquared);
    if(!allowed)
    {
        Printer::updateCurrentPosition(true);
        Commands::printCurrentPosition("isPositionAllowed ");
    }
    return allowed;
}

void Printer::setFanSpeedDirectly(uint8_t speed) {
#if FAN_PIN>-1
    if(pwm_pos[pwm_fan1] == speed)
        return;
#if FAN_KICKSTART_TIME
    if(fanKickstart == 0 && speed > pwm_pos[pwm_fan1] && speed < 85)
    {
         if(pwm_pos[pwm_fan1]) fanKickstart = FAN_KICKSTART_TIME*390 / 100;
         else                          fanKickstart = FAN_KICKSTART_TIME*390 / 25;
    }
#endif
    pwm_pos[pwm_fan1] = speed;
#endif
}

void Printer::setFan2SpeedDirectly(uint8_t speed) {
	#if FAN2_PIN>-1
	if(pwm_pos[pwm_fan2] == speed)
		return;
	#if FAN_KICKSTART_TIME
	if(fan2Kickstart == 0 && speed > pwm_pos[pwm_fan2] && speed < 85)
	{
		if(pwm_pos[pwm_fan2]) fan2Kickstart = FAN_KICKSTART_TIME*390 / 100;
		else                  fan2Kickstart = FAN_KICKSTART_TIME*390 / 25;
	}
	#endif
	pwm_pos[pwm_fan2] = speed;
#endif
}
void Printer::setFan3SpeedDirectly(uint8_t speed) {
	#if FAN3_PIN>-1
	if(pwm_pos[pwm_fan3] == speed)
	return;
	#if FAN_KICKSTART_TIME
	if(fan3Kickstart == 0 && speed > pwm_pos[pwm_fan3] && speed < 85)
	{
		if(pwm_pos[pwm_fan3]) fan3Kickstart = FAN_KICKSTART_TIME*390 / 100;
		else                  fan3Kickstart = FAN_KICKSTART_TIME*390 / 25;
	}
	#endif
	pwm_pos[pwm_fan3] = speed;
	#endif
}

void Printer::reportPrinterMode() {
    switch(Printer::mode) {
    case PRINTER_MODE_FFF:
        Com::printFLN(Com::tPrinterModeFFF);
        break;
    case PRINTER_MODE_LASER:
        Com::printFLN(Com::tPrinterModeLaser);
        break;
    case PRINTER_MODE_CNC:
        Com::printFLN(Com::tPrinterModeCNC);
        break;
    }
}
void Printer::updateDerivedParameter()
{
    travelMovesPerSecond = EEPROM::deltaSegmentsPerSecondMove();
    printMovesPerSecond = EEPROM::deltaSegmentsPerSecondPrint();
    axisStepsPerMM[X_AXIS] = axisStepsPerMM[Y_AXIS] = axisStepsPerMM[Z_AXIS];
    maxAccelerationMMPerSquareSecond[X_AXIS] = maxAccelerationMMPerSquareSecond[Y_AXIS] = maxAccelerationMMPerSquareSecond[Z_AXIS];
    homingFeedrate[X_AXIS] = homingFeedrate[Y_AXIS] = homingFeedrate[Z_AXIS];
    maxFeedrate[X_AXIS] = maxFeedrate[Y_AXIS] = maxFeedrate[Z_AXIS];
    maxTravelAccelerationMMPerSquareSecond[X_AXIS] = maxTravelAccelerationMMPerSquareSecond[Y_AXIS] = maxTravelAccelerationMMPerSquareSecond[Z_AXIS];
    zMaxSteps = axisStepsPerMM[Z_AXIS] * (zLength);
    xMinSteps = axisStepsPerMM[A_TOWER] * xMin;
    yMinSteps = axisStepsPerMM[B_TOWER] * yMin;
    zMinSteps = axisStepsPerMM[C_TOWER] * zMin;
    //radius0 = EEPROM::deltaHorizontalRadius();
    float radiusA = radius0 + EEPROM::deltaRadiusCorrectionA();
    float radiusB = radius0 + EEPROM::deltaRadiusCorrectionB();
    float radiusC = radius0 + EEPROM::deltaRadiusCorrectionC();
    deltaAPosXSteps = floor(radiusA * cos(EEPROM::deltaAlphaA() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaAPosYSteps = floor(radiusA * sin(EEPROM::deltaAlphaA() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaBPosXSteps = floor(radiusB * cos(EEPROM::deltaAlphaB() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaBPosYSteps = floor(radiusB * sin(EEPROM::deltaAlphaB() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaCPosXSteps = floor(radiusC * cos(EEPROM::deltaAlphaC() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaCPosYSteps = floor(radiusC * sin(EEPROM::deltaAlphaC() * M_PI / 180.0f) * axisStepsPerMM[Z_AXIS] + 0.5f);
    deltaDiagonalStepsSquaredA.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionA() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    deltaDiagonalStepsSquaredB.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionB() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    deltaDiagonalStepsSquaredC.l = static_cast<uint32_t>((EEPROM::deltaDiagonalCorrectionC() + EEPROM::deltaDiagonalRodLength())*axisStepsPerMM[Z_AXIS]);
    if(deltaDiagonalStepsSquaredA.l > 65534 || 2 * radius0*axisStepsPerMM[Z_AXIS] > 65534)
    {
        setLargeMachine(true);
        deltaDiagonalStepsSquaredA.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredA.l));
        deltaDiagonalStepsSquaredB.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredB.l));
        deltaDiagonalStepsSquaredC.L = RMath::sqr(static_cast<uint64_t>(deltaDiagonalStepsSquaredC.l));
    }
    else
    {
        setLargeMachine(false);
        deltaDiagonalStepsSquaredA.l = RMath::sqr(deltaDiagonalStepsSquaredA.l);
        deltaDiagonalStepsSquaredB.l = RMath::sqr(deltaDiagonalStepsSquaredB.l);
        deltaDiagonalStepsSquaredC.l = RMath::sqr(deltaDiagonalStepsSquaredC.l);
    }
    deltaMaxRadiusSquared = RMath::sqr(EEPROM::deltaMaxRadius());
    long cart[Z_AXIS_ARRAY], delta[TOWER_ARRAY];
    cart[X_AXIS] = cart[Y_AXIS] = 0;
    cart[Z_AXIS] = zMaxSteps;
    transformCartesianStepsToDeltaSteps(cart, delta);
    maxDeltaPositionSteps = delta[0];
    xMaxSteps = yMaxSteps = zMaxSteps;
    xMinSteps = yMinSteps = zMinSteps = 0;
    deltaFloorSafetyMarginSteps = DELTA_FLOOR_SAFETY_MARGIN_MM * axisStepsPerMM[Z_AXIS];
    for(uint8_t i = 0; i < E_AXIS_ARRAY; i++)
    {
        invAxisStepsPerMM[i] = 1.0f/axisStepsPerMM[i];
#if RAMP_ACCELERATION
        /** Acceleration in steps/s^3 in printing mode.*/
        maxPrintAccelerationStepsPerSquareSecond[i] = maxAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
        /** Acceleration in steps/s^2 in movement mode.*/
        maxTravelAccelerationStepsPerSquareSecond[i] = maxTravelAccelerationMMPerSquareSecond[i] * axisStepsPerMM[i];
#endif
    }
    float accel = RMath::max(maxAccelerationMMPerSquareSecond[X_AXIS], maxTravelAccelerationMMPerSquareSecond[X_AXIS]);
    minimumSpeed = accel * sqrt(2.0f / (axisStepsPerMM[X_AXIS]*accel));
    accel = RMath::max(maxAccelerationMMPerSquareSecond[Z_AXIS], maxTravelAccelerationMMPerSquareSecond[Z_AXIS]);
    minimumZSpeed = accel * sqrt(2.0f / (axisStepsPerMM[Z_AXIS] * accel));
#if DISTORTION_CORRECTION
    distortion.updateDerived();
#endif // DISTORTION_CORRECTION
    Printer::updateAdvanceFlags();
}
/**
  \brief Stop heater and stepper motors. Disable power,if possible.
*/
void Printer::kill(uint8_t only_steppers)
{
    if(areAllSteppersDisabled() && only_steppers) return;
    if(Printer::isAllKilled()) return;
    setAllSteppersDiabled();
#if defined(NUM_MOTOR_DRIVERS) && NUM_MOTOR_DRIVERS > 0
    disableAllMotorDrivers();
#endif // defined
    disableXStepper();
    disableYStepper();
#if !defined(PREVENT_Z_DISABLE_ON_STEPPER_TIMEOUT)
    disableZStepper();
#else
    if(!only_steppers)
        disableZStepper();
#endif
    Extruder::disableAllExtruderMotors();
    if(!only_steppers)
    {
        for(uint8_t i = 0; i < NUM_TEMPERATURE_LOOPS; i++)
            Extruder::setTemperatureForExtruder(0, i);
        Extruder::setHeatedBedTemperature(0);
        Printer::setAllKilled(true);
    }
#if BED_LEDS
	if (Printer::ledVal > 1) Light.ShowTemps();
#endif
#if FAN_BOARD_PIN>-1
#if HAVE_HEATED_BED
    if(heatedBedController.targetTemperatureC < 15)      // turn off FAN_BOARD only if bed heater is off
#endif
       pwm_pos[PWM_BOARD_FAN] = 0;
#endif // FAN_BOARD_PIN
}

void Printer::updateAdvanceFlags()
{
    Printer::setAdvanceActivated(false);
#if USE_ADVANCE
    for(uint8_t i = 0; i < NUM_EXTRUDER; i++)
    {
        if(extruder[i].advanceL != 0)
        {
            Printer::setAdvanceActivated(true);
        }
#if ENABLE_QUADRATIC_ADVANCE
        if(extruder[i].advanceK != 0) Printer::setAdvanceActivated(true);
#endif
    }
#endif
}

// This is for untransformed move to coordinates in printers absolute cartesian space
uint8_t Printer::moveTo(float x,float y,float z,float e,float f)
{
    if(x != IGNORE_COORDINATE)
        destinationSteps[X_AXIS] = (x + Printer::offsetX) * axisStepsPerMM[X_AXIS];
    if(y != IGNORE_COORDINATE)
        destinationSteps[Y_AXIS] = (y + Printer::offsetY) * axisStepsPerMM[Y_AXIS];
    if(z != IGNORE_COORDINATE)
        destinationSteps[Z_AXIS] = (z + Printer::offsetZ) * axisStepsPerMM[Z_AXIS];
    if(e != IGNORE_COORDINATE)
        destinationSteps[E_AXIS] = e * axisStepsPerMM[E_AXIS];
    if(f != IGNORE_COORDINATE)
        feedrate = f;
    // Disable software endstop or we get wrong distances when length < real length
    if (!PrintLine::queueDeltaMove(ALWAYS_CHECK_ENDSTOPS, true, false))
    {
        Com::printWarningFLN("moveTo / queueDeltaMove returns error");
        return 0;
    }
    updateCurrentPosition(false);
    return 1;
}

// Move to transformed cartesian coordinates, mapping real (model) space to printer space
uint8_t Printer::moveToReal(float x, float y, float z, float e, float f,bool pathOptimize)
{
    if(x == IGNORE_COORDINATE)
        x = currentPosition[X_AXIS];
    else
        currentPosition[X_AXIS] = x;
    if(y == IGNORE_COORDINATE)
        y = currentPosition[Y_AXIS];
    else
        currentPosition[Y_AXIS] = y;
    if(z == IGNORE_COORDINATE)
        z = currentPosition[Z_AXIS];
    else
        currentPosition[Z_AXIS] = z;
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
        transformToPrinter(x + Printer::offsetX, y + Printer::offsetY, z + Printer::offsetZ, x, y, z);
    else
#endif // FEATURE_AUTOLEVEL
    {
        x += Printer::offsetX;
        y += Printer::offsetY;
        z += Printer::offsetZ;
    }
    // There was conflicting use of IGNOR_COORDINATE
    destinationSteps[X_AXIS] = static_cast<int32_t>(floor(x * axisStepsPerMM[X_AXIS] + 0.5f));
    destinationSteps[Y_AXIS] = static_cast<int32_t>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5f));
    destinationSteps[Z_AXIS] = static_cast<int32_t>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5f));
    if(e != IGNORE_COORDINATE && !Printer::debugDryrun()
#if MIN_EXTRUDER_TEMP > 30
            && (Extruder::current->tempControl.currentTemperatureC > MIN_EXTRUDER_TEMP || Printer::isColdExtrusionAllowed())
#endif
      )
        destinationSteps[E_AXIS] = e * axisStepsPerMM[E_AXIS];
    if(f != IGNORE_COORDINATE)
        feedrate = f;

    if (!PrintLine::queueDeltaMove(ALWAYS_CHECK_ENDSTOPS, pathOptimize, true))
    {
        Com::printWarningFLN("moveToReal / queueDeltaMove returns error");
        SHOWM(x);
        SHOWM(y);
        SHOWM(z);
        return 0;
    }
    return 1;
}

void Printer::setOrigin(float xOff, float yOff, float zOff)
{
    coordinateOffset[X_AXIS] = xOff;
    coordinateOffset[Y_AXIS] = yOff;
    coordinateOffset[Z_AXIS] = zOff;
}

/** Computes currentPosition from currentPositionSteps including correction for offset. */
void Printer::updateCurrentPosition(bool copyLastCmd)
{
    currentPosition[X_AXIS] = static_cast<float>(currentPositionSteps[X_AXIS]) * invAxisStepsPerMM[X_AXIS];
    currentPosition[Y_AXIS] = static_cast<float>(currentPositionSteps[Y_AXIS]) * invAxisStepsPerMM[Y_AXIS];
    currentPosition[Z_AXIS] = static_cast<float>(currentPositionSteps[Z_AXIS]) * invAxisStepsPerMM[Z_AXIS];
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
        transformFromPrinter(currentPosition[X_AXIS], currentPosition[Y_AXIS], currentPosition[Z_AXIS],
                             currentPosition[X_AXIS], currentPosition[Y_AXIS], currentPosition[Z_AXIS]);
#endif // FEATURE_AUTOLEVEL
    currentPosition[X_AXIS] -= Printer::offsetX;
    currentPosition[Y_AXIS] -= Printer::offsetY;
    currentPosition[Z_AXIS] -= Printer::offsetZ;
    if(copyLastCmd)
    {
        lastCmdPos[X_AXIS] = currentPosition[X_AXIS];
        lastCmdPos[Y_AXIS] = currentPosition[Y_AXIS];
        lastCmdPos[Z_AXIS] = currentPosition[Z_AXIS];
    }
}

/**
  \brief Sets the destination coordinates to values stored in com.

  For the computation of the destination, the following facts are considered:
  - Are units inches or mm.
  - Reltive or absolute positioning with special case only extruder relative.
  - Offset in x and y direction for multiple extruder support.
*/

uint8_t Printer::setDestinationStepsFromGCode(GCode *com)
{
    register int32_t p;
    float x, y, z;
#if FEATURE_RETRACTION
    if(com->hasNoXYZ() && com->hasE() && isAutoretract()) { // convert into autoretract
        if(relativeCoordinateMode || relativeExtruderCoordinateMode) {
            Extruder::current->retract(com->E < 0,false);
        } else {
            p = convertToMM(com->E * axisStepsPerMM[E_AXIS]); // current position
            Extruder::current->retract(com->E < p,false);
        }
        return 0; // Fake no move so nothing gets added
    }
#endif
    if(!relativeCoordinateMode)
    {
        if(com->hasX()) lastCmdPos[X_AXIS] = currentPosition[X_AXIS] = convertToMM(com->X) - coordinateOffset[X_AXIS];
        if(com->hasY()) lastCmdPos[Y_AXIS] = currentPosition[Y_AXIS] = convertToMM(com->Y) - coordinateOffset[Y_AXIS];
        if(com->hasZ()) lastCmdPos[Z_AXIS] = currentPosition[Z_AXIS] = convertToMM(com->Z) - coordinateOffset[Z_AXIS];
    }
    else
    {
        if(com->hasX()) currentPosition[X_AXIS] = (lastCmdPos[X_AXIS] += convertToMM(com->X));
        if(com->hasY()) currentPosition[Y_AXIS] = (lastCmdPos[Y_AXIS] += convertToMM(com->Y));
        if(com->hasZ()) currentPosition[Z_AXIS] = (lastCmdPos[Z_AXIS] += convertToMM(com->Z));
    }
#if FEATURE_AUTOLEVEL
    if(isAutolevelActive())
    {
        transformToPrinter(lastCmdPos[X_AXIS] + Printer::offsetX, lastCmdPos[Y_AXIS] + Printer::offsetY, lastCmdPos[Z_AXIS] +  Printer::offsetZ, x, y, z);
    }
    else
#endif // FEATURE_AUTOLEVEL
    {
        x = lastCmdPos[X_AXIS] + Printer::offsetX;
        y = lastCmdPos[Y_AXIS] + Printer::offsetY;
        z = lastCmdPos[Z_AXIS] + Printer::offsetZ;
    }
    destinationSteps[X_AXIS] = static_cast<int32_t>(floor(x * axisStepsPerMM[X_AXIS] + 0.5f));
    destinationSteps[Y_AXIS] = static_cast<int32_t>(floor(y * axisStepsPerMM[Y_AXIS] + 0.5f));
    destinationSteps[Z_AXIS] = static_cast<int32_t>(floor(z * axisStepsPerMM[Z_AXIS] + 0.5f));
    if(com->hasE() && !Printer::debugDryrun())
    {
        p = convertToMM(com->E * axisStepsPerMM[E_AXIS]);

        if(relativeCoordinateMode || relativeExtruderCoordinateMode)
        {
			if (
#if MIN_EXTRUDER_TEMP > 20
				(Extruder::current->tempControl.currentTemperatureC < MIN_EXTRUDER_TEMP && !Printer::isColdExtrusionAllowed())) 
				p = 0; else
#endif
				if(fabs(com->E) * extrusionFactor > EXTRUDE_MAXLENGTH) {
				p = 0;
				if (debugErrors())
					Com::printWarningFLN("Ignoring - E rel exceeds max E length");
			}
            destinationSteps[E_AXIS] = currentPositionSteps[E_AXIS] + p;
        }
        else
        {
			if (
#if MIN_EXTRUDER_TEMP > 20
				(Extruder::current->tempControl.currentTemperatureC < MIN_EXTRUDER_TEMP  && !Printer::isColdExtrusionAllowed()))
				currentPositionSteps[E_AXIS] = p; else
#endif
				if (fabs(p - currentPositionSteps[E_AXIS]) * extrusionFactor > EXTRUDE_MAXLENGTH * axisStepsPerMM[E_AXIS]) {
				currentPositionSteps[E_AXIS] = p;
				if (debugErrors())
					Com::printWarningFLN("Ignoring - E exceeds max E length");
			}
            destinationSteps[E_AXIS] = p;
        }
    }
    else Printer::destinationSteps[E_AXIS] = Printer::currentPositionSteps[E_AXIS];
    if(com->hasF())
    {
        if(unitIsInches)
            feedrate = com->F * 0.0042333f * (float)feedrateMultiply;  // Factor is 25.5/60/100
        else
            feedrate = com->F * (float)feedrateMultiply * 0.00016666666f;
    }
    if(!Printer::isPositionAllowed(lastCmdPos[X_AXIS], lastCmdPos[Y_AXIS], lastCmdPos[Z_AXIS]))
    {
        currentPositionSteps[E_AXIS] = destinationSteps[E_AXIS];
		if (debugErrors())
			Com::printWarningFLN("Ignoring - XYZ position not allowed");
        return false; // ignore move
    }
    return !com->hasNoXYZ() || (com->hasE() && destinationSteps[E_AXIS] != currentPositionSteps[E_AXIS]); // ignore unproductive moves
}

void Printer::setup()
{
    HAL::stopWatchdog();
    //HAL::delayMilliseconds(500);  // add a delay at startup to give hardware time for initalization
    HAL::hwSetup();
#if defined(EEPROM_AVAILABLE) && defined(EEPROM_SPI_ALLIGATOR) && EEPROM_AVAILABLE == EEPROM_SPI_ALLIGATOR
    HAL::spiBegin();
#endif
    Printer::setPowerOn(true);

    //Initialize Step Pins
    SET_OUTPUT(X_STEP_PIN);
    SET_OUTPUT(Y_STEP_PIN);
    SET_OUTPUT(Z_STEP_PIN);

    //Initialize Dir Pins
#if X_DIR_PIN > -1
    SET_OUTPUT(X_DIR_PIN);
#endif
#if Y_DIR_PIN > -1
    SET_OUTPUT(Y_DIR_PIN);
#endif
#if Z_DIR_PIN > -1
    SET_OUTPUT(Z_DIR_PIN);
#endif

    //Steppers default to disabled.
#if X_ENABLE_PIN > -1
    SET_OUTPUT(X_ENABLE_PIN);
    WRITE(X_ENABLE_PIN, !X_ENABLE_ON);
#endif
#if Y_ENABLE_PIN > -1
    SET_OUTPUT(Y_ENABLE_PIN);
    WRITE(Y_ENABLE_PIN, !Y_ENABLE_ON);
#endif
#if Z_ENABLE_PIN > -1
    SET_OUTPUT(Z_ENABLE_PIN);
    WRITE(Z_ENABLE_PIN, !Z_ENABLE_ON);
#endif
#if FEATURE_TWO_XSTEPPER
    SET_OUTPUT(X2_STEP_PIN);
    SET_OUTPUT(X2_DIR_PIN);
#if X2_ENABLE_PIN > -1
    SET_OUTPUT(X2_ENABLE_PIN);
    WRITE(X2_ENABLE_PIN, !X_ENABLE_ON);
#endif
#endif

#if FEATURE_TWO_YSTEPPER
    SET_OUTPUT(Y2_STEP_PIN);
    SET_OUTPUT(Y2_DIR_PIN);
#if Y2_ENABLE_PIN > -1
    SET_OUTPUT(Y2_ENABLE_PIN);
    WRITE(Y2_ENABLE_PIN, !Y_ENABLE_ON);
#endif
#endif

#if FEATURE_TWO_ZSTEPPER
    SET_OUTPUT(Z2_STEP_PIN);
    SET_OUTPUT(Z2_DIR_PIN);
#if Z2_ENABLE_PIN > -1
    SET_OUTPUT(Z2_ENABLE_PIN);
    WRITE(Z2_ENABLE_PIN, !Z_ENABLE_ON);
#endif
#endif

#if FEATURE_THREE_ZSTEPPER
    SET_OUTPUT(Z3_STEP_PIN);
    SET_OUTPUT(Z3_DIR_PIN);
#if Z3_ENABLE_PIN > -1
    SET_OUTPUT(Z3_ENABLE_PIN);
    WRITE(Z3_ENABLE_PIN, !Z_ENABLE_ON);
#endif
#endif
    Endstops::init();
#if FAN_PIN>-1
    SET_OUTPUT(FAN_PIN);
    WRITE(FAN_PIN, LOW);
#endif
#if FAN2_PIN > -1
	SET_OUTPUT(FAN2_PIN);
	WRITE(FAN2_PIN, LOW);
#endif
#if FAN3_PIN > -1
	SET_OUTPUT(FAN3_PIN);
	WRITE(FAN3_PIN, LOW);
#endif
#if FAN_BOARD_PIN>-1
    SET_OUTPUT(FAN_BOARD_PIN);
    WRITE(FAN_BOARD_PIN, LOW);
#endif
#if defined(EXT0_HEATER_PIN) && EXT0_HEATER_PIN>-1
    SET_OUTPUT(EXT0_HEATER_PIN);
    WRITE(EXT0_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT1_HEATER_PIN) && EXT1_HEATER_PIN>-1 && NUM_EXTRUDER>1
    SET_OUTPUT(EXT1_HEATER_PIN);
    WRITE(EXT1_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT2_HEATER_PIN) && EXT2_HEATER_PIN>-1 && NUM_EXTRUDER>2
    SET_OUTPUT(EXT2_HEATER_PIN);
    WRITE(EXT2_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT3_HEATER_PIN) && EXT3_HEATER_PIN>-1 && NUM_EXTRUDER>3
    SET_OUTPUT(EXT3_HEATER_PIN);
    WRITE(EXT3_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT4_HEATER_PIN) && EXT4_HEATER_PIN>-1 && NUM_EXTRUDER>4
    SET_OUTPUT(EXT4_HEATER_PIN);
    WRITE(EXT4_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT5_HEATER_PIN) && EXT5_HEATER_PIN>-1 && NUM_EXTRUDER>5
    SET_OUTPUT(EXT5_HEATER_PIN);
    WRITE(EXT5_HEATER_PIN, HEATER_PINS_INVERTED);
#endif
#if defined(EXT0_EXTRUDER_COOLER_PIN) && EXT0_EXTRUDER_COOLER_PIN>-1
    SET_OUTPUT(EXT0_EXTRUDER_COOLER_PIN);
    WRITE(EXT0_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(EXT1_EXTRUDER_COOLER_PIN) && EXT1_EXTRUDER_COOLER_PIN > -1 && NUM_EXTRUDER > 1
    SET_OUTPUT(EXT1_EXTRUDER_COOLER_PIN);
    WRITE(EXT1_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(EXT2_EXTRUDER_COOLER_PIN) && EXT2_EXTRUDER_COOLER_PIN > -1 && NUM_EXTRUDER > 2
    SET_OUTPUT(EXT2_EXTRUDER_COOLER_PIN);
    WRITE(EXT2_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(EXT3_EXTRUDER_COOLER_PIN) && EXT3_EXTRUDER_COOLER_PIN > -1 && NUM_EXTRUDER > 3
    SET_OUTPUT(EXT3_EXTRUDER_COOLER_PIN);
    WRITE(EXT3_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(EXT4_EXTRUDER_COOLER_PIN) && EXT4_EXTRUDER_COOLER_PIN > -1 && NUM_EXTRUDER > 4
    SET_OUTPUT(EXT4_EXTRUDER_COOLER_PIN);
    WRITE(EXT4_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(EXT5_EXTRUDER_COOLER_PIN) && EXT5_EXTRUDER_COOLER_PIN > -1 && NUM_EXTRUDER > 5
    SET_OUTPUT(EXT5_EXTRUDER_COOLER_PIN);
    WRITE(EXT5_EXTRUDER_COOLER_PIN, LOW);
#endif
#if defined(SUPPORT_LASER) && SUPPORT_LASER
    LaserDriver::initialize();
#endif // defined
#if defined(SUPPORT_CNC) && SUPPORT_CNC
    CNCDriver::initialize();
#endif // defined

#ifdef RED_BLUE_STATUS_LEDS
    SET_OUTPUT(RED_STATUS_LED);
    SET_OUTPUT(BLUE_STATUS_LED);
    WRITE(BLUE_STATUS_LED,HIGH);
    WRITE(RED_STATUS_LED,LOW);
#endif // RED_BLUE_STATUS_LEDS
#if STEPPER_CURRENT_CONTROL != CURRENT_CONTROL_MANUAL
    motorCurrentControlInit(); // Set current if it is firmware controlled
#endif
#if defined(NUM_MOTOR_DRIVERS) && NUM_MOTOR_DRIVERS > 0
    initializeAllMotorDrivers();
#endif // defined
#if FEATURE_AUTOLEVEL
    resetTransformationMatrix(true);
#endif // FEATURE_AUTOLEVEL
    feedrate = 50; ///< Current feedrate in mm/s.
    feedrateMultiply = 100;
    extrudeMultiply = 100;
    lastCmdPos[X_AXIS] = lastCmdPos[Y_AXIS] = lastCmdPos[Z_AXIS] = 0;
#if USE_ADVANCE
#if ENABLE_QUADRATIC_ADVANCE
    advanceExecuted = 0;
#endif
    advanceStepsSet = 0;
#endif
    for(uint8_t i = 0; i < NUM_EXTRUDER + 3; i++) pwm_pos[i] = 0;
    maxJerk = MAX_JERK;
    offsetX = offsetY = offsetZ = 0;
    interval = 5000;
    stepsPerTimerCall = 1;
    msecondsPrinting = 0;
    filamentPrinted = 0;
    flag0 = PRINTER_FLAG0_STEPPER_DISABLED;
    xLength = X_MAX_LENGTH;
    yLength = Y_MAX_LENGTH;
    zLength = Z_MAX_LENGTH;
    xMin = X_MIN_POS;
    yMin = Y_MIN_POS;
    zMin = Z_MIN_POS;
    radius0 = ROD_RADIUS;
#if USE_ADVANCE
    extruderStepsNeeded = 0;
#endif
    EEPROM::initBaudrate();
    Serial.begin(115200);
    //SerialUSB.begin(115200);
    Com::printFLN(Com::tStart);
    HAL::showStartReason();
    Extruder::initExtruder();
    // sets autoleveling in eeprom init
    EEPROM::init(); // Read settings from eeprom if wanted

	//Load axis direction from EEPROM and set flags
	Commands::fillDefAxisDir();

	Printer::setXdir(retDefAxisDir[X_AXIS]);
	Printer::setYdir(retDefAxisDir[Y_AXIS]);
	Printer::setZdir(retDefAxisDir[Z_AXIS]);

    for(uint8_t i = 0; i < E_AXIS_ARRAY; i++)
    {
        currentPositionSteps[i] = 0;
    }
    currentPosition[X_AXIS] = currentPosition[Y_AXIS]= currentPosition[Z_AXIS] =  0.0;
//setAutolevelActive(false); // fixme delete me
    //Commands::printCurrentPosition("Printer::setup 0 ");
#if DISTORTION_CORRECTION
    distortion.init();
#endif // DISTORTION_CORRECTION

    updateDerivedParameter();
    Commands::checkFreeMemory(true);
    HAL::setupTimer();

    transformCartesianStepsToDeltaSteps(Printer::currentPositionSteps, Printer::currentDeltaPositionSteps);

#if DELTA_HOME_ON_POWER
    homeAxis(true,true,true);
#endif
    //setAutoretract(EEPROM_BYTE(AUTORETRACT_ENABLED));
    Commands::printCurrentPosition("Printer::setup ");
    Extruder::selectExtruderById(0);
#if BED_LEDS
if (EEPROM::getBedLED()>1)
	Light.init();
#endif
}

void Printer::defaultLoopActions()
{
    static millis_t lastActivity;

    Commands::checkForPeriodicalActions();  //check heater every n milliseconds
    if(PrintLine::hasLines()) {
        lastActivity = millis();
    } else {
         if(maxInactiveTime && millis() - lastActivity >  maxInactiveTime )
            Printer::kill(false);
        else
            Printer::setAllKilled(false); // prevent repeated kills
        if(stepperInactiveTime && millis() - lastActivity >  stepperInactiveTime )
            Printer::kill(true);
    }

    DEBUG_MEMORY;
}

void Printer::MemoryPosition()
{
    Commands::waitUntilEndOfAllMoves();
    updateCurrentPosition(false);
    realPosition(memoryX, memoryY, memoryZ);
    memoryE = currentPositionSteps[E_AXIS] * invAxisStepsPerMM[E_AXIS];
    memoryF = feedrate;
}

void Printer::GoToMemoryPosition(bool x, bool y, bool z, bool e, float feed)
{
    if(memoryF < 0) return; // Not stored before call, so we ignor eit
    bool all = !(x || y || z);
    moveToReal((all || x ? (lastCmdPos[X_AXIS] = memoryX) : IGNORE_COORDINATE)
               ,(all || y ?(lastCmdPos[Y_AXIS] = memoryY) : IGNORE_COORDINATE)
               ,(all || z ? (lastCmdPos[Z_AXIS] = memoryZ) : IGNORE_COORDINATE)
               ,(e ? memoryE : IGNORE_COORDINATE),
               feed);
    feedrate = memoryF;
    updateCurrentPosition(false);
}

void Printer::deltaMoveToTopEndstops(float feedrate)
{
    for (fast8_t i = 0; i < 3; i++)
        Printer::currentPositionSteps[i] = 0;
    Printer::stepsRemainingAtXHit = -1;
    Printer::stepsRemainingAtYHit = -1;
    Printer::stepsRemainingAtZHit = -1;
    setHoming(true);
    transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
    PrintLine::moveRelativeDistanceInSteps(0, 0, (zMaxSteps + EEPROM::deltaDiagonalRodLength()*axisStepsPerMM[Z_AXIS]) * 1.5, 0, feedrate, true, true);
    offsetX = offsetY = offsetZ = 0;
    setHoming(false);
}
void Printer::homeXAxis()
{
    destinationSteps[X_AXIS] = 0;
    if (!PrintLine::queueDeltaMove(true,false,false))
    {
        Com::printWarningFLN("homeXAxis / queueDeltaMove returns error");
    }
}
void Printer::homeYAxis()
{
    Printer::destinationSteps[Y_AXIS] = 0;
    if (!PrintLine::queueDeltaMove(true,false,false))
    {
        Com::printWarningFLN("homeYAxis / queueDeltaMove returns error");
    }
}
void Printer::homeZAxis() // Delta z homing
{
	bool homingSuccess = false; // By default fail homing (safety feature)

	Commands::checkForPeriodicalActions(); // Temporary disable new command read from buffer

	Printer::deltaMoveToTopEndstops(Printer::homingFeedrate[Z_AXIS]);
	PrintLine::moveRelativeDistanceInSteps(0, 0, axisStepsPerMM[Z_AXIS] * -ENDSTOP_Z_BACK_MOVE, 0, Printer::homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR, true, true);
	if (!(Endstops::xMax() || Endstops::yMax() || Endstops::zMax())) {

		Printer::deltaMoveToTopEndstops(Printer::homingFeedrate[Z_AXIS] / ENDSTOP_Z_RETEST_REDUCTION_FACTOR);
		if (Endstops::xMax() && Endstops::yMax() && Endstops::zMax()) {
			homingSuccess = true;
		}

	}
	// Check if homing failed.  If so, request pause!
	if (!homingSuccess) {
		setHomed(false); // Clear the homed flag
		Com::printFLN("Homing failed!");
	}
	Commands::checkForPeriodicalActions();

    // Correct different endstop heights
    // These can be adjusted by two methods. You can use offsets stored by determining the center
    // or you can use the xyzMinSteps from G100 calibration. Both have the same effect but only one
    // should be measuredas both have the same effect.
    long dx = -xMinSteps - EEPROM::deltaTowerXOffsetSteps();
    long dy = -yMinSteps - EEPROM::deltaTowerYOffsetSteps();
    long dz = -zMinSteps - EEPROM::deltaTowerZOffsetSteps();
    long dm = RMath::min(dx, dy, dz);
    //Com::printFLN(Com::tTower1,dx);
    //Com::printFLN(Com::tTower2,dy);
    //Com::printFLN(Com::tTower3,dz);
    dx -= dm; // now all dxyz are positive
    dy -= dm;
    dz -= dm;
    currentPositionSteps[X_AXIS] = 0; // here we should be
    currentPositionSteps[Y_AXIS] = 0;
    currentPositionSteps[Z_AXIS] = zMaxSteps;
    transformCartesianStepsToDeltaSteps(currentPositionSteps,currentDeltaPositionSteps);
    currentDeltaPositionSteps[A_TOWER] -= dx;
    currentDeltaPositionSteps[B_TOWER] -= dy;
    currentDeltaPositionSteps[C_TOWER] -= dz;
    PrintLine::moveRelativeDistanceInSteps(0, 0, dm, 0, homingFeedrate[Z_AXIS], true, false);
    currentPositionSteps[X_AXIS] = 0; // now we are really here
    currentPositionSteps[Y_AXIS] = 0;
    currentPositionSteps[Z_AXIS] = zMaxSteps; // Extruder is now exactly in the delta center
    coordinateOffset[X_AXIS] = 0;
    coordinateOffset[Y_AXIS] = 0;
    coordinateOffset[Z_AXIS] = 0;
    transformCartesianStepsToDeltaSteps(currentPositionSteps, currentDeltaPositionSteps);
    realDeltaPositionSteps[A_TOWER] = currentDeltaPositionSteps[A_TOWER];
    realDeltaPositionSteps[B_TOWER] = currentDeltaPositionSteps[B_TOWER];
    realDeltaPositionSteps[C_TOWER] = currentDeltaPositionSteps[C_TOWER];
    Extruder::selectExtruderById(Extruder::current->id);
}
// This home axis is for delta
void Printer::homeAxis(bool xaxis,bool yaxis,bool zaxis) // Delta homing code
{
    bool autoLevel = isAutolevelActive();
    setAutolevelActive(false);
    setHomed(true);
    // The delta has to have home capability to zero and set position,
    // so the redundant check is only an opportunity to
    // gratuitously fail due to incorrect settings.
    // The following movements would be meaningless unless it was zeroed for example.
    // Homing Z axis means that you must home X and Y
    homeZAxis();
    moveToReal(0,0,Printer::zLength,IGNORE_COORDINATE,homingFeedrate[Z_AXIS]); // Move to designed coordinates including translation
    updateCurrentPosition(true);
    Commands::printCurrentPosition("homeAxis ");
    setAutolevelActive(autoLevel);
#if BED_LEDS
		if (Printer::ledVal > 1) Light.ShowTemps();
#endif
}

void Printer::babyStep(float Zmm)
{
    bool dir, xDir, yDir, zDir;
    uint32_t steps;
    /* Check for zeros and NaNs */
    if (!Zmm || Zmm != Zmm) return;
    /* Disable motor timer */
    TC_Stop(TIMER1_TIMER, TIMER1_TIMER_CHANNEL);
    /* Save xyz direction settings */
    xDir = Printer::getXDirection();
    yDir = Printer::getYDirection();
    zDir = Printer::getZDirection();
    /* Configure babystep direction */
    dir = (Zmm > 0) ? true : false;
    Printer::setXDirection(dir);
    Printer::setYDirection(dir);
    Printer::setZDirection(dir);
    /* Calculate steps neccessary */
    steps = floor(abs(Zmm)*ZAXIS_STEPS_PER_MM + 0.5);
    /* Execute steps */
    while (steps) {
        startXStep();
        startYStep();
        startZStep();
        HAL::delayMicroseconds(STEPPER_HIGH_DELAY + 2);
        Printer::endXYZSteps();
        HAL::delayMicroseconds(17);
        steps--;
    }
    /* Set old xyz settings */
    Printer::setXDirection(xDir);
    Printer::setYDirection(yDir);
    Printer::setZDirection(zDir);
    /* Reenable motor timer */
    TC_Start(TIMER1_TIMER, TIMER1_TIMER_CHANNEL);
}

#define START_EXTRUDER_CONFIG(i)     Com::printF(Com::tConfig);Com::printF(Com::tExtrDot,i+1);Com::print(':');
void Printer::showConfiguration() {
    Com::config("Baudrate:",baudrate);
#ifndef EXTERNALSERIAL
    Com::config("InputBuffer:",SERIAL_BUFFER_SIZE - 1);
#endif
    Com::config("NumExtruder:",NUM_EXTRUDER);
    Com::config("MixingExtruder:",MIXING_EXTRUDER);
    Com::config("HeatedBed:",HAVE_HEATED_BED);
    Com::config("Fan:",FAN_PIN > -1);
#if defined(FAN2_PIN) && FAN2_PIN > -1	
    Com::config("Fan2:1");
#else
    Com::config("Fan2:0");
#endif	
    Com::config("LCD:", true);
    Com::config("SoftwarePowerSwitch:",0);
    Com::config("XHomeDir:",X_HOME_DIR);
    Com::config("YHomeDir:",Y_HOME_DIR);
    Com::config("ZHomeDir:",Z_HOME_DIR);
    Com::config("SupportG10G11:",FEATURE_RETRACTION);
    Com::config("SupportLocalFilamentchange:",FEATURE_RETRACTION);
    Com::config("CaseLights:",0);
    Com::config("ZProbe:",FEATURE_Z_PROBE);
    Com::config("Autolevel:",FEATURE_AUTOLEVEL);
    Com::config("EEPROM:",EEPROM_MODE != 0);
    Com::config("PrintlineCache:", PRINTLINE_CACHE_SIZE);
    Com::config("JerkXY:",maxJerk);
#if FEATURE_RETRACTION
    Com::config("RetractionLength:",EEPROM_FLOAT(RETRACTION_LENGTH));
    Com::config("RetractionLongLength:",EEPROM_FLOAT(RETRACTION_LONG_LENGTH));
    Com::config("RetractionSpeed:",EEPROM_FLOAT(RETRACTION_SPEED));
    Com::config("RetractionZLift:",EEPROM_FLOAT(RETRACTION_Z_LIFT));
    Com::config("RetractionUndoExtraLength:",EEPROM_FLOAT(RETRACTION_UNDO_EXTRA_LENGTH));
    Com::config("RetractionUndoExtraLongLength:",EEPROM_FLOAT(RETRACTION_UNDO_EXTRA_LONG_LENGTH));
    Com::config("RetractionUndoSpeed:",EEPROM_FLOAT(RETRACTION_UNDO_SPEED));
#endif // FEATURE_RETRACTION
    Com::config("XMin:",xMin);
    Com::config("YMin:",yMin);
    Com::config("ZMin:",zMin);
    Com::config("XMax:",xMin + xLength);
    Com::config("YMax:",yMin + yLength);
    Com::config("ZMax:",zMin + zLength);
    Com::config("XSize:", xLength);
    Com::config("YSize:", yLength);
	Com::config("ZSize:", zLength);
	Com::config("OffsetX:", offsetX);
	Com::config("OffsetY:", offsetY);
	Com::config("OffsetZ:", offsetZ);
    Com::config("XPrintAccel:", maxAccelerationMMPerSquareSecond[X_AXIS]);
    Com::config("YPrintAccel:", maxAccelerationMMPerSquareSecond[Y_AXIS]);
    Com::config("ZPrintAccel:", maxAccelerationMMPerSquareSecond[Z_AXIS]);
    Com::config("XTravelAccel:", maxTravelAccelerationMMPerSquareSecond[X_AXIS]);
    Com::config("YTravelAccel:", maxTravelAccelerationMMPerSquareSecond[Y_AXIS]);
    Com::config("ZTravelAccel:", maxTravelAccelerationMMPerSquareSecond[Z_AXIS]);
    Com::config("PrinterType:Delta");
    Com::config("MaxBedTemp:", HEATED_BED_MAX_TEMP);
    for(fast8_t i = 0; i < NUM_EXTRUDER; i++) {
        START_EXTRUDER_CONFIG(i)
        Com::printFLN("Jerk:",extruder[i].maxStartFeedrate);
        START_EXTRUDER_CONFIG(i)
        Com::printFLN("MaxSpeed:",extruder[i].maxFeedrate);
        START_EXTRUDER_CONFIG(i)
        Com::printFLN("Acceleration:",extruder[i].maxAcceleration);
        START_EXTRUDER_CONFIG(i)
        Com::printFLN("Diameter:",extruder[i].diameter);
        START_EXTRUDER_CONFIG(i)
        Com::printFLN("MaxTemp:",MAXTEMP);
    }
}

#if DISTORTION_CORRECTION

Distortion Printer::distortion;

void Printer::measureDistortion(void)
{
    distortion.measure();
}

Distortion::Distortion()
{
}

void Distortion::init() {
    updateDerived();
#if !DISTORTION_PERMANENT
    resetCorrection();
#endif
#if EEPROM_MODE != 0
    enabled = EEPROM::isZCorrectionEnabled();
    Com::printFLN("zDistortionCorrection:",(int)enabled);
#else
    enabled = false;
#endif
}

void Distortion::updateDerived()
{
    step = (2 * Printer::axisStepsPerMM[Z_AXIS] * DISTORTION_CORRECTION_R) / (DISTORTION_CORRECTION_POINTS - 1.0f);
    radiusCorrectionSteps = DISTORTION_CORRECTION_R * Printer::axisStepsPerMM[Z_AXIS];
    zStart = DISTORTION_START_DEGRADE * Printer::axisStepsPerMM[Z_AXIS];
    zEnd = DISTORTION_END_HEIGHT * Printer::axisStepsPerMM[Z_AXIS];
}

void Distortion::enable(bool permanent)
{
    enabled = true;
#if DISTORTION_PERMANENT && EEPROM_MODE != 0
    if(permanent)
        EEPROM::setZCorrectionEnabled(enabled);
#endif
    Com::printFLN(Com::tZCorrectionEnabled);
}

void Distortion::disable(bool permanent)
{
    enabled = false;
#if DISTORTION_PERMANENT && EEPROM_MODE != 0
    if(permanent)
        EEPROM::setZCorrectionEnabled(enabled);
#endif
	Printer::updateCurrentPosition(false);
    Com::printFLN(Com::tZCorrectionDisabled);
}

void Distortion::reportStatus() {
    Com::printFLN(enabled ? Com::tZCorrectionEnabled : Com::tZCorrectionDisabled);
}

void Distortion::resetCorrection(void)
{
    Com::printInfoFLN("Resetting Z correction");
    for(int i = 0; i < DISTORTION_CORRECTION_POINTS * DISTORTION_CORRECTION_POINTS; i++)
        setMatrix(0, i);
}

int Distortion::matrixIndex(fast8_t x, fast8_t y) const
{
    return static_cast<int>(y) * DISTORTION_CORRECTION_POINTS + x;
}

int32_t Distortion::getMatrix(int index) const
{
#if DISTORTION_PERMANENT
    return EEPROM::getZCorrection(index);
#else
    return matrix[index];
#endif
}
void Distortion::setMatrix(int32_t val, int index)
{
#if DISTORTION_PERMANENT
#if EEPROM_MODE != 0
    EEPROM::setZCorrection(val, index);
#endif
#else
    matrix[index] = val;
#endif
}

bool Distortion::isCorner(fast8_t i, fast8_t j) const
{
    return (i == 0 || i == DISTORTION_CORRECTION_POINTS - 1)
           && (j == 0 || j == DISTORTION_CORRECTION_POINTS - 1);
}

/**
 Extrapolates the changes from p1 to p2 to p3 which has the same distance as p1-p2.
*/
inline int32_t Distortion::extrapolatePoint(fast8_t x1, fast8_t y1, fast8_t x2, fast8_t y2) const
{
    return 2 * getMatrix(matrixIndex(x2,y2)) - getMatrix(matrixIndex(x1,y1));
}

void Distortion::extrapolateCorner(fast8_t x, fast8_t y, fast8_t dx, fast8_t dy)
{
    setMatrix((extrapolatePoint(x + 2 * dx, y, x + dx, y) + extrapolatePoint(x, y + 2 * dy, x, y + dy)) / 2.0,
              matrixIndex(x,y));
}

void Distortion::extrapolateCorners()
{
    const fast8_t m = DISTORTION_CORRECTION_POINTS - 1;
    extrapolateCorner(0, 0, 1, 1);
    extrapolateCorner(0, m, 1,-1);
    extrapolateCorner(m, 0,-1, 1);
    extrapolateCorner(m, m,-1,-1);
}

void Distortion::measure(void)
{
    fast8_t ix, iy;
    float z = EEPROM::zProbeBedDistance() + (EEPROM::zProbeHeight() > 0 ? EEPROM::zProbeHeight() : 0);
    disable(false);
    //Com::printFLN("radiusCorr:", radiusCorrectionSteps);
    //Com::printFLN("steps:", step);
	int32_t zCorrection = 0;
#if Z_HOME_DIR < 0
	zCorrection += Printer::zBedOffset * Printer::axisStepsPerMM[Z_AXIS];
#endif		
#if Z_HOME_DIR < 0 && Z_PROBE_Z_OFFSET_MODE == 1
	zCorrection += Printer::zBedOffset * Printer::axisStepsPerMM[Z_AXIS];
#endif
    for (iy = DISTORTION_CORRECTION_POINTS - 1; iy >= 0; iy--)
        for (ix = 0; ix < DISTORTION_CORRECTION_POINTS; ix++)
        {
            float mtx = Printer::invAxisStepsPerMM[X_AXIS] * (ix * step - radiusCorrectionSteps);
            float mty = Printer::invAxisStepsPerMM[Y_AXIS] * (iy * step - radiusCorrectionSteps);
            //Com::printF("mx ",mtx);
            //Com::printF("my ",mty);
            //Com::printF("ix ",(int)ix);
            //Com::printFLN("iy ",(int)iy);
            Printer::moveToReal(mtx, mty, z, IGNORE_COORDINATE, EEPROM::zProbeXYSpeed());
            setMatrix(floor(0.5f + Printer::axisStepsPerMM[Z_AXIS] * (z -
                        Printer::runZProbe(ix == 0 && iy == DISTORTION_CORRECTION_POINTS - 1, ix == DISTORTION_CORRECTION_POINTS - 1 && iy == 0, Z_PROBE_REPETITIONS))) + zCorrection,
                      matrixIndex(ix,iy));
        }
    // make average center
	// Disabled since we can use grid measurement to get average plane if that is what we want.
	// Shifting z with each measuring is a pain and can result in unexpected behavior.
	/*
    float sum = 0;
    for(int k = 0;k < DISTORTION_CORRECTION_POINTS * DISTORTION_CORRECTION_POINTS; k++)
        sum += getMatrix(k);
    sum /= static_cast<float>(DISTORTION_CORRECTION_POINTS * DISTORTION_CORRECTION_POINTS);
    for(int k = 0;k < DISTORTION_CORRECTION_POINTS * DISTORTION_CORRECTION_POINTS; k++)
        setMatrix(getMatrix(k) - sum, k);
    Printer::zLength -= sum * Printer::invAxisStepsPerMM[Z_AXIS];
	*/
#if EEPROM_MODE
    EEPROM::storeDataIntoEEPROM();
#endif
// print matrix
    Com::printInfoFLN("Distortion correction matrix:");
    for (iy = DISTORTION_CORRECTION_POINTS - 1; iy >=0 ; iy--)
    {
        for(ix = 0; ix < DISTORTION_CORRECTION_POINTS; ix++)
            Com::printF(ix ? ", " : "", getMatrix(matrixIndex(ix,iy)));
        Com::println();
    }
	showMatrix();
    enable(true);
    //Printer::homeAxis(false, false, true);
}

int32_t Distortion::correct(int32_t x, int32_t y, int32_t z) const
{
    if (!enabled || z > zEnd || Printer::isZProbingActive()) return 0.0f;
    if(false && z == 0) {
  Com::printF("correcting (", x); Com::printF(",", y);
    }
    x += radiusCorrectionSteps;
    y += radiusCorrectionSteps;
    int32_t fxFloor = (x - (x < 0 ? step - 1 : 0)) / step; // special case floor for negative integers!
    int32_t fyFloor = (y - (y < 0 ? step -1 : 0)) / step;

// indexes to the matrix
    if (fxFloor < 0)
        fxFloor = 0;
    else if (fxFloor > DISTORTION_CORRECTION_POINTS - 2)
        fxFloor = DISTORTION_CORRECTION_POINTS - 2;
    if (fyFloor < 0)
        fyFloor = 0;
    else if (fyFloor > DISTORTION_CORRECTION_POINTS - 2)
        fyFloor = DISTORTION_CORRECTION_POINTS - 2;

// position between cells of matrix, range=0 to 1 - outside of the matrix the value will be outside this range and the value will be extrapolated
    int32_t fx = x - fxFloor * step; // Grid normalized coordinates
    int32_t fy = y - fyFloor * step;

    int32_t idx11 = matrixIndex(fxFloor, fyFloor);
    int32_t m11 = getMatrix(idx11), m12 = getMatrix(idx11 + 1);
    int32_t m21 = getMatrix(idx11 + DISTORTION_CORRECTION_POINTS);
    int32_t m22 = getMatrix(idx11 + DISTORTION_CORRECTION_POINTS + 1);
    int32_t zx1 = m11 + ((m12 - m11) * fx) / step;
    int32_t zx2 = m21 + ((m22 - m21) * fx) / step;
    int32_t correction_z = zx1 + ((zx2 - zx1) * fy) / step;
    if(false && z == 0) {
      Com::printF(") by ", correction_z);
      Com::printF(" ix= ", fxFloor); Com::printF(" fx= ", fx);
      Com::printF(" iy= ", fyFloor); Com::printFLN(" fy= ", fy);
    }
    if (z > zStart && z > 0)
        correction_z *= (zEnd - z) / (zEnd - zStart);
   /* if(correction_z > 20 || correction_z < -20) {
            Com::printFLN("Corr. error too big:",correction_z);
        Com::printF("fxf",(int)fxFloor);
        Com::printF(" fyf",(int)fyFloor);
        Com::printF(" fx",fx);
        Com::printF(" fy",fy);
        Com::printF(" x",x);
        Com::printFLN(" y",y);
        Com::printF(" m11:",m11);
        Com::printF(" m12:",m12);
        Com::printF(" m21:",m21);
        Com::printF(" m22:",m22);
        Com::printFLN(" step:",step);
        correction_z = 0;
    }*/
    return correction_z;
}

void Distortion::set(float x,float y,float z) {
	int ix = (x * Printer::axisStepsPerMM[Z_AXIS] + radiusCorrectionSteps + step / 2) / step;
	int iy = (y * Printer::axisStepsPerMM[Z_AXIS] + radiusCorrectionSteps + step / 2) / step;
	if(ix < 0) ix = 0;
	if(iy < 0) iy = 0;
	if(ix >= DISTORTION_CORRECTION_POINTS-1) ix = DISTORTION_CORRECTION_POINTS-1;
	if(iy >= DISTORTION_CORRECTION_POINTS-1) iy = DISTORTION_CORRECTION_POINTS-1;
	int32_t idx = matrixIndex(ix, iy);
	setMatrix(z * Printer::axisStepsPerMM[Z_AXIS],idx);
}

void Distortion::showMatrix() {
	for(int ix = 0;ix < DISTORTION_CORRECTION_POINTS; ix++) {
		for(int iy = 0;iy < DISTORTION_CORRECTION_POINTS; iy++) {
			float x = (-radiusCorrectionSteps + ix * step) * Printer::invAxisStepsPerMM[Z_AXIS];
			float y = (-radiusCorrectionSteps + iy * step) * Printer::invAxisStepsPerMM[Z_AXIS];
			int32_t idx = matrixIndex(ix, iy);
			float z = getMatrix(idx) * Printer::invAxisStepsPerMM[Z_AXIS];
			Com::printF("Distortion correction at px:",x,2);
			Com::printF(" py:",y,2);
			Com::printFLN(" zCoorection:",z,3);
		}
	}
}

#endif // DISTORTION_CORRECTION
