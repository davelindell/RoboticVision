#include <windows.h>
#ifndef TCHW_H
#define TCHW_H

#ifdef PTGREY
#include "FlyCapture2.h"
using namespace FlyCapture2;
#endif

#include "opencv2/opencv.hpp"	// openCV headers
using namespace std;
using namespace cv;

#define PTG_COLOR
#define DEVELOPMENT

#define APP_DIRECTORY		"C:\\Projects\\VisualInspection\\"
#define	SHUTTER_SPEED		12.0					// Shutter speed in mSec
#define WHITE_BALANCE_R		560;
#define WHITE_BALANCE_B		740;
#define GAIN_VALUE_A		200;
#define GAIN_VALUE_B		0;
#ifdef PTG_COLOR
	#define VIDEO_FORMAT		VIDEOMODE_640x480RGB	// VIDEOMODE_640x480RGB for color or VIDEOMODE_640x480Y8 for B/W
	#define	CAMERA_FPS			FRAMERATE_30			// on 30 FPS for color camera
#else
	#define VIDEO_FORMAT		VIDEOMODE_640x480Y8	// VIDEOMODE_640x480RGB for color or VIDEOMODE_640x480Y8 for B/W
	#define	CAMERA_FPS			FRAMERATE_60			// or FRAMERATE_30
#endif

#define TRIGGER_ON			0						// Is the camera on an external trigger or not

#define MAX_CAMERA		1
#define MAX_BUFFER		4

#define SQUARE(a) ((a)*(a))
#define DISTANCE_SQ(x1,y1,x2,y2) (SQUARE((x1)-(x2))+SQUARE((y1)-(y2)))

#if defined(DEVELOPMENT) || defined(XLPORT) 
#define OPENF(txt) fopen_s(&m_fp, txt, "wt")
#define	FLUSHF _flushall()
#define CLOSEF fclose(m_fp)
#define DECLARE_F FILE *m_fp;
#else
#define OPENF(txt) 
#define	FLUSHF
#define CLOSEF 
#define DECLARE_F
#endif

#ifdef DEVELOPMENT
#define FTRACE(txt) fprintf(m_fp,txt)
#define FTDTRACE(txt) fprintf(pt->m_fp,txt)
#define FGTDTRACE(txt) fprintf(TP->m_fp,txt)
#else
#define FTRACE(txt) 
#define FTDTRACE(txt) 
#define FGTDTRACE(txt)
#endif

#ifdef XLPORT
#define XLTRACE(txt) fprintf(m_fp,txt)
#else
#define XLTRACE(txt) 
#endif

#define CMA ,

typedef struct
{
	int			DigSizeX;	
	int			DigSizeY;
	int			NumCameras;
	int			from_to[6];

	BOOL		Acquisition;
	BOOL		UpdateImage;
	BOOL		Inspection;
	// OpenCV Stuff
	Mat			AcqBuf[MAX_CAMERA];				// Processing Buffer in OpenCV
	Mat			DispBuf[MAX_CAMERA];			// Display Buffer in OpenCV
	Mat			DispROI[MAX_CAMERA];			// Display Buffer in OpenCV
	Mat			ProcBuf[MAX_CAMERA];			// rocess Buffer in OpenCV
	Mat			OutBuf1[MAX_CAMERA];			// Binary Process Buffer in OpenCV
	Mat			OutROI1[MAX_CAMERA];			// Binary Process Buffer in OpenCV
	Mat			OutBuf2[MAX_CAMERA];			// Binary Process Buffer in OpenCV
	Mat			OutROI2[MAX_CAMERA];			// Binary Process Buffer in OpenCV
	uchar*		AcqPtr[MAX_CAMERA];				// imageData pointer to be saved for later to release imageData
	Mat			OutBuf[3];						// vector of Mats for display

#ifdef PTGREY
	FC2Config	cameraConfig;				// Camera Configuration
	Property	cameraProperty;				// current camera property being queried or modified
	TriggerMode	cameraTrigger;				// Camera trigger mode settings
	BusManager	busMgr;
	PGRGuid		prgGuid[MAX_CAMERA];
	Camera*		pgrCamera[MAX_CAMERA];
	Image		PtGBuf[MAX_CAMERA];			// Array of most current images
#endif

} ImagingResources;		// Imaging Resources

class CTCSys 
{
protected:
	CWnd*	MainWnd;
	CWnd*	ParentWnd[MAX_CAMERA];	// Image Display Window
	CDC*	ImageDC[MAX_CAMERA];
	BOOL	EventEndProcess;		// Ends Program Execution
	HANDLE	QSProcessThreadHandle;
	HANDLE	QSProcessThreadHandleID;
	HANDLE	QSProcessEvent;
	DWORD	ExitCode;

	BITMAPINFO  m_bitmapInfo;

	CTCSys(void);
	~CTCSys(void);
	void	QSStartThread();
	void	QSStopThread();
	friend	static long	QSProcessThreadFunc(CTCSys *QS);
	void	initBitmapStruct(long iCols, long iRows);
	void	QSSysInit();
	void	QSSysFree();
	void	QSSysDisplayImage();
#ifdef PTGREY
	void	QSSysConvertToOpenCV(Mat* openCV_image, Image PGR_image);
#endif
	void	QSSysPrintResult(bool pass);
public:
	static  ImagingResources IR;
	RECT	ImageRect[MAX_CAMERA];
	CRect	DispRect[MAX_CAMERA];
	CRect	SelectRect;
	CDC*	SelectDC;			// Drawing Device Context
	CPen    DrawPen;			// CPEN variables
	CPen    ClearPen;			// CPEN variables
	DECLARE_F
};
#endif