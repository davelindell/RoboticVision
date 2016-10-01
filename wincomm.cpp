#include "StdAfx.h"
#include <time.h>
#include <vector>
#include "atlstr.h"
using namespace std;

HANDLE HandleSerPort;
int CommRunning;
int FirstStart = true;

int SerialGet(int nBytesToRead, char* message) {
	DWORD nBytes = 0;
	time_t finish;
	int success = 0;

	finish = time(NULL) + 5;
	while ((success == 0) && (time(NULL) < finish)) {
		ReadFile(HandleSerPort, message, nBytesToRead, &nBytes, NULL);
		success = nBytes;
	}
	if (success == 0) {
		printf("Timed out waiting for communication\n");
		return 0;
	} else {
		return success;
	}
}


void SerialPutC(char c) {
	DWORD nBytes;
	if (!WriteFile(HandleSerPort, &c, 1, &nBytes, NULL)) {
		printf("Problem writing to serial port\n");
		CommRunning = false;
	}
}

void SerialPutS(unsigned char *s, int Length) {
	DWORD nBytes;
	if (!WriteFile(HandleSerPort, s, Length, &nBytes, NULL)) {
		printf("Problem writing to serial port\n");
		CommRunning = false;
	}
}

boolean SerialSetup(char* PortName, int BaudRate) {
	DCB dcbSerPort;
	COMMTIMEOUTS t = {MAXDWORD, 0, 0, 0, 0};
	int CommRunning = false;
	int CommBaud;

	switch (BaudRate) {
	case 9600:	CommBaud = CBR_9600; break;
	case 19200:	CommBaud = CBR_19200; break;
	case 38400:	CommBaud = CBR_38400; break;
	case 57600:	CommBaud = CBR_57600; break;
	case 115200:CommBaud = CBR_115200; break;
	default: CommBaud = CBR_115200; break;
	}

	HandleSerPort = CreateFile(CA2W(PortName),
								GENERIC_READ | GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);

	if (HandleSerPort == INVALID_HANDLE_VALUE) {
		printf("Cannot open serial port %s\n", PortName);
		return false;
	} else {
		printf("Open serial port %s\n", PortName);
	}


	GetCommState(HandleSerPort, &dcbSerPort);
	dcbSerPort.BaudRate = CommBaud;
	dcbSerPort.fBinary = TRUE;
	dcbSerPort.fParity = FALSE;
	dcbSerPort.fDsrSensitivity = FALSE;
	dcbSerPort.fOutxDsrFlow = FALSE;
	dcbSerPort.fOutX = FALSE;
	dcbSerPort.fInX = FALSE;
	dcbSerPort.fNull = FALSE;
	dcbSerPort.fAbortOnError = FALSE;
	dcbSerPort.ByteSize = 8;
	dcbSerPort.Parity = NOPARITY;
	dcbSerPort.StopBits = ONESTOPBIT;

	if (!SetCommState(HandleSerPort, &dcbSerPort)) {
		printf("Bad parameters for port");
		return false;
	}

	SetCommTimeouts(HandleSerPort, &t);
	CommRunning = true;
	return true;
}

void SerialShutdown(void) {
	if (FirstStart) {
		FirstStart = false;
		return;
	}

	if (!CloseHandle(HandleSerPort)) {
		printf("Problem closing the serial port");
		CommRunning = false;
	}
}

void DetectComPort(std::vector<CString> *PortNames) //added function to find the present serial 
{
	TCHAR lpTargetPath[5000]; // buffer to store the path of the COMPORTS
    DWORD test;
    bool gotPort=0; // in case the port is not found

    for(int i=0; i<255; i++) // checking ports from COM0 to COM255
    {
        CString str;
        str.Format(_T("%d"),i);
        CString ComName=CString("COM") + CString(str); // converting to COM0, COM1, COM2

        test = QueryDosDevice(ComName, (LPWSTR)lpTargetPath, 5000);

            // Test the return value and error if any
        if(test!=0) //QueryDosDevice returns zero if it didn't find an object
        {
			PortNames->push_back(ComName);
            // printf("%s\n", ComName);
            gotPort++; // found port
        }

        if(::GetLastError()==ERROR_INSUFFICIENT_BUFFER)
        {
            lpTargetPath[10000]; // in case the buffer got filled, increase size of the buffer.
            continue;
        }
    }
    if(!gotPort) // if not port
		printf("No port found\n");
}
