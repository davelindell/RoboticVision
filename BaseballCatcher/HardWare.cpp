#include "stdafx.h"
#include "time.h"
#include "math.h"
#include "Hardware.h"
#include <fstream>

ImagingResources	CTCSys::IR;

struct KeyPointCompare {
	bool operator() (KeyPoint i, KeyPoint j) { return (i.size>j.size); }
} keyPointCompare;

CTCSys::CTCSys()
{
	EventEndProcess = TRUE;
	EventEndMove = TRUE;
	IR.Acquisition = TRUE;
	IR.UpdateImage = TRUE;
	IR.CaptureSequence = FALSE;
	IR.DisplaySequence = FALSE;
	IR.PlayDelay = 30;
	IR.CaptureDelay = 30;
	IR.FrameID = 0;
	IR.CatchBall = FALSE;
	OPENF("c:\\Projects\\RunTest.txt");
}

CTCSys::~CTCSys()
{
	CLOSEF;
}

void CTCSys::QSStartThread()
{
	EventEndProcess = FALSE;
	EventEndMove = FALSE;
	QSMoveEvent = CreateEvent(NULL, TRUE, FALSE, NULL);		// Create a manual-reset and initially nosignaled event handler to control move event
	ASSERT(QSMoveEvent != NULL);

	// Image Processing Thread
	QSProcessThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSProcessThreadFunc,
		this, NULL, (LPDWORD) &QSProcessThreadHandleID);
	ASSERT(QSProcessThreadHandle != NULL);
	SetThreadPriority(QSProcessThreadHandle, THREAD_PRIORITY_HIGHEST);

	QSMoveThreadHandle = CreateThread(NULL, 0L,
		(LPTHREAD_START_ROUTINE)QSMoveThreadFunc,
		this, NULL, (LPDWORD) &QSMoveThreadHandleID);
	ASSERT(QSMoveThreadHandle != NULL);
	SetThreadPriority(QSMoveThreadHandle, THREAD_PRIORITY_HIGHEST);
}

void CTCSys::QSStopThread()
{
	// Must close the move event first
	EventEndMove = TRUE;				// Set the falg to true first
	SetEvent(QSMoveEvent);				// must set event to complete the while loop so the flag can be checked
	do { 
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSMoveThreadHandle);

	// need to make sure camera acquisiton has stopped
	EventEndProcess = TRUE;
	do { 
		Sleep(100);
		// SetEvent(QSProcessEvent);
	} while(EventEndProcess == TRUE);
	CloseHandle(QSProcessThreadHandle);
}

long QSMoveThreadFunc(CTCSys *QS)
{
	while (QS->EventEndMove == FALSE) {
		WaitForSingleObject(QS->QSMoveEvent, INFINITE);
		if (QS->EventEndMove == FALSE) QS->Move(QS->Move_X, QS->Move_Y);
		ResetEvent(QS->QSMoveEvent);
	}
	QS->EventEndMove = FALSE;
	return 0;
}


