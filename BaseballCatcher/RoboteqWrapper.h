/* DESCRIPTION


*/

#ifndef ROBOTEQWRAPPER_H
#define ROBOTEQWRAPPER_H 

using namespace std;
#include <iostream>
#include <stdio.h>
#include <string.h>

//Bototeq header files
#include "RoboteqDevice.h"
#include "ErrorCodes.h"
#include "Constants.h"

#define USE_STAGE			// Using the X-Y Stage?   

// Doigital input values for limit switch inputs
// DIN3 and DIN4 are hard wired for Y_AXIS_SWITCH and X_AXIS_SWITCH
#define Y_MOTOR			1		// Motor 1 is used for Y
#define X_MOTOR			2		// Motor 2 is used for X
#define Y_SWITCH		3		// Y switches are wired for vertical
#define X_SWITCH		4		// X switches are wired for horizontal
#define Y_OFFSET		16		// offset for motor 1 (Y axis). It is 32 in the manual but it is wrong.
#define X_OFFSET		32		// offset for motor 2 (X axis). It is 64 in the manual but it is wrong.
#define X_MOTOR_PPR		50		// Motor pulses per revolution.  Controller get 4 counts per pulse.
#define Y_MOTOR_PPR		50		// Motor pulses per revolution.  Controller get 4 counts per pulse.

#define CATCHER_H		16.0	// catcher height in inches
#define	CATCHER_W		20.0	// catcher width in inches
#define Y_INCH_COUNTS	218		// # of encoder counts per linear inch = 4 (quadrature) * encoder pulses per linear inch.  This depends on the gear ratio 
#define X_INCH_COUNTS	85		// # of encoder counts per linear inch = 4 (quadrature) * encoder pulses per linear inch.  This depends on the gear ratio
#define INIT_POWER		100		// Initial power output %

#define WDT_TIMEOUT		0		// WDT timeout in mSec, 0 to disable .  Delay for WDT_TIMEOUT seconds then set command to 0 position.
#define OPEN_LOOP_SP	0		// open loop speed mode
#define CLOSED_LOOP_SP	1		// closed loop speed
#define CLOSED_LOOP_POS	2		// closed loop position relative
#define OPEN_MODE		0
#define CLOSE_MODE		1
#define MAX_RANGE		950		// Closed loop position mode can only send relative position +/- 1000
#define	POLL_TIME		200		// polling time interval

#define MAX_POWER		100		// Maximum output power %  
#define MAX_RPM			4000	// Maximum motor RPM at  full power that is to be used as +1,000 in relative
#define ACELERATION		500000	// Must be 100~32,000.  Countedd as 0.1RPM per second, e.g., 10,000 means it will acelerate 10,000x0.1=1,000 RPM per second.  
#define DECELERATION	500000	// Must be 100~32,000.  Countedd as 0.1RPM per second, e.g., 10,000 means it will decelerate 10,000x0.1=1,000 RPM per second.  
#define POS_VELOCITY	4000	// Default position mode velocity in RMP
#define OPEN_LOOP_LIMIT	0.7		// Open loop speed limit to % of full speed

#define Y_PID_ICAP		100		// default 100%
#define Y_PID_KP		120		// Proportional Gain, 0~250 and 150 means 15.0
#define Y_PID_KI		0		// Integral Gain, 0~250 and 150 means 15.0
#define Y_PID_KD		00		// Differential Gain, 0~250 and 150 means 15.0

#define X_PID_ICAP		100		// default 100%
#define X_PID_KP		80		// Proportional Gain, 0~250 and 150 means 15.0
#define X_PID_KI		0		// Integral Gain, 0~250 and 150 means 15.0
#define X_PID_KD		0		// Differential Gain, 0~250 and 150 means 15.0

class RoboteqWrapper {
	
public:
	RoboteqWrapper();
	~RoboteqWrapper();
	// Module info
	RoboteqDevice	Device;									// controller device handler
	// Set al to 3 because X_MOTOR is wired to channel 2 and Y_MOTOR is wired to channel 1
	long			LowCountLimit[3];						// store low count limits to be easily loaded into each board
	long			HighCountLimit[3];						// store high count limits to be easily loaded into each board
	long			PID_ICAP[3];
	long			PID_KP[3];
	long			PID_KI[3];
	long			PID_KD[3];
	long			MotorMode[3];
	double			Move_X, Move_Y;									// Distance to move 
	// Configuration
	void			ConnectToControllers();							// Connect to controllers
	void			SetDeviceDefault();								// set controller default parameters
	// Motor Control
	void			SetMotorMode(int Channel, int Mode);			// Set motor operation mode
	void			SetEncoderCount(int Channel, int Count);		// set encoder count at current location
	void			GetEncoderCount(int Channel, char Count[]);		// read encoder count and return a char string
	long			GetEncoderCount(int Channel);					// read encoder count and return a long integer
	long			GetMotorSpeed(int Channel);						// read motor speed in actual RPM in any mode
	// Motor Movement
	long			SetSpeedOrPosition(int Channel, int Speed_position);	// Set open loop sppeed -1000 ~ 1000 or close loop position
	void			StopMotor(int Channel);									// stop the motor immediately (not just setting to zero speed and wait for it to stop)
	void			ReleaseMotor(int Channel);								// release emergency stop
	BOOL			IsMotorStopped(int Channel);							// check motor status
	void			Move(double x, double y);						// degrees from the bottom, + back, - front  

	void			LoadEEPROM();									// load parameters from EEPROM
	void			SaveEEPROM();									// save parameters to EEPROM
	void			SetWDT(int Time);								// set watchdog timer timeout value in mSec, 0 to disable
}; // end RoboteqWrapper
#endif
