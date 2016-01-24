#include "stdafx.h"
#include "time.h"
#include "math.h"
#include "Hardware.h"
#include "Mnm.h"

#define MEH 2
#define GOOD 1
#define BAD 0
#define UGLY -1


ImagingResources	CTCSys::IR;

CTCSys::CTCSys()
{
	EventEndProcess = TRUE;
	IR.Acquisition = TRUE;
	IR.UpdateImage = TRUE;
	IR.Inspection = FALSE;
	OPENF("c:\\Projects\\RunTest.txt");
}

CTCSys::~CTCSys()
{
	CLOSEF;
}

void CTCSys::QSStartThread()
{
	EventEndProcess = FALSE;
	//QSProcessEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	// Image Processing Thread
	QSProcessThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSProcessThreadFunc,
		this, NULL, (LPDWORD) &QSProcessThreadHandleID);
	ASSERT(QSProcessThreadHandle != NULL);
	SetThreadPriority(QSProcessThreadHandle, THREAD_PRIORITY_HIGHEST);
}

void CTCSys::QSStopThread()
{
	// need to make sure camera acquisiton has stopped
	EventEndProcess = TRUE;
	do { 
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSProcessThreadHandle);
}

long QSProcessThreadFunc(CTCSys *QS)
{
// Some variables for loading test images
#ifndef PTGRAY
	unsigned int im_i = 0;
	char im_path[100];
#endif

	//namedWindow("Lindell", WINDOW_AUTOSIZE);
	Mnm mnm = Mnm();
	

	int		i;
	int	pass = GOOD;
	while (QS->EventEndProcess == FALSE) {

#ifdef PTGREY
		if (QS->IR.Acquisition == TRUE) {
			for(i=0; i < QS->IR.NumCameras; i++) {
				if (QS->IR.pgrCamera[i]->RetrieveBuffer(&QS->IR.PtGBuf[i]) == PGRERROR_OK) {
					QS->QSSysConvertToOpenCV(&QS->IR.AcqBuf[i], QS->IR.PtGBuf[i]);	
				}
			}
			for(i=0; i < QS->IR.NumCameras; i++) {
#ifdef PTG_COLOR
				mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.ProcBuf[i], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
				QS->IR.AcqBuf[i].copyTo(QS->IR.ProcBuf[i][BufID]);	// Has to copy out of acquisition buffer before processing
#endif
			}
		}
#else
		Sleep (200);
#endif

		// Process Image ProcBuf
		if (QS->IR.Inspection) {
			// Images are acquired into ProcBuf{0] 
			// May need to create child image for processing to exclude background and speed up processing

#ifndef PTGREY
			// copy image to ProcBuf
			sprintf(im_path, "images\\%d.bmp", im_i % 61);
			QS->IR.ProcBuf[0] = imread(im_path, CV_LOAD_IMAGE_COLOR);

			if (!QS->IR.ProcBuf[0].data) {
				cout << "ERROR! Could not read image file." << std::endl;
				im_i++;
				continue;
			}

			im_i++;
#endif

			for(i=0; i < QS->IR.NumCameras; i++) {
				// Example using Canny.  Input is ProcBuf.  Output is OutBuf1
				//cvtColor(QS->IR.ProcBuf[i], QS->IR.OutBuf1[i], CV_RGB2GRAY);
				// Canny(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], 70, 100);
				QS->IR.ProcBuf[i].copyTo(QS->IR.OutBuf1[i]);

			}

		}
		// Display Image
		if (QS->IR.UpdateImage) {
			for (i=0; i<QS->IR.NumCameras; i++) {
				if (!QS->IR.Inspection) {
					// Example of displaying color buffer ProcBuf
					QS->IR.ProcBuf[i].copyTo(QS->IR.DispBuf[i]);
				} else {					
					// TODO: Insert processing code here

					// make some copies
					QS->IR.OutBuf1[i].copyTo(QS->IR.OutROI1[i]);

					// isolate the conveyer belt
					Rect leftROI(0, 0, 130, 480);
					Rect rightROI(510, 0, 130, 480);
					Mat mask = QS->IR.OutBuf1[i](leftROI);
					mask.setTo(0);
					mask = QS->IR.OutBuf1[i](rightROI);
					mask.setTo(0);

					// blur the image
					GaussianBlur(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], Size(15, 15), 11, 0);

					//QS->IR.OutROI1[i].setTo(Scalar(0, 0, 255));

					// convert to HSV colorspace
					cvtColor(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], CV_RGB2HSV);
					cvtColor(QS->IR.OutROI1[i], QS->IR.OutROI1[i], CV_RGB2HSV);

					// threshold intense colors
					inRange(QS->IR.OutBuf1[i], Scalar(0, 60, 60), Scalar(255, 255, 255), QS->IR.OutBuf1[i]);
					
					// separate HSV channels
					vector<Mat> hsv_planes;
					split(QS->IR.OutROI1[i], hsv_planes);

					// Find avg hue of remaining area
					Scalar mean_value = mean(hsv_planes[0], QS->IR.OutBuf1[i]);
					float avg_hue = mean_value.val[0];

					Scalar mean_mat, std_mat;
					meanStdDev(hsv_planes[0], mean_mat, std_mat, QS->IR.OutBuf1[i]);
					float std_dev = std_mat.val[0];

					//bitwise_not(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i]);

					// get color and write on image
					Mnm::Color color = Mnm::getColor(avg_hue);
					char text[32];
					if (color == Mnm::blue) {
						sprintf(text, "blue");
						pass = GOOD;
					}
					else if (color == Mnm::red) {
						sprintf(text, "red");
						pass = BAD;
					}
					else if (color == Mnm::green) {
						sprintf(text, "green");
						pass = BAD;
					}
					else if (color == Mnm::yellow) {
						sprintf(text, "yellow");
						pass = BAD;
					}
					else {
						sprintf(text, "unrecognized");
						pass = MEH;
					}

					// then our color has a lot of variation, check if broken
					if (std_dev > 20) {
						QS->IR.OutBuf1[i].copyTo(QS->IR.OutROI1[i]);
						bitwise_not(QS->IR.OutROI1[i], QS->IR.OutROI1[i]);

						SimpleBlobDetector::Params params;

						// Change thresholds
						params.minThreshold = 0;
						params.maxThreshold = 255;

						// Filter by Area.
						params.filterByArea = true;
						params.minArea = 1200;

						// Filter by Circularity
						params.filterByCircularity = true;
						params.minCircularity = 0.5;

						// Filter by Convexity
						params.filterByConvexity = true;
						params.minConvexity = .9;

						// Filter by Inertia
						params.filterByInertia = true;
						params.minInertiaRatio = 0.5;
						
						Ptr<SimpleBlobDetector> d = SimpleBlobDetector::create(params);
						vector<KeyPoint> keypoints;
						d->detect(QS->IR.OutROI1[i], keypoints);

						drawKeypoints(QS->IR.OutROI1[i], keypoints, QS->IR.OutROI1[i], Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

						//namedWindow("Contours", CV_WINDOW_AUTOSIZE);
						//imshow("Contours", QS->IR.OutROI1[i]);
						//waitKey(0);

						//Canny(QS->IR.OutROI1[i], QS->IR.OutROI1[i], 0, 255, 3);

						//vector<Vec3f> circles;

						///// Apply the Hough Transform to find the circles
						//HoughCircles(QS->IR.OutROI1[i], circles, CV_HOUGH_GRADIENT, 1, QS->IR.OutROI1[i].rows / 8, 255, 70, 0, 0);

						//cvtColor(QS->IR.OutROI1[i], QS->IR.OutROI1[i], CV_GRAY2BGR);

						///// Draw the circles detected
						//for (size_t i = 0; i < circles.size(); i++)
						//{
						//	Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
						//	int radius = cvRound(circles[i][2]);
						//	// circle center
						//	circle(QS->IR.OutROI1[i], center, 3, Scalar(0, 255, 0), -1, 8, 0);
						//	// circle outline
						//	circle(QS->IR.OutROI1[i], center, radius, Scalar(0, 0, 255), 3, 8, 0);
						//}
						///// Show your results
						//namedWindow("Hough Circle Transform Demo", CV_WINDOW_AUTOSIZE);
						//imshow("Hough Circle Transform Demo", QS->IR.OutROI1[i]);
						//waitKey(0);

						//vector<vector<Point> > contours;
						//vector<Vec4i> hierarchy;
						//RNG rng(12345);
						////QS->IR.OutBuf1[i].copyTo(QS->IR.OutROI1[i]);
						//findContours(QS->IR.OutROI1[i], contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

						//Mat drawing = Mat::zeros(QS->IR.OutROI1[i].size(), CV_8UC3);
						//for (int k = 0; k < contours.size(); k++)
						//{
						//	Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
						//	drawContours(drawing, contours, k, color, 2, 8, hierarchy, 0, Point());
						//}

						///// Get the moments
						//vector<Moments> mu(contours.size());
						//for (int k = 0; k < contours.size(); k++)
						//{
						//	mu[k] = moments(contours[k], false);
						//}

						/////  Get the mass centers:
						//vector<Point2f> mc(contours.size());
						//for (int k = 0; k < contours.size(); k++)
						//{
						//	mc[k] = Point2f(mu[k].m10 / mu[k].m00, mu[k].m01 / mu[k].m00);
						//}

						// Show in a window
						//namedWindow("Contours", CV_WINDOW_AUTOSIZE);
						//imshow("Contours", drawing);
						//waitKey(0);
						
						//if(mc.size() > 1){
						//		sprintf(text, "broken");
						//		pass = UGLY;
						//}

						//vector<Point2f> corner_pts;
						//double qualityLevel = 0.01;
						//double minDistance = 10;
						//bool useHarrisDetector = false;
						//int maxCorners = 50;
						//int blockSize = 2;
						//double k = 0.04;
						//Size winSize = Size(5, 5);
						//Size zeroZone = Size(-1, -1);
						//TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);

						///// Apply corner detection
						//goodFeaturesToTrack(QS->IR.OutROI1[i],
						//	corner_pts,
						//	maxCorners,
						//	qualityLevel,
						//	minDistance,
						//	Mat(),
						//	blockSize,
						//	useHarrisDetector,
						//	k);

						//cornerSubPix(QS->IR.OutROI1[i], corner_pts, winSize, zeroZone, criteria);
						//cvtColor(QS->IR.OutROI1[i], QS->IR.OutROI1[i], CV_GRAY2RGB);

						///// Drawing a circle around corners
						//for (unsigned k = 0; k < corner_pts.size(); k++){
						//	circle(QS->IR.OutROI1[i], Point(corner_pts.at(k).x, corner_pts.at(k).y), 5, Scalar(0, 0, 250), 2, 8, 0);
						//}

						//namedWindow("Contours", CV_WINDOW_AUTOSIZE);
						//imshow("Contours", QS->IR.OutROI1[i]);
						//waitKey(0);

					}

					// have output image show just M&M
					QS->IR.OutBuf1[i].copyTo(mask);
					bitwise_and(mask, hsv_planes[0], hsv_planes[0]);
					bitwise_and(mask, hsv_planes[1], hsv_planes[1]);
					bitwise_and(mask, hsv_planes[2], hsv_planes[2]);
					merge(hsv_planes, QS->IR.OutBuf1[i]);
					cvtColor(QS->IR.OutBuf1[i], QS->IR.OutBuf1[i], CV_HSV2RGB);
					putText(QS->IR.OutBuf1[i], text, Point(400, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
					sprintf(text, "%.3f", std_dev);
					putText(QS->IR.OutBuf1[i], text, Point(30, 70), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);

					// Copy output to display buffer
					QS->IR.OutBuf1[i].copyTo(QS->IR.DispBuf[i]);
					QS->QSSysPrintResult(pass);
				}
			}
			QS->QSSysDisplayImage();
		}
	} 
	QS->EventEndProcess = FALSE;
	return 0;
}

void CTCSys::QSSysInit()
{
	long i;
	IR.DigSizeX = 640;
	IR.DigSizeY = 480;
	initBitmapStruct(IR.DigSizeX, IR.DigSizeY);
	// Camera Initialization
#ifdef PTGREY
	IR.cameraConfig.asyncBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.isochBusSpeed = BUSSPEED_S800;
	IR.cameraConfig.grabMode = DROP_FRAMES;			// take the last one, block grabbing, same as flycaptureLockLatest
	IR.cameraConfig.grabTimeout = TIMEOUT_INFINITE;	// wait indefinitely
	IR.cameraConfig.numBuffers = 4;					// really does not matter since DROP_FRAMES is set not to accumulate buffers

	// How many cameras are on the bus?
	if(IR.busMgr.GetNumOfCameras((unsigned int *)&IR.NumCameras) != PGRERROR_OK){	// something didn't work correctly - print error message
		AfxMessageBox(L"Connect Failure", MB_ICONSTOP);
	} else {
		IR.NumCameras = (IR.NumCameras > MAX_CAMERA) ? MAX_CAMERA : IR.NumCameras;
		for(i = 0; i < IR.NumCameras; i++) {		
			// Get PGRGuid
			if (IR.busMgr.GetCameraFromIndex(0, &IR.prgGuid[i]) != PGRERROR_OK) {
				AfxMessageBox(L"PGRGuID Failure", MB_ICONSTOP);
			}
			IR.pgrCamera[i] = new Camera;
			if (IR.pgrCamera[i]->Connect(&IR.prgGuid[i]) != PGRERROR_OK) { 
				AfxMessageBox(L"PConnect Failure", MB_ICONSTOP);
			}
			// Set all camera configuration parameters
			if (IR.pgrCamera[i]->SetConfiguration(&IR.cameraConfig) != PGRERROR_OK) { 
				AfxMessageBox(L"Set Configuration Failure", MB_ICONSTOP);
			}
			// Set video mode and frame rate
			if (IR.pgrCamera[i]->SetVideoModeAndFrameRate(VIDEO_FORMAT, CAMERA_FPS) != PGRERROR_OK) { 
				AfxMessageBox(L"Video Format Failure", MB_ICONSTOP);
			}
			// Sets the onePush option off, Turns the control on/off on, disables auto control.  These are applied to all properties.
			IR.cameraProperty.onePush = false;
			IR.cameraProperty.autoManualMode = false;
			IR.cameraProperty.absControl = true;
			IR.cameraProperty.onOff = true;
			// Set shutter sppeed
			IR.cameraProperty.type = SHUTTER;
			IR.cameraProperty.absValue = SHUTTER_SPEED;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				AfxMessageBox(L"Shutter Failure", MB_ICONSTOP);
			}
#ifdef  PTG_COLOR
			// Set white balance (R and B values)
			IR.cameraProperty = WHITE_BALANCE;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = WHITE_BALANCE_R;
			IR.cameraProperty.valueB = WHITE_BALANCE_B;
//			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
//				AfxMessageBox(L"White Balance Failure", MB_ICONSTOP);
//			}
#endif
			// Set gain values (350 here gives 12.32dB, varies linearly)
			IR.cameraProperty = GAIN;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = GAIN_VALUE_A;
			IR.cameraProperty.valueB = GAIN_VALUE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				AfxMessageBox(L"Gain Failure", MB_ICONSTOP);
			}
			// Set trigger state
			IR.cameraTrigger.mode = 0;
			IR.cameraTrigger.onOff = TRIGGER_ON;
			IR.cameraTrigger.polarity = 0;
			IR.cameraTrigger.source = 0;
			IR.cameraTrigger.parameter = 0;
			if(IR.pgrCamera[i]->SetTriggerMode(&IR.cameraTrigger, false) != PGRERROR_OK){	
				AfxMessageBox(L"Trigger Failure", MB_ICONSTOP);
			}
			// Start Capture Individually
			if (IR.pgrCamera[i]->StartCapture() != PGRERROR_OK) {
				char Msg[128];
				sprintf_s(Msg, "Start Capture Camera %d Failure", i);
				AfxMessageBox(CA2W(Msg), MB_ICONSTOP);
			}
		}
		// Start Sync Capture (only need to do it with one camera)
//		if (IR.pgrCamera[0]->StartSyncCapture(IR.NumCameras, (const Camera**)IR.pgrCamera, NULL, NULL) != PGRERROR_OK) {
//			AfxMessageBox(L"Start Sync Capture Failure", MB_ICONSTOP);
//		}
	}
