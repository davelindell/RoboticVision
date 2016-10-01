
// BaseballCatcherDlg.h : header file
//

#pragma once
#include "Hardware.h"

// CBaseballCatcherDlg dialog
class CBaseballCatcherDlg : public CDialogEx, public CTCSys
{
// Construction
public:
	CBaseballCatcherDlg(CWnd* pParent = NULL);	// standard constructor
	CBitmapButton		m_Exit;
// Dialog Data
	enum { IDD = IDD_BASEBALLCATCHER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLoadimg();
	afx_msg void OnBnClickedSaveimg();
	afx_msg void OnBnClickedUpdateimg();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedGrab();
	afx_msg void OnBnClickedCapture();
	afx_msg void OnBnClickedShowseq();
	afx_msg void OnBnClickedCenter();
	afx_msg void OnBnClickedMove();
	afx_msg void OnBnClickedReset();
	afx_msg void OnBnClickedCatch();
	BOOL m_UpdateImage;
	BOOL m_Acquisition;
	int m_PlayDelay;
	double m_CatchX;
	double m_CatchY;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedExit();
	afx_msg void OnBnClickedLoadMotor();
};