long QSProcessThreadFunc(CTCSys *QS)
{
	//namedWindow("debug", WINDOW_AUTOSIZE);

	int     i;
	int     BufID = 0;
	int		xl = 310;
	int		xr = 220;
	int		yt = 0;
	int		width = 130;
	int		height = 280;
	int		wf = 5;
	float	xoffset = 13;
	float	yoffset = 9;
	char    str[32];
    long	FrameStamp;
	int num_updates = 0;

	vector<Point2f> l_points, r_points, l_points_ctd, r_points_ctd;
	vector<Point3f> l_3dpoints;
    FrameStamp = 0;

	char path[100];
	int im_i = 20;

	Mat calib[2], roi[2], roi_diff[2];
	bool calibrated = false;
	bool detected_ball = false;
	bool first_ball = true;
	vector<Point2f> l_prev_ball_location(1);
	vector<Point2f> r_prev_ball_location(1);

	// read calibration params
	// read yml file
	FileStorage L_fs("cal\\l_camera.yml", FileStorage::READ);
	FileStorage R_fs("cal\\r_camera.yml", FileStorage::READ);
	FileStorage fs("cal\\rectify.yml", FileStorage::READ);
	FileStorage fshill("cal\\parameters.yml", FileStorage::READ);

	Mat L_camera_matrix, L_dist_coeffs;
	Mat R_camera_matrix, R_dist_coeffs;
	Mat R1, R2, P1, P2, Q;

	/*L_fs["cameraMatrix"] >> L_camera_matrix;
	L_fs["distCoeffs"] >> L_dist_coeffs;
	R_fs["cameraMatrix"] >> R_camera_matrix;
	R_fs["distCoeffs"] >> R_dist_coeffs;
	fs["R1"] >> R1;
	fs["R2"] >> R2;
	fs["P1"] >> P1;
	fs["P2"] >> P2;
	fs["Q"] >> Q;*/

	fshill["intrinsicL"] >> L_camera_matrix;
	fshill["distortionL"] >> L_dist_coeffs;
	fshill["intrinsicR"] >> R_camera_matrix;
	fshill["distortionR"] >> R_dist_coeffs;
	fshill["R1"] >> R1;
	fshill["R2"] >> R2;
	fshill["P1"] >> P1;
	fshill["P2"] >> P2;
	fshill["Q"] >> Q;

	fs.release();
	L_fs.release();
	R_fs.release();
	fshill.release();

	// Initialize the catcher.  QS->moveX and QS->moveY (both in inches) must be calculated and set first.
	QS->Move_X = 0;					// replace 0 with your x coordinate
	QS->Move_Y = 0;					// replace 0 with your y coordinate
	SetEvent(QS->QSMoveEvent);		// Signal the move event to move catcher. The event will be reset in the move thread.

	// initialize blob detector params
	SimpleBlobDetector::Params params;

	// Change thresholds
	params.minThreshold = 100;
	params.maxThreshold = 255;
	params.filterByArea = true;
	params.minArea = 10;
	params.maxArea = 500;
	params.filterByCircularity = false;
	params.minCircularity = 0.01;
	params.filterByConvexity = false;
	params.minConvexity = 0.01;
	params.filterByInertia = false;
	params.minInertiaRatio = 0.01;


	while (QS->EventEndProcess == FALSE) {
#ifdef PTGREY		// Image Acquisition
		if (QS->IR.Acquisition == TRUE) {
			for(i=0; i < QS->IR.NumCameras; i++) {
				QS->IR.PGRError = QS->IR.pgrCamera[i]->RetrieveBuffer(&QS->IR.PtGBuf[i]);
				// Get frame timestamp if exact frame time is needed.  Divide FrameStamp by 32768 to get frame time stamp in mSec
                QS->IR.metaData[i] = QS->IR.PtGBuf[i].GetMetadata();
				FrameStamp = QS->IR.metaData[i].embeddedTimeStamp;               
				if(QS->IR.PGRError == PGRERROR_OK){
					QS->QSSysConvertToOpenCV(&QS->IR.AcqBuf[i], QS->IR.PtGBuf[i]);		// copy image data pointer to OpenCV Mat structure
				}
			}
			for(i=0; i < QS->IR.NumCameras; i++) {
				if (QS->IR.CaptureSequence) {
#ifdef PTG_COLOR
					mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.SaveBuf[i][QS->IR.FrameID], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
					QS->IR.AcqBuf[i].copyTo(QS->IR.SaveBuf[i][QS->IR.FrameID]);
#endif
				} else {
#ifdef PTG_COLOR
					mixChannels(&QS->IR.AcqBuf[i], 1, &QS->IR.ProcBuf[i][BufID], 1, QS->IR.from_to, 3); // Swap B and R channels anc=d copy out the image at the same time.
#else
					QS->IR.AcqBuf[i].copyTo(QS->IR.ProcBuf[i]);	// Has to be copied out of acquisition buffer before processing
#endif
				}
			}
		}
#else
		// load in training images
		sprintf(path, "im\\Ball8L%02d.bmp", im_i);
		Mat l_im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
		sprintf(path, "im\\Ball8R%02d.bmp", im_i);
		Mat r_im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		if (!l_im.data || !r_im.data) {
			printf("Error reading images\n");
			return -1;
		}

		l_im.copyTo(QS->IR.ProcBuf[0]);
		r_im.copyTo(QS->IR.ProcBuf[1]);


#endif
		// Process Image ProcBuf
		if (QS->IR.CatchBall) {  	// Click on "Catch" button to toggle the CatchBall flag when done catching
			// Images are acquired into ProcBuf[0] for left and ProcBuf[1] for right camera
			// Need to create child image or small region of interest for processing to exclude background and speed up processing
			// Mat child = QS->IR.ProcBuf[i](Rect(x, y, width, height));
#ifdef PTG_COLOR
			cvtColor(QS->IR.ProcBuf[i][BufID], QS->IR.OutBuf1[i], CV_RGB2GRAY, 0);
#else			

#endif
			// use first image as calibration image
			if (!calibrated) {
				QS->IR.ProcBuf[0](Rect(xl, yt, width, height)).copyTo(calib[0]);
				QS->IR.ProcBuf[1](Rect(xr, yt, width, height)).copyTo(calib[1]);
				calibrated = true;

				QS->Move_X = 0;					// replace 0 with your x coordinate
				QS->Move_Y = 0;					// replace 0 with your y coordinate
				SetEvent(QS->QSMoveEvent);		// Signal the move event to move catcher. The event will be reset in the move thread.

			}

			// identify ROI in L and R cameras
			roi[0] = QS->IR.ProcBuf[0](Rect(xl, yt, width, height));
			roi[1] = QS->IR.ProcBuf[1](Rect(200, yt, width, height));

			// use difference from l_calib/r_calib to find if ball is there
			absdiff(roi[0], calib[0], roi_diff[0]);
			absdiff(roi[1], calib[1], roi_diff[1]);

			QS->IR.ProcBuf[0].copyTo(QS->IR.OutBuf[0]);
			QS->IR.ProcBuf[1].copyTo(QS->IR.OutBuf[1]);

#ifndef PTGREY
			im_i++;
#endif

			// use difference images to detect if ball is there
			// threshold image
			int th = 15;
			inRange(roi_diff[0], Scalar(0, 0, 0), Scalar(th, th, th), roi_diff[0]);
			inRange(roi_diff[1], Scalar(0, 0, 0), Scalar(th, th, th), roi_diff[1]);

			// blob detect
			Ptr<SimpleBlobDetector> d = SimpleBlobDetector::create(params);
			vector<KeyPoint> l_keypoints, r_keypoints;
			d->detect(roi_diff[0], l_keypoints);
			d->detect(roi_diff[1], r_keypoints);

			// check to see if we found any blobs
			if (l_keypoints.size() < 1 || r_keypoints.size() < 1){
				continue;
			}

			detected_ball = true;



			// sort by blob size
			sort(l_keypoints.begin(), l_keypoints.end(), keyPointCompare);
			sort(r_keypoints.begin(), r_keypoints.end(), keyPointCompare);

			// track ball location, constrain to nearest to prev ball
			//if (first_ball) {
			//	l_prev_ball_location[0].x = l_keypoints[0].pt.x;
			//	l_prev_ball_location[0].y = l_keypoints[0].pt.y;
			//	r_prev_ball_location[0].x = r_keypoints[0].pt.x;
			//	r_prev_ball_location[0].y = r_keypoints[0].pt.y;
			//	first_ball = false;
			//}
			//else {
			//	float cur_x;
			//	float cur_y;
			//	float cur_dist;
			//	float prev_x = l_prev_ball_location[0].x;
			//	float prev_y = l_prev_ball_location[0].y;
			//	float min_dist = INT_MAX;
			//	int min_dist_ind = -1;
			//	// left 
			//	for (int j = 0; j < l_keypoints.size(); j++) {
			//		cur_x = l_keypoints[j].pt.x;
			//		cur_y = l_keypoints[j].pt.y;
			//		cur_dist = sqrt(pow((prev_x - cur_x), 2) + pow((prev_y - cur_y), 2));

			//		if (cur_dist < min_dist) {
			//			min_dist = cur_dist;
			//			min_dist_ind = j;
			//		}
			//	}
			//	l_keypoints[0] = l_keypoints[min_dist_ind];

			//	// right
			//	prev_x = r_prev_ball_location[0].x;
			//	prev_y = r_prev_ball_location[0].y;
			//	min_dist = INT_MAX;
			//	min_dist_ind = -1;
			//	for (int j = 0; j < l_keypoints.size(); j++) {
			//		cur_x = r_keypoints[j].pt.x;
			//		cur_y = r_keypoints[j].pt.y;
			//		cur_dist = sqrt(pow((prev_x - cur_x), 2) + pow((prev_y - cur_y), 2));

			//		if (cur_dist < min_dist) {
			//			min_dist = cur_dist;
			//			min_dist_ind = j;
			//		}
			//	}
			//	r_keypoints[0] = r_keypoints[min_dist_ind];

			//	l_prev_ball_location[0].x = l_keypoints[0].pt.x;
			//	l_prev_ball_location[0].y = l_keypoints[0].pt.y;
			//	r_prev_ball_location[0].x = r_keypoints[0].pt.x;
			//	r_prev_ball_location[0].y = r_keypoints[0].pt.y;
			//}



			// display all blobs
			//cvtColor(roi_diff[0], roi_diff[0], CV_GRAY2BGR);
			//cvtColor(roi_diff[1], roi_diff[1], CV_GRAY2BGR);

			//drawKeypoints(roi_diff[0], l_keypoints, roi_diff[0], Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
			//drawKeypoints(roi_diff[1], r_keypoints, roi_diff[1], Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

			//char diff_path[20];
			//sprintf(diff_path, "diff\\L%02d.bmp", l_points.size());
			//imwrite(diff_path, roi_diff[0]);
			//sprintf(diff_path, "diff\\R%02d.bmp", l_points.size());
			//imwrite(diff_path, roi_diff[1]);

			//cvtColor(roi_diff[0], roi_diff[0], CV_BGR2GRAY);
			//cvtColor(roi_diff[1], roi_diff[1], CV_BGR2GRAY);

			l_keypoints[0].pt.x = l_keypoints[0].pt.x + xl;
			r_keypoints[0].pt.x = r_keypoints[0].pt.x + xr;

			// add points to list
			l_points.push_back(l_keypoints[0].pt);
			r_points.push_back(r_keypoints[0].pt);

			int first = 5;
			int second = 17;
			if (l_points.size() == first || l_points.size() == second) {

				// undistort points
				undistortPoints(l_points, l_points_ctd, L_camera_matrix, L_dist_coeffs, R1, P1);
				undistortPoints(r_points, r_points_ctd, R_camera_matrix, R_dist_coeffs, R2, P2);

				vector <Point3f> l_points_disparity, r_points_disparity;
				for (int ii = 0; ii < l_points_ctd.size(); ii++) {
					float disparity = l_points_ctd[ii].x - r_points_ctd[ii].x;
					l_points_disparity.push_back(Point3f(l_points_ctd[ii].x, l_points_ctd[ii].y, disparity));
					r_points_disparity.push_back(Point3f(r_points_ctd[ii].x, r_points_ctd[ii].y, disparity));
				}

				// get 3d points and store
				vector <Point3f> l_3dpoints;
				perspectiveTransform(l_points_disparity, l_3dpoints, Q);


				if (l_points.size() != first) {
					l_3dpoints.erase(l_3dpoints.begin());
					for (int ii = 0; ii < l_3dpoints.size(); ii++) {
						if (l_3dpoints[ii].z > 500 || l_3dpoints[ii].z < 100) {
							l_3dpoints.erase(l_3dpoints.begin() + ii);
							ii--;
						}
					}

					float zprev;
					for (int ii = 0; ii < l_3dpoints.size(); ii++) {
						if (ii == 0) {
							zprev = l_3dpoints[ii].z;
						}
						else {
							if (l_3dpoints[ii].z > zprev + 3) {
								l_3dpoints.erase(l_3dpoints.begin() + ii);
								zprev = l_3dpoints[ii].z;
								ii--;
							}
							else {
								zprev = l_3dpoints[ii].z;
							}
						}
					}
				}




				// smooth estimate
				//for (int ii = 0; ii < l_3dpoints.size() - 4; ii++) {
				//	//l_3dpoints[ii].x = (l_3dpoints[ii].x + l_3dpoints[ii + 1].x + l_3dpoints[ii + 2].x) / 2;
				//	l_3dpoints[ii].y = (l_3dpoints[ii].y + l_3dpoints[ii + 1].y + l_3dpoints[ii + 2].y) / 2;
				//	l_3dpoints[ii].z = (l_3dpoints[ii].z + l_3dpoints[ii + 1].z + l_3dpoints[ii + 2].z) / 2;

				//}




				//determine polynomial fit, calculate z
				int degreey = 3;
				int degreex = 2;
				double chisq;
				gsl_matrix *Zx, *Zy, *covx, *covy;
				gsl_vector *x, *y, *cx, *cy, *wx, *wy;

				Zx = gsl_matrix_alloc(l_3dpoints.size(), degreex);
				x = gsl_vector_alloc(l_3dpoints.size());
				Zy = gsl_matrix_alloc(l_3dpoints.size(), degreey);
				y = gsl_vector_alloc(l_3dpoints.size());
				wx = gsl_vector_alloc(l_3dpoints.size());
				wy = gsl_vector_alloc(l_3dpoints.size());

				cx = gsl_vector_alloc(degreex);
				cy = gsl_vector_alloc(degreey);
				covx = gsl_matrix_alloc(degreex, degreex);
				covy = gsl_matrix_alloc(degreey, degreey);

				for (int i = 0; i < l_3dpoints.size(); i++){
					gsl_matrix_set(Zx, i, 0, 1.0);
					gsl_matrix_set(Zx, i, 1, l_3dpoints[i].z);

					gsl_matrix_set(Zy, i, 0, 1.0);
					gsl_matrix_set(Zy, i, 1, l_3dpoints[i].z);
					gsl_matrix_set(Zy, i, 2, l_3dpoints[i].z*l_3dpoints[i].z);

					gsl_vector_set(x, i, l_3dpoints[i].x);
					gsl_vector_set(y, i, l_3dpoints[i].y);
					gsl_vector_set(wy, i, wf*i);
					gsl_vector_set(wx, i, 1);
				}

				gsl_multifit_linear_workspace *workx = gsl_multifit_linear_alloc(l_3dpoints.size(), degreex);
				gsl_multifit_linear_workspace *worky = gsl_multifit_linear_alloc(l_3dpoints.size(), degreey);
				gsl_multifit_wlinear(Zx, wx, x, cx, covx, &chisq, workx);
				gsl_multifit_wlinear(Zy, wy, y, cy, covy, &chisq, worky);

				float x_est, y_est;
				x_est = cx->data[0];
				y_est = cy->data[0];

				// move estimates by offset
				x_est = -x_est + xoffset;
				y_est = -y_est + yoffset;
				
				if (l_points.size() == first) {
					y_est = y_est - 1;
				}


				// additional offset by quadrant
				//if (x_est < 2 && x_est < -2) {
				//	x_est = x_est+2;
				//	y_est = y_est - 1;
				//}
				//else if (x_est > 2) {
				//	x_est = x_est - 3;
				//}
				//else if (x_est < -2) {
				//	x_est = x_est + 2;
				//	if (y_est > 3) {
				//		y_est = y_est -1.5;
				//	}
				//	else if (y_est < -4) {
				//		y_est = y_est + 1;
				//	}
				//}

				// move catcher
				QS->Move_X = x_est;					// replace 0 with your x coordinate
				QS->Move_Y = y_est;					// replace 0 with your y coordinate
				SetEvent(QS->QSMoveEvent);		// Signal the move event to move catcher. The event will be reset in the move thread.
				num_updates++;

				/*ofstream ofs;
				ofs.open("points.txt", ofstream::out);
				ofs << "x: " << x_est << endl;
				ofs << "y: " << y_est << endl;
				ofs << "3dpts: " << l_3dpoints << endl;
				for (int jj = 0; jj < 2; jj++) {
					ofs << "cx: " << cx->data[jj] << endl;
				}
				for (int jj = 0; jj < 3; jj++) {
					ofs << "cy: " << cy->data[jj] << endl;
				}
				ofs.close();*/

				//cleanup
				gsl_matrix_free(Zx);
				gsl_matrix_free(Zy);
				gsl_matrix_free(covx);
				gsl_matrix_free(covy);
				gsl_vector_free(x);
				gsl_vector_free(y);
				gsl_vector_free(cx);
				gsl_vector_free(cy);
				gsl_vector_free(wx);
				gsl_vector_free(wy);
			}


			if (num_updates == 2) {
				//cleanup
				detected_ball = false;
				calibrated = false;
				first_ball = true;
				l_points.clear();
				r_points.clear();
				l_points_ctd.clear();
				r_points_ctd.clear();
				im_i = 0;
				num_updates = 0;
				//while (QS->IR.CatchBall) {
				//	;;
				//}
				Sleep(2000);
				QS->IR.ProcBuf[0](Rect(xl, yt, width, height)).copyTo(calib[0]);
				QS->IR.ProcBuf[1](Rect(xr, yt, width, height)).copyTo(calib[1]);
			}

			
		}
		// Display Image
		if (QS->IR.UpdateImage) {
			for (i=0; i<QS->IR.NumCameras; i++) {
				if (QS->IR.CaptureSequence || QS->IR.DisplaySequence) {
#ifdef PTG_COLOR
					QS->IR.SaveBuf[i][QS->IR.FrameID].copyTo(QS->IR.DispBuf[i]);
#else
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = QS->IR.SaveBuf[i][QS->IR.FrameID];
					merge(QS->IR.OutBuf, 3, QS->IR.DispBuf[i]);
#endif

					sprintf_s(str,"%d",QS->IR.FrameID);
					putText(QS->IR.DispBuf[0], str, Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, CV_RGB(0, 255, 0), 2);
					if (QS->IR.PlayDelay) Sleep(QS->IR.PlayDelay);
				} else {
#ifdef PTG_COLOR
					QS->IR.ProcBuf[i][BufID].copyTo(QS->IR.DispBuf[i]);
#else
					// Display OutBuf1 when Catch Ball, otherwise display the input image
					QS->IR.OutBuf[0] = QS->IR.OutBuf[1] = QS->IR.OutBuf[2] = (QS->IR.CatchBall) ? QS->IR.OutBuf1[i] : QS->IR.ProcBuf[i];
					merge(QS->IR.OutBuf, 3, QS->IR.DispBuf[i]);
					// line(QS->IR.DispBuf[i], Point(0, 400), Point(640, 400), Scalar(0, 255, 0), 1, 8, 0);
#endif
				}
				QS->QSSysDisplayImage();
			}
		}
		if (QS->IR.CaptureSequence || QS->IR.DisplaySequence) {
			QS->IR.FrameID++;
			if (QS->IR.FrameID == MAX_BUFFER) {				// Sequence if filled
				QS->IR.CaptureSequence = FALSE;
				QS->IR.DisplaySequence = FALSE;
			} else {
				QS->IR.FrameID %= MAX_BUFFER;
			}
		}
		BufID = 1 - BufID;
	} 
	QS->EventEndProcess = FALSE;
	return 0;
}

