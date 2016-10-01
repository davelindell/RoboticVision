
// BaseballCatcherDlg.cpp : implementation file
//

#include "stdafx.h"
#include "BaseballCatcher.h"
#include "BaseballCatcherDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseballCatcherDlg dialog
CBaseballCatcherDlg::CBaseballCatcherDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBaseballCatcherDlg::IDD, pParent)
	, m_UpdateImage(FALSE)
	, m_Acquisition(FALSE)
	, m_PlayDelay(0)
	, m_CatchX(0)
	, m_CatchY(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CBaseballCatcherDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UPDATEIMG, m_UpdateImage);
	DDX_Radio(pDX, IDC_STOP, m_Acquisition);
	DDX_Text(pDX, IDC_SDELAY, m_PlayDelay);
	DDV_MinMaxInt(pDX, m_PlayDelay, 0, 2000);
	DDX_Text(pDX, IDC_CATCHX, m_CatchX);
	DDV_MinMaxDouble(pDX, m_CatchX, -9.0, 9.0);
	DDX_Text(pDX, IDC_CATCHY, m_CatchY);
	DDV_MinMaxDouble(pDX, m_CatchY, -8.0, 8.0);
}

BEGIN_MESSAGE_MAP(CBaseballCatcherDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LOADIMG, &CBaseballCatcherDlg::OnBnClickedLoadimg)
	ON_BN_CLICKED(IDC_SAVEIMG, &CBaseballCatcherDlg::OnBnClickedSaveimg)
	ON_BN_CLICKED(IDC_UPDATEIMG, &CBaseballCatcherDlg::OnBnClickedUpdateimg)
	ON_BN_CLICKED(IDC_STOP, &CBaseballCatcherDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_GRAB, &CBaseballCatcherDlg::OnBnClickedGrab)
	ON_BN_CLICKED(IDC_CAPTURE, &CBaseballCatcherDlg::OnBnClickedCapture)
	ON_BN_CLICKED(IDC_SHOWSEQ, &CBaseballCatcherDlg::OnBnClickedShowseq)
	ON_BN_CLICKED(IDC_CENTER, &CBaseballCatcherDlg::OnBnClickedCenter)
	ON_BN_CLICKED(IDC_MOVE, &CBaseballCatcherDlg::OnBnClickedMove)
	ON_BN_CLICKED(IDC_RESET, &CBaseballCatcherDlg::OnBnClickedReset)
	ON_BN_CLICKED(IDC_CATCH, &CBaseballCatcherDlg::OnBnClickedCatch)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDEXIT, &CBaseballCatcherDlg::OnBnClickedExit)
	ON_BN_CLICKED(IDC_LOAD_MOTOR, &CBaseballCatcherDlg::OnBnClickedLoadMotor)
END_MESSAGE_MAP()


// CBaseballCatcherDlg message handlers
BOOL CBaseballCatcherDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	VERIFY(m_Exit.AutoLoad(IDEXIT, this));
	WINDOWPLACEMENT lpwndpl;
	GetWindowPlacement(&lpwndpl);
	//lpwndpl.showCmd = SW_SHOWMAXIMIZED;
	//SetWindowPlacement(&lpwndpl);

	MainWnd = this;
	ImageDC[0] = GetDlgItem(IDC_IMAGE_DISPLAY1)->GetDC();
	ParentWnd[0] = GetDlgItem(IDC_IMAGE_DISPLAY1);
	ParentWnd[0]->GetWindowPlacement(&lpwndpl);
	ImageRect[0] = lpwndpl.rcNormalPosition;
	ParentWnd[0]->GetWindowRect(DispRect[0]);

	ImageDC[1] = GetDlgItem(IDC_IMAGE_DISPLAY2)->GetDC();
	ParentWnd[1] = GetDlgItem(IDC_IMAGE_DISPLAY2);
	ParentWnd[1]->GetWindowPlacement(&lpwndpl);
	ImageRect[1] = lpwndpl.rcNormalPosition;
	ParentWnd[1]->GetWindowRect(DispRect[1]);

	QSSysInit();
	m_UpdateImage = IR.UpdateImage;
	m_Acquisition = IR.Acquisition;
	m_PlayDelay = IR.PlayDelay;
	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBaseballCatcherDlg::OnPaint()
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
HCURSOR CBaseballCatcherDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBaseballCatcherDlg::OnBnClickedLoadimg()
{
	char	DirectoryName[128];
	char	Filter[128];
	sprintf_s(Filter, "Bitmap Image (*.bmp)|*.bmp|| | TIFF Document (*.tif)|*.tif");
	sprintf_s(DirectoryName, "%sImages\\*.bmp", APP_DIRECTORY);
	CFileDialog dlg(TRUE, NULL, CA2W(DirectoryName), OFN_PATHMUSTEXIST, CA2W(Filter), NULL);
	if (dlg.DoModal() == IDOK) {
		char	FileName[512], FileExt[512];
		CString WholeName;
		WholeName = dlg.GetPathName();
		strcpy_s(FileName, CT2A(WholeName.Left(WholeName.Find(L".") - 3)));
		strcpy_s(FileExt, CT2A(dlg.GetFileExt()));
		char	FullName[512];
		int i;
		for (i = 0; i<MAX_BUFFER; i++) {
			if (i<10)
				sprintf_s(FullName, "%sL0%d.%s", FileName, i, FileExt);
			else
				sprintf_s(FullName, "%sL%d.%s", FileName, i, FileExt);
			IR.SaveBuf[0][i] = imread(FullName, -1);
		}
		for (i = 0; i<MAX_BUFFER; i++) {
			if (i<10)
				sprintf_s(FullName, "%sR0%d.%s", FileName, i, FileExt);
			else
				sprintf_s(FullName, "%sR%d.%s", FileName, i, FileExt);
			IR.SaveBuf[1][i] = imread(FullName, -1);
		}
		QSSysDisplayImage();
	}
}

void CBaseballCatcherDlg::OnBnClickedSaveimg()
{
	char	DirectoryName[128];
	char	Filter[128];
	sprintf_s(Filter, "Bitmap Image (*.bmp)|*.bmp|| | TIFF Document (*.tif)|*.tif");
	sprintf_s(DirectoryName, "%sImages\\*.bmp", APP_DIRECTORY);
	CFileDialog dlg(FALSE, L"bmp", CA2W(DirectoryName), OFN_PATHMUSTEXIST, CA2W(Filter), NULL);
	if (dlg.DoModal() == IDOK) {
		char	FileName[512], FileExt[512];
		CString WholeName;
		WholeName = dlg.GetPathName();
		strcpy_s(FileName, CT2A(WholeName.Left(WholeName.Find(L"."))));
		strcpy_s(FileExt, CT2A(dlg.GetFileExt()));
		char	FullName[512];
		int i;
		for (i = 0; i<MAX_BUFFER; i++) {
			if (i<10)
				sprintf_s(FullName, "%sL0%d.%s", FileName, i, FileExt);
			else
				sprintf_s(FullName, "%sL%d.%s", FileName, i, FileExt);
			imwrite(FullName, IR.SaveBuf[0][i]);
		}
		for (i = 0; i<MAX_BUFFER; i++) {
			if (i<10)
				sprintf_s(FullName, "%sR0%d.%s", FileName, i, FileExt);
			else
				sprintf_s(FullName, "%sR%d.%s", FileName, i, FileExt);
			imwrite(FullName, IR.SaveBuf[1][i]);
		}
	}
}

void CBaseballCatcherDlg::OnBnClickedUpdateimg()
{
	UpdateData(TRUE);
	IR.UpdateImage = m_UpdateImage;
}

void CBaseballCatcherDlg::OnBnClickedStop()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = FALSE;
}

void CBaseballCatcherDlg::OnBnClickedGrab()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = TRUE;
}

void CBaseballCatcherDlg::OnBnClickedCapture()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = TRUE;
	UpdateData(FALSE);

	IR.PlayDelay = m_PlayDelay;
	IR.FrameID = 0;
	IR.CaptureSequence = TRUE;
}

void CBaseballCatcherDlg::OnBnClickedShowseq()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = FALSE;
	UpdateData(FALSE);

	IR.PlayDelay = m_PlayDelay;
	IR.FrameID = 0;
	IR.DisplaySequence = TRUE;
}

void CBaseballCatcherDlg::OnBnClickedCenter()
{
#ifdef USE_STAGE
	// Functions to move the catcher back to center
	SetMotorMode(X_MOTOR, CLOSED_LOOP_POS);
	SetMotorMode(Y_MOTOR, CLOSED_LOOP_POS);
	UpdateData(TRUE);
	Move(0.0, 0.0);
#endif
}

void CBaseballCatcherDlg::OnBnClickedMove()
{
#ifdef USE_STAGE
	// Functions to move the catcher
	SetMotorMode(X_MOTOR, CLOSED_LOOP_POS);
	SetMotorMode(Y_MOTOR, CLOSED_LOOP_POS);
	UpdateData(TRUE);
	Move(m_CatchX, m_CatchY);
#endif
}

void CBaseballCatcherDlg::OnBnClickedReset()
{
#ifdef USE_STAGE
	SetMotorMode(X_MOTOR, OPEN_LOOP_SP);
	SetMotorMode(Y_MOTOR, OPEN_LOOP_SP);
	SetSpeedOrPosition(X_MOTOR, 1000 * OPEN_LOOP_LIMIT);
	SetSpeedOrPosition(Y_MOTOR, -1000 * OPEN_LOOP_LIMIT);
	IsMotorStopped(Y_MOTOR);		
	IsMotorStopped(X_MOTOR);	
	SetEncoderCount(Y_MOTOR, LowCountLimit[Y_MOTOR]);
	SetEncoderCount(X_MOTOR, HighCountLimit[X_MOTOR]);
	SetMotorMode(Y_MOTOR, CLOSED_LOOP_POS);
	OnBnClickedCenter();
#endif
}

void CBaseballCatcherDlg::OnBnClickedLoadMotor()
{
	SetDeviceDefault();
}

void CBaseballCatcherDlg::OnBnClickedCatch()
{
	UpdateData(TRUE);
	IR.Acquisition = m_Acquisition = TRUE;
	UpdateData(FALSE);
	IR.CatchBall = ~IR.CatchBall;		// Toggle CatchBall every time the button is clicked.
	SetDlgItemText(IDC_CATCH, IR.CatchBall ? L"Stop Catch" : L"Catch Ball");
	if (IR.CatchBall) {
		SetMotorMode(X_MOTOR, CLOSED_LOOP_POS);
		SetMotorMode(Y_MOTOR, CLOSED_LOOP_POS);
	} else {
		SetMotorMode(X_MOTOR, OPEN_LOOP_SP);
		SetSpeedOrPosition(X_MOTOR, 0);
		SetMotorMode(Y_MOTOR, OPEN_LOOP_SP);
		SetSpeedOrPosition(Y_MOTOR, 0);
	}
}

void CBaseballCatcherDlg::OnDestroy()
{
	__super::OnDestroy();

	IR.Acquisition = FALSE;
	IR.UpdateImage = FALSE;
	// AfxMessageBox("Closing Point Grey");
	QSSysFree();
	UpdateData(TRUE);
}


void CBaseballCatcherDlg::OnBnClickedExit()
{
	CDialog::OnOK();
}
