// class header
#include "stdafx.h"
#include "RoboteqWrapper.h"
#include <iostream>
#include <vector>
#include "wincomm.h"

// CONSTRUCTOR
RoboteqWrapper::RoboteqWrapper() {
#ifdef USE_STAGE
	// Move motors have different moving range
	LowCountLimit[X_MOTOR] = 0;
	LowCountLimit[Y_MOTOR] = 0;
	HighCountLimit[X_MOTOR] = X_INCH_COUNTS*CATCHER_W;
	HighCountLimit[Y_MOTOR] = Y_INCH_COUNTS*CATCHER_H;
	PID_ICAP[X_MOTOR] = X_PID_ICAP;
	PID_KP[X_MOTOR] = X_PID_KP;
	PID_KI[X_MOTOR] = X_PID_KI;
	PID_KD[X_MOTOR] = X_PID_KD;
	PID_ICAP[Y_MOTOR] = Y_PID_ICAP;
	PID_KP[Y_MOTOR] = Y_PID_KP;
	PID_KI[Y_MOTOR] = Y_PID_KI;
	PID_KD[Y_MOTOR] = Y_PID_KD;
	MotorMode[X_MOTOR] = MotorMode[Y_MOTOR] = OPEN_MODE;
	ConnectToControllers();
#endif
}

// DESTRUCTOR
RoboteqWrapper::~RoboteqWrapper() {
#ifdef USE_STAGE
	SetMotorMode(X_MOTOR, OPEN_LOOP_SP);
	SetSpeedOrPosition(X_MOTOR, 0);
	SetMotorMode(Y_MOTOR, OPEN_LOOP_SP);
	SetSpeedOrPosition(Y_MOTOR, 0);
	SaveEEPROM();
	// Device[i].Disconnect();   // Don't need to disconnect the device.  It is done in the Device destructor
#endif
}

void RoboteqWrapper::ConnectToControllers()
{
#ifdef USE_STAGE
	int i;
	vector<CString>	PortNameList;
	char Command[32];
	string Read;
	CStringW	Msg;

	DetectComPort(&PortNameList);							// Detect all available COM ports and return as a vector of CString
	for (i = 0; i < PortNameList.size(); i++) {
		string Port = CStringA(PortNameList.at(i)).GetString();
		int status = Device.Connect(Port);					// connect through serial port
		if (status != RQ_SUCCESS) {
			Msg.Format(L"Connection to %s failed", PortNameList.at(i).GetString());
			//			AfxMessageBox(Msg);
		}
		else {
			break;
		}
	}	// Detect all devices
	// Set to Open loop speed mode and power to 0% (stop) as default
	SetMotorMode(X_MOTOR, OPEN_LOOP_SP);
	SetSpeedOrPosition(X_MOTOR, 0);
	SetMotorMode(Y_MOTOR, OPEN_LOOP_SP);
	SetSpeedOrPosition(Y_MOTOR, 0);
	if (Device.IsConnected()) {
		sprintf_s(Command, "^MXPF %d %d\r", X_MOTOR, INIT_POWER);    // ^MXPF cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor forward maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary			
		sprintf_s(Command, "^MXPF %d %d\r", Y_MOTOR, INIT_POWER);    // ^MXPF cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor forward maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary			
		sprintf_s(Command, "^MXPR %d %d\r", X_MOTOR, INIT_POWER);    // ^MXPR cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor reverse maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary			
		sprintf_s(Command, "^MXPR %d %d\r", Y_MOTOR, INIT_POWER);    // ^MXPR cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor reverse maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary		
		SetWDT(WDT_TIMEOUT);					// delay for WDT_TIMEOUT seconds then set command to 0 position.  
	}
#endif
}

void RoboteqWrapper::SetDeviceDefault()
{
#ifdef USE_STAGE
	char Command[32];
	if (!Device.IsConnected()) return;
	// LoadEEPROM(i);				//Actually there is no need to load from EEPROM to RAM because it is done at power up automatically	
	// Encoder Setting
	for (int i = Y_MOTOR; i <= X_MOTOR; i++) {
		sprintf_s(Command, "^EMOD %d %d\r", i, (i == Y_MOTOR ? Y_OFFSET+2 : X_OFFSET + 2));    // ^EMOD cc (aa + mm) cc:Channel number, aa = 0 : Unused 1 : Command 2 : Feedback, mm = mot1 * 16 + mot2 * 32
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder operation mode");		// set encoder operation mode to feedback. value = MOT1 (16) + FEEDBACK (2) 
		sprintf_s(Command, "^EPPR %d %d\r", i, (i == Y_MOTOR ? Y_MOTOR_PPR : X_MOTOR_PPR));			// ^EPPR cc nn   cc : Channel number, nn : PPR value (1 to 5000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder PPR value");			// set encoder pulse per revolution value 
		sprintf_s(Command, "^ELL %d %d\r", i, LowCountLimit[i]);								// ^ELL cc nn     cc : Channel number, nn : low count limit (default -20000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder low limit");			// set encoder low limit
		sprintf_s(Command, "^EHL %d %d\r", i, HighCountLimit[i]);								// ^EHL cc nn     cc : Channel number, nn : high count limit (default 20000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder high limit");			// set encoder high limi
		// Set encoder home value.  This value will be loaded in the selected encoder counter when a home switch is detected, 
		// or when a Home command is received from the serial / USB, or issued from a MicroBasic script.
		// Y home value is zero.  X home value is the HighCount because of wiring.  
		sprintf_s(Command, "^EHOME %d %d\r", i, (i == Y_MOTOR ? 0 : HighCountLimit[i]));				// ^EHOME cc nn     cc : Channel number, nn : home value
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder home value");	// This value will be loaded when the stage moves the home position			
		// Set General Power Settings
		sprintf_s(Command, "^ALIM %d %d\r", i, 200);				// ^ALIM cc nn     cc : Channel number, nn :  amp limit = amps * 10, e.g., enter 200 for 20 amps		
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting amps limit");	// Input value = amps * 10, e.g., enter 200 for 20 amps			
		// Amps Triger 20 A, Amps Trigger Delay 500 mSec - Not needed if no action is required
		sprintf_s(Command, "^MXPF %d %d\r", i, INIT_POWER);    // ^MXPF cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor forward maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary			
		sprintf_s(Command, "^MXPR %d %d\r", i, INIT_POWER);    // ^MXPR cc nn (25~100)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor reverse maximum power");	// Input value = %  Set it to 25% first and adjust later if necessary			
		// Motor Settings
		sprintf_s(Command, "^MXRPM %d %d\r", i, MAX_RPM);    // ^MXRPM cc nn   cc: channel, nn: Max RPM (10 to 65000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor maximum RPM");	// Maximum RPM that is to be set as +1,000 for relative speed
		sprintf_s(Command, "^MAC %d %d\r", i, ACELERATION);    // ^MAC cc nn   cc: channel, nn:  Acceleration time in 0.1 RPM per second (100 to 32000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor aceleration RPM");	// Aceleration is 0.1RPM per second.  1,000 will acelerate 100RPM per esecond		
		sprintf_s(Command, "^MDEC %d %d\r", i, DECELERATION);  // ^MDEC cc nn   cc: channel, nn:  Deceleration time in 0.1 RPM per second (100 to 32000)
		if (Device.Write(Command) != RQ_SUCCESS)  AfxMessageBox(L"Error setting motor deceleration RPM");// DEceleration is 0.1RPM per second.  1,000 will decelerate 100RPM per esecond		
		// Close Loop Parameters
		sprintf_s(Command, "^MVEL %d %d\r", i, POS_VELOCITY);  // ^MVEL cc nn   cc: channel, nn:  velocity in RPM
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting default position velocity");		// Velocity in RPM in position mode
		sprintf_s(Command, "^CLERD %d %d\r", i, 2);  // ^CLERD cc nn   cc: channel, nn:  nn = 0 : Detection disabled, 1 : 250ms at Error > 100, 2 : 500ms at Error > 250, 3 : 1000ms at Error > 500 (default 2)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting closed loop error detection");	// 0:Detection disabled 1:250ms at Error > 100 2:500ms at Error > 250  3:1000ms at Error > 500
		sprintf_s(Command, "^ICAP %d %d\r", i, PID_ICAP[i]);  // ^ICAP cc nn   cc: channel, nn: Integral cap in % (1% to 100%)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting integral limit");	// Default 100%
		sprintf_s(Command, "^KP %d %d\r", i, PID_KP[i]);  // ^KP cc nn   cc: channel, nn:  Gain *10 (0  to 250)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting propotional gain");
		sprintf_s(Command, "^KI %d %d\r", i, PID_KI[i]);  // ^KI cc nn   cc: channel, nn:  Deceleration time in 0.1 RPM per second (100 to 32000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting Integral Gain");
		sprintf_s(Command, "^KD %d %d\r", i, PID_KD[i]);  // ^KD cc nn   cc: channel, nn:  Deceleration time in 0.1 RPM per second (100 to 32000)
		if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting Differential Gain");
	}
	// Set General Power Settings
	sprintf_s(Command, "^PWMF %d\r", 180);  // ^PWMF cc nn   nn:  Frequency * 10 (10 to 200 or 1 to 20 KHz, default 180=18.0 KHz)
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting PWM frequency");			// Input value = Frequency in KHz * 10, e.g., enter 200 for 20 KHz			
	sprintf_s(Command, "^OVL %d\r", 600);  // ^OVL nn   nn:  voltage * 10, e.g., enter 350 for 35 volts
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting overvoltage limit");		// Input value = voltage * 10, e.g., enter 350 for 35 volts			
	sprintf_s(Command, "^UVL %d\r", 50);  // ^UVL nn   nn:  voltage * 10, e.g., enter 50 for 5 volts
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting undervoltage limit");		// Input value = voltage * 10, e.g., enter 50 for 5 volts							
	sprintf_s(Command, "^ECHOF %d\r", 1);  // ^ECHOF nn   nn:  0 eanbled 1 disabled
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error turning echo off");	// 1: disabled, 0:enabled
	// Set DIN3 nd DIN4 to be active low 
	sprintf_s(Command, "^DINL %d\r", 12);    // L1 + (L2 * 2) + (L3 * 4) + (L4 * 8) + (L5 * 16) + (L6 * 32)
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting din to run script");	// This value will set din to rnu script			
	sprintf_s(Command, "^DINA %d %d\r", X_SWITCH, X_OFFSET + 4);    // ^DINA cc (aa + mm) cc:Channel number, aa = 6 : forward limit switch,  mm = mot1 * 16 + mot2 * 32
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting din to run script");	// This value will set din to rnu script			
	sprintf_s(Command, "^DINA %d %d\r", Y_SWITCH, Y_OFFSET + 5);    // ^DINA cc (aa + mm) cc:Channel number, aa = 5 : reverse limit switch,  mm = mot1 * 16 + mot2 * 32
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting din to run script");	// This value will set din to rnu script			
	SaveEEPROM();
#endif
}

void RoboteqWrapper::SetMotorMode(int Channel, int Mode) {
#ifdef USE_STAGE
	//	^MMOD cc nn, cc = motor channel, nn = 0 : open - loop speed, 1 : closed - loop speed, 2 : closed - loop position relative, 
	//		3 : closed - loop count position 4 : closed - loop position tracking, 5 : torque
	if (!Device.IsConnected()) return;
	char Command[32];
	sprintf_s(Command, "^MMOD %d %d\r", Channel, Mode);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor mode");
#endif
}

void RoboteqWrapper::SetEncoderCount(int Channel, int Count) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return;
	//Set Encoder Counters  !C [nn] mm for the current location
	Count = (Count < LowCountLimit[Channel]) ? LowCountLimit[Channel] : Count;
	Count = (Count > HighCountLimit[Channel]) ? HighCountLimit[Channel] : Count;
	char Command[32];
	sprintf_s(Command, "!C %d %d\r", Channel, Count);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting encoder counter value");
#endif
}

void RoboteqWrapper::GetEncoderCount(int Channel, char Count[]) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return;
	char Command[32];
	string Read;
	sprintf_s(Command, "?C %d\r", Channel);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error requesting encoder count");
	if (Device.ReadAll(Read) != RQ_SUCCESS) AfxMessageBox(L"Error reading encoder count");
	size_t found = Read.find_last_of("C=");										// Remove C=
	strncpy_s(Count, 32, &Read.at(found + 1), Read.length() - found - 2);		// ignore the last two characters "r\"
#endif
}

long RoboteqWrapper::GetEncoderCount(int Channel) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return 0;
	char Command[32];
	string Read;
	sprintf_s(Command, "?C %d\r", Channel);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error requesting encoder count");
	if (Device.ReadAll(Read) != RQ_SUCCESS) AfxMessageBox(L"Error reading encoder count");
	size_t found = Read.find_last_of("C=");								// Remove C=
	strncpy_s(Command, 32, &Read.at(found + 1), Read.length() - found - 2);		// ignore the last two characters "r\"
	return atoi(Command);
#else
	return (0);
#endif
}

long RoboteqWrapper::GetMotorSpeed(int Channel) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return 0;
	// Read motor speed in RPM
	char Command[32];
	string Read;
	sprintf_s(Command, "?S %d\r", Channel);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error requesting motor speed");
	if (Device.ReadAll(Read) != RQ_SUCCESS) AfxMessageBox(L"Error reading motor speed");
	size_t found = Read.find_last_of("S=");								// Remove C=
	strncpy_s(Command, 32, &Read.at(found + 1), Read.length() - found - 2);		// ignore the last two characters "r\"
	return atoi(Command);
#else
	return (0);
#endif
}

long RoboteqWrapper::SetSpeedOrPosition(int Channel, int Speed_Position) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return 0;
	//	!G [nn] mm, mm : -1000 ~ 1000	
	char Command[32];
	Speed_Position = (Speed_Position < -MAX_RANGE) ? -MAX_RANGE : Speed_Position;
	Speed_Position = (Speed_Position > MAX_RANGE) ? MAX_RANGE : Speed_Position;
	sprintf_s(Command, "!G %d %d\r", Channel, Speed_Position);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error setting motor speed");
	return Speed_Position;
#else
	return 0;
#endif
}

void RoboteqWrapper::StopMotor(int Channel) {			// stop the motor immediately (not just setting to zero speed and wait for it to stop)
#ifdef USE_STAGE
	if (!Device.IsConnected()) return;
	char Command[32];
	sprintf_s(Command, "!MS %d\r", Channel);
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error stopping motor");
#endif
}

void RoboteqWrapper::ReleaseMotor(int Channel) {			// release emergency stop, no channel selection
#ifdef USE_STAGE
	if (!Device.IsConnected()) return;
	char Command[32];
	sprintf_s(Command, "!MG\r");
	if (Device.Write(Command) != RQ_SUCCESS) AfxMessageBox(L"Error releasing motors");
#endif
}

BOOL RoboteqWrapper::IsMotorStopped(int Channel) {		// check motor status
#ifdef USE_STAGE
	if (!Device.IsConnected()) return TRUE;
	do {
		Sleep(POLL_TIME);
	} while (GetMotorSpeed(Channel) != 0);
#endif
	return TRUE;
}

void RoboteqWrapper::Move(double x, double y) {
#ifdef USE_STAGE
	if (!Device.IsConnected()) return;
	int x_relative, y_relative;
	char command[32];

	y_relative = y * 2000 / CATCHER_H;		// Total distance travel is normalized to -1000 ~ +1000
	y_relative = (y_relative > MAX_RANGE) ? MAX_RANGE : y_relative;
	y_relative = (y_relative < -MAX_RANGE) ? -MAX_RANGE : y_relative;

	x_relative = x * 2000 / CATCHER_W;		// Total distance travel is normalized to -1000 ~ +1000
	x_relative = (x_relative > MAX_RANGE) ? MAX_RANGE : x_relative;
	x_relative = (x_relative < -MAX_RANGE) ? -MAX_RANGE : x_relative;

	sprintf_s(command, "!$01 %d %d\r", y_relative, x_relative);		// 1: Y_MOTOR, 2: X_MOTOR
	if (Device.Write(command) != RQ_SUCCESS) AfxMessageBox(L"Error sending move and kick command");
#endif
}

void RoboteqWrapper::LoadEEPROM() {
#ifdef USE_STAGE
	char command[32];
	string read;
	sprintf_s(command, "%%EELD\r");			// need two %'s to specify one %
	if (Device.Write(command) != RQ_SUCCESS) AfxMessageBox(L"Error loading parameters");
	if (Device.ReadAll(read) != RQ_SUCCESS) AfxMessageBox(L"Error reading feedback of load command");		// should return '+' to acknowledge
#endif
}

void RoboteqWrapper::SaveEEPROM() {
#ifdef USE_STAGE
	char command[32];
	string read;
	sprintf_s(command, "%%EESAV\r");		// need two %'s to specify one %
	if (Device.Write(command) != RQ_SUCCESS) AfxMessageBox(L"Error savng parameters");
	if (Device.ReadAll(read) != RQ_SUCCESS) AfxMessageBox(L"Error reading feedback of save command");		// should return '+' to acknowledge
#endif
}

// set watchdog timer timeout value in mSec, 0 to disable.  command moves to 0 after the timeout
void RoboteqWrapper::SetWDT(int timeout)
{
#ifdef USE_STAGE
	char command[32];
	string read;
	sprintf_s(command, "^RWD %d\r", timeout);		// need two %'s to specify one %
	if (Device.Write(command) != RQ_SUCCESS) AfxMessageBox(L"Error setting WDT timeout");
//	if (Device.ReadAll(read) != RQ_SUCCESS) AfxMessageBox(L"Error reading WDT timeout");	// should return '+' to acknowledge
#endif
}