void CTCSys::QSSysInit()
{
	long i, j;
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
	char ErrorMsg[64];

	// How many cameras are on the bus?
	if(IR.busMgr.GetNumOfCameras((unsigned int *)&IR.NumCameras) != PGRERROR_OK){	// something didn't work correctly - print error message
        sprintf_s(ErrorMsg, "Connect Failure: %s", IR.PGRError.GetDescription());
        AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP );
	} else {
		IR.NumCameras = (IR.NumCameras > MAX_CAMERA) ? MAX_CAMERA : IR.NumCameras;
		for(i = 0; i < IR.NumCameras; i++) {		
			// Get PGRGuid
			if (IR.busMgr.GetCameraFromIndex(i, &IR.prgGuid[i]) != PGRERROR_OK) {    // change to 1-i is cameras are swapped after powered up
				sprintf_s(ErrorMsg, "PGRGuID Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			IR.pgrCamera[i] = new Camera;
			if (IR.pgrCamera[i]->Connect(&IR.prgGuid[i]) != PGRERROR_OK) { 
				sprintf_s(ErrorMsg, "PConnect Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set video mode and frame rate
			if (IR.pgrCamera[i]->SetVideoModeAndFrameRate(VIDEO_FORMAT, CAMERA_FPS) != PGRERROR_OK) { 
				sprintf_s(ErrorMsg, "Video Format Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set all camera configuration parameters
			if (IR.pgrCamera[i]->SetConfiguration(&IR.cameraConfig) != PGRERROR_OK) { 
				sprintf_s(ErrorMsg, "Set Configuration Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
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
				sprintf_s(ErrorMsg, "Shutter Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set gamma value
			IR.cameraProperty.type = GAMMA;
			IR.cameraProperty.absValue = 1.0;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				sprintf_s(ErrorMsg, "Gamma Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set sharpness value
			IR.cameraProperty.type = SHARPNESS;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.valueA = 2000;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				sprintf_s(ErrorMsg, "Sharpness Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
#ifdef  PTG_COLOR
			// Set white balance (R and B values)
			IR.cameraProperty = WHITE_BALANCE;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = WHITE_BALANCE_R;
			IR.cameraProperty.valueB = WHITE_BALANCE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				ErrorMsg.Format("White Balance Failure: %s",IR.PGRError.GetDescription());
				AfxMessageBox( ErrorMsg, MB_ICONSTOP );
			}
#endif
			// Set gain values (350 here gives 12.32dB, varies linearly)
			IR.cameraProperty = GAIN;
			IR.cameraProperty.absControl = false;
			IR.cameraProperty.onOff = true;
			IR.cameraProperty.valueA = GAIN_VALUE_A;
			IR.cameraProperty.valueB = GAIN_VALUE_B;
			if(IR.pgrCamera[i]->SetProperty(&IR.cameraProperty, false) != PGRERROR_OK){	
				sprintf_s(ErrorMsg, "Gain Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
			// Set trigger state
			IR.cameraTrigger.mode = 0;
			IR.cameraTrigger.onOff = TRIGGER_ON;
			IR.cameraTrigger.polarity = 0;
			IR.cameraTrigger.source = 0;
			IR.cameraTrigger.parameter = 0;
			if(IR.pgrCamera[i]->SetTriggerMode(&IR.cameraTrigger, false) != PGRERROR_OK){	
				sprintf_s(ErrorMsg, "Trigger Failure: %s", IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
            IR.embeddedInfo[i].frameCounter.onOff = true;
            IR.embeddedInfo[i].timestamp.onOff = true;
            IR.pgrCamera[i]->SetEmbeddedImageInfo(&IR.embeddedInfo[i]);
			// Start Capture Individually
			if (IR.pgrCamera[i]->StartCapture() != PGRERROR_OK) {
				sprintf_s(ErrorMsg, "Start Capture Camera %d Failure: %s", i, IR.PGRError.GetDescription());
				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP);
			}
		}
		// Start Sync Capture (only need to do it with one camera)
//		if (IR.pgrCamera[0]->StartSyncCapture(IR.NumCameras, (const Camera**)IR.pgrCamera, NULL, NULL) != PGRERROR_OK) {
//				sprintf_s(ErrorMsg, "Start Sync Capture Failure: %s", IR.PGRError.GetDescription());
//				AfxMessageBox(CA2W(ErrorMsg), MB_ICONSTOP );
//		}
	}

#else
	IR.NumCameras = MAX_CAMERA;
#endif
	Rect R = Rect(0, 0, 640, 480);
	// create openCV image
	for(i=0; i<IR.NumCameras; i++) {
#ifdef PTG_COLOR
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
		for (j=0; j<MAX_BUFFER; j++) 
			IR.SaveBuf[i][j].create(IR.DigSizeY, IR.DigSizeX, CV_8UC3);
#else
		IR.AcqBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.DispBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.ProcBuf[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		for (j=0; j<MAX_BUFFER; j++) 
			IR.SaveBuf[i][j].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
#endif
		IR.AcqPtr[i] = IR.AcqBuf[i].data;
		IR.DispROI[i] = IR.DispBuf[i](R); 
		IR.ProcROI[i] = IR.ProcBuf[i](R); 

		IR.OutBuf1[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutBuf2[i].create(IR.DigSizeY, IR.DigSizeX, CV_8UC1);
		IR.OutROI1[i] = IR.OutBuf1[i](R); 
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
	for (int i = 0; i < 2; i++) {
		::SetDIBitsToDevice(
			ImageDC[i]->GetSafeHdc(), 1, 1,
			m_bitmapInfo.bmiHeader.biWidth,
			::abs(m_bitmapInfo.bmiHeader.biHeight),
			0, 0, 0,
			::abs(m_bitmapInfo.bmiHeader.biHeight),
			IR.DispBuf[i].data,
			&m_bitmapInfo, DIB_RGB_COLORS);
	}
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