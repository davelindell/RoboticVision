#ifndef WINCOMM_H
#define WINCOMM_H
#include <windows.h>
#include <vector>
#include "atlstr.h"
using namespace std;

int SerialGet(int nBytesToRead, char *message);
void SerialPutC(char c);
void SerialPutS(unsigned char *s, int Length);
boolean SerialSetup(char* PortName, int BaudRate);
void SerialShutdown(void);
void DetectComPort(std::vector<CString> *PortNames);

#endif
