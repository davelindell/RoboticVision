
// VisualInspection.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CVisualInspectionApp:
// See VisualInspection.cpp for the implementation of this class
//

class CVisualInspectionApp : public CWinApp
{
public:
	CVisualInspectionApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CVisualInspectionApp theApp;