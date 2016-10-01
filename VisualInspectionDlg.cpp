
// VisualInspectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "VisualInspection.h"
#include "VisualInspectionDlg.h"
#include "afxdialogex.h"
#include "math.h"
#include "string.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVisualInspectionDlg dialog



CVisualInspectionDlg::CVisualInspectionDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CVisualInspectionDlg::IDD, pParent)
	, m_UpdateImage(FALSE)
	, m_Acquisition(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVisualInspectionDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Check(pDX, IDC_UPDATEIMG, m_UpdateImage);
	DDX_Radio(pDX, IDC_STOP, m_Acquisition);
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CVisualInspectionDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOADIMG, &CVisualInspectionDlg::OnBnClickedLoadimg)
	ON_BN_CLICKED(IDC_SAVEIMG, &CVisualInspectionDlg::OnBnClickedSaveimg)
	ON_BN_CLICKED(IDC_STARTINSP, &CVisualInspectionDlg::OnBnClickedStartinsp)
	ON_BN_CLICKED(IDC_UPDATEIMG, &CVisualInspectionDlg::OnBnClickedUpdateimg)
	ON_BN_CLICKED(IDC_GRAB, &CVisualInspectionDlg::OnBnClickedGrab)
	ON_BN_CLICKED(IDC_STOP, &CVisualInspectionDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDEXIT, &CVisualInspectionDlg::OnBnClickedExit)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CVisualInspectionDlg message handlers

BOOL CVisualInspectionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	VERIFY(m_Exit.AutoLoad(IDEXIT, this));

	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(&lpwndpl);
	// lpwndpl.showCmd = SW_SHOWMAXIMIZED;
	// SetWindowPlacement(&lpwndpl);

	// Added for Display Using DIB
	MainWnd = this;
	ImageDC[0] = GetDlgItem(IDC_DISPLAY1)->GetDC();
	ParentWnd[0] = GetDlgItem(IDC_DISPLAY1);
	ParentWnd[0]->GetWindowPlacement(&lpwndpl);
	ImageRect[0] = lpwndpl.rcNormalPosition;
	ParentWnd[0]->GetWindowRect(DispRect[0]);
	QSSysInit();
	m_UpdateImage = IR.UpdateImage;
	m_Acquisition = IR.Acquisition;
	UpdateData(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVisualInspectionDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVisualInspectionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CVisualInspectionDlg::OnBnClickedLoadimg()
{
	OnBnClickedStop();
	char	DirectoryName[128];
	char	Filter[128];
	sprintf_s(Filter, "Bitmap Image (*.bmp)|*.bmp|| | TIFF Document (*.tif)|*.tif");
	sprintf_s(DirectoryName, "%sImages\\*.bmp", APP_DIRECTORY);
	CFileDialog dlg(TRUE, NULL, CA2W(DirectoryName), OFN_PATHMUSTEXIST, CA2W(Filter), NULL);
	if (dlg.DoModal() == IDOK) {
		char	FileName[512];
		strcpy_s(FileName, CT2A(dlg.GetPathName()));
		//AfxMessageBox(FileName);
		IR.ProcBuf[0] = imread(FileName, -1);
		IR.ProcBuf[0].copyTo(IR.DispBuf[0]);
		QSSysDisplayImage(); 
	}
}


void CVisualInspectionDlg::OnBnClickedSaveimg()
{
	OnBnClickedStop();
	char	DirectoryName[128];
	char	Filter[128];
	sprintf_s(Filter, "Bitmap Image (*.bmp)|*.bmp|| | TIFF Document (*.tif)|*.tif");
	sprintf_s(DirectoryName, "%sImages\\*.bmp", APP_DIRECTORY);
	CFileDialog dlg(FALSE, L"bmp", CA2W(DirectoryName), OFN_PATHMUSTEXIST, CA2W(Filter), NULL);
	if (dlg.DoModal() == IDOK) {
		char	FileName[512];
		strcpy_s(FileName, CT2A(dlg.GetPathName()));
		imwrite(FileName, IR.DispBuf[0]);
	}
}


void CVisualInspectionDlg::OnBnClickedStartinsp()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = TRUE;
	UpdateData(FALSE);
	IR.Inspection = ~IR.Inspection;
}


void CVisualInspectionDlg::OnBnClickedUpdateimg()
{
	UpdateData(TRUE);
	IR.UpdateImage = m_UpdateImage;
}


void CVisualInspectionDlg::OnBnClickedGrab()
{
	IR.Acquisition = m_Acquisition = TRUE;
	UpdateData(FALSE);
}


void CVisualInspectionDlg::OnBnClickedStop()
{
	IR.Acquisition = m_Acquisition = FALSE;
	UpdateData(FALSE);
}

void CVisualInspectionDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	IR.Acquisition = FALSE;
	IR.UpdateImage = FALSE;
	QSSysFree();
}

void CVisualInspectionDlg::OnBnClickedExit()
{
	CDialog::OnOK();
}