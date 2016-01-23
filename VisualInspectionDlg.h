
// VisualInspectionDlg.h : header file
//

#pragma once
#include "Hardware.h"

// CVisualInspectionDlg dialog
class CVisualInspectionDlg : public CDialogEx, public CTCSys
{
// Construction
public:
	CVisualInspectionDlg(CWnd* pParent = NULL);	// standard constructor
	CBitmapButton		m_Exit;

// Dialog Data
	enum { IDD = IDD_VISUALINSPECTION_DIALOG };

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
	afx_msg void OnBnClickedStartinsp();
	afx_msg void OnBnClickedUpdateimg();
	afx_msg void OnBnClickedGrab();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedExit();
	afx_msg void OnDestroy();
	BOOL m_UpdateImage;
	BOOL m_Acquisition;
};