#else
	IR.NumCameras = MAX_CAMERA;
#endif
	Rect R = Rect(0, 0, IR.DigSizeX, IR.DigSizeY);
	// create openCV image
	for(i=0; i<IR.NumCameras; i++) {
#ifdef PTG_COLOR
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
#else 
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][0].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i][1].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
#endif
		IR.AcqPtr[i] = IR.AcqBuf[i].data;
		IR.DispROI[i] = IR.DispBuf[i](R); 

		IR.OutBuf1[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI1[i] = IR.OutBuf1[i](R); 
		IR.OutBuf2[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI2[i] = IR.OutBuf2[i](R); 
		IR.DispBuf[i] = Scalar(0);
		IR.ProcBuf[i] = Scalar(0);
	}
	IR.from_to[0] = 0;
	IR.from_to[1] = 2;
	IR.from_to[2] = 1;
	IR.from_to[3] = 1;
	IR.from_to[4] = 2;
	IR.from_to[5] = 0;
	QSStartThread();
}

void CTCSys::QSSysFree()
{
	QSStopThread(); // Move to below PTGREY if on Windows Vista
#ifdef PTGREY
	for(int i=0; i<IR.NumCameras; i++) {
		if (IR.pgrCamera[i]) {
			IR.pgrCamera[i]->StopCapture();
			IR.pgrCamera[i]->Disconnect();
			delete IR.pgrCamera[i];
		}
	}
#endif
}

void CTCSys::initBitmapStruct(long iCols, long iRows)
{
	m_bitmapInfo.bmiHeader.biSize			= sizeof( BITMAPINFOHEADER );
	m_bitmapInfo.bmiHeader.biPlanes			= 1;
	m_bitmapInfo.bmiHeader.biCompression	= BI_RGB;
	m_bitmapInfo.bmiHeader.biXPelsPerMeter	= 120;
	m_bitmapInfo.bmiHeader.biYPelsPerMeter	= 120;
    m_bitmapInfo.bmiHeader.biClrUsed		= 0;
    m_bitmapInfo.bmiHeader.biClrImportant	= 0;
    m_bitmapInfo.bmiHeader.biWidth			= iCols;
    m_bitmapInfo.bmiHeader.biHeight			= -iRows;
    m_bitmapInfo.bmiHeader.biBitCount		= 24;
	m_bitmapInfo.bmiHeader.biSizeImage = 
      m_bitmapInfo.bmiHeader.biWidth * m_bitmapInfo.bmiHeader.biHeight * (m_bitmapInfo.bmiHeader.biBitCount / 8 );
}

void CTCSys::QSSysDisplayImage()
{
	SetDIBitsToDevice(ImageDC[0]->GetSafeHdc(), 1, 1,
		m_bitmapInfo.bmiHeader.biWidth,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		0, 0, 0,
		::abs(m_bitmapInfo.bmiHeader.biHeight),
		IR.DispBuf[0].data,
		&m_bitmapInfo, DIB_RGB_COLORS);
}


#ifdef PTGREY
void CTCSys::QSSysConvertToOpenCV(Mat* openCV_image, Image PGR_image)
{
	openCV_image->data = PGR_image.GetData();	// Pointer to image data
	openCV_image->cols = PGR_image.GetCols();	// Image width in pixels
	openCV_image->rows = PGR_image.GetRows();	// Image height in pixels
	openCV_image->step = PGR_image.GetStride(); // Size of aligned image row in bytes
}
#endif

void CTCSys::QSSysPrintResult(int pass)
{
	char text[32];
	if (pass == 2){
		strcpy(text, "Meh");
	}
	else if (pass == 1){
		strcpy(text, "Good");
	}
	else if (pass == 0) {
		strcpy(text, "Bad");
	}
	else {
		strcpy(text, "Ugly");
	}
	putText(IR.DispBuf[0], text, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
}
