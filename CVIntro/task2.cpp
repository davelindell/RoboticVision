#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "hw1.h"

using namespace cv;
using namespace std;

int task1() {
	VideoCapture video(0); // get a camera object
	Mat frame; // allocate an image buffer object
	namedWindow("Lindell", CV_WINDOW_AUTOSIZE); // initialize a display window
	video >> frame; // grab a frame from the video feed
	imshow("Lindell", frame); // display the grabbed frame
	waitKey(0);
	return 0;
}

// task 2 functions

void detectCorner(Mat &frame) {
	vector<Point2f> corner_pts;
	double qualityLevel = 0.01;
	double minDistance = 10;
	bool useHarrisDetector = false;
	int maxCorners = 50;
	int blockSize = 2;
	double k = 0.04;
	Size winSize = Size(5, 5);
	Size zeroZone = Size(-1, -1);
	TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);

	/// Apply corner detection
	goodFeaturesToTrack(frame,
		corner_pts,
		maxCorners,
		qualityLevel,
		minDistance,
		Mat(),
		blockSize,
		useHarrisDetector,
		k);

	cornerSubPix(frame, corner_pts, winSize, zeroZone, criteria);
	cvtColor(frame, frame, CV_GRAY2RGB);

	/// Drawing a circle around corners
	for (unsigned i = 0; i < corner_pts.size(); i++){
		circle(frame, Point(corner_pts.at(i).x, corner_pts.at(i).y), 5, Scalar(0, 0, 250), 2, 8, 0);
	}
	return; 
}

void detectLines(Mat &frame, int th){
#ifdef EasyDetectLines
	vector<Vec2f> lines;
	Mat lines_frame;
	cvtColor(frame, lines_frame, CV_BGR2GRAY);
	Canny(lines_frame, lines_frame, th, th * 3, 3);
	HoughLines(lines_frame, lines, 1, CV_PI / 180, 100, 0, 0);

	for (size_t i = 0; i < lines.size(); i++)
	{
		float rho = lines[i][0], theta = lines[i][1];
		Point pt1, pt2;
		double a = cos(theta), b = sin(theta);
		double x0 = a*rho, y0 = b*rho;
		pt1.x = cvRound(x0 + 1000 * (-b));
		pt1.y = cvRound(y0 + 1000 * (a));
		pt2.x = cvRound(x0 - 1000 * (-b));
		pt2.y = cvRound(y0 - 1000 * (a));
		line(frame, pt1, pt2, Scalar(0, 0, 255), 3, CV_AA);
	}
#else
	vector<Vec4i> lines;
	Mat lines_frame;
	cvtColor(frame, lines_frame, CV_BGR2GRAY);
	Canny(lines_frame, lines_frame, th, th * 3, 3);
	HoughLinesP(lines_frame, lines, 1, CV_PI / 180, 50, 50, 10);
	for (size_t i = 0; i < lines.size(); i++)
	{
		Vec4i l = lines[i];
		line(frame, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0, 0, 255), 3, CV_AA);
	}
#endif


	return;
}

void addLabel(Mat &frame, int th){
	// put text to label the threshold
	char th_str[32];
	if (frame.channels() == 1)
		cvtColor(frame, frame, CV_GRAY2RGB);
	sprintf(th_str, "%d", th);
	putText(frame, th_str, cvPoint(30, 30),
		FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0, 0, 255), 1, CV_AA);
	return;
}

int task2() {
	int state = 0;
	int key_in;
	int th = 150;
	int ratio = 3;
	int kernel_size = 3;

	VideoCapture video(0); // get a camera object
	Mat frame, prev_frame, diff; // allocate an image buffer object
	namedWindow("Lindell", CV_WINDOW_AUTOSIZE); // initialize a display window
	
	VideoWriter VOut; // Create a video write object.
	// Initialize video write object (only done once). Change frame size to match your camera resolution.
	VOut.open("lindell_task2.avi", CV_FOURCC('M', 'P', 'E', 'G'), 30, Size(640,480), 1);

	for (;;)
	{
		// grab a frame from the video feed
		video >> frame; 

		// initialize prev_frame
		if (prev_frame.empty()){
			frame.copyTo(prev_frame);
		}

		// get user inputs
		key_in = waitKey(30);
		if (key_in >> 16 == VK_UP) {
			state++;
			state = state % 6;
		}
		else if (key_in >> 16 == VK_DOWN) {
			if (state == 0)
				state = 5;
			else
				state--;
		}
		else if (key_in == VK_SPACE)
			break;
		else if (key_in >> 16 == VK_LEFT) 
		{
			th -= 5;
			if (th < 0)
				th = 0;
		}
		else if (key_in >> 16 == VK_RIGHT)
		{
			th += 5;
			if (th > 255)
				th = 255;
		}

		// debug
		printf("State: %d th: %d Key Code: 0x%x\n",state,th,key_in);

		// detect mode and process video frame
		switch (state) {
		case 0: // normal video
			break;
		case 1: // thresholded video
			inRange(frame, Scalar(0, 0, 0), Scalar(th, th, th), frame);
			break;
		case 2: // edge detection
			cvtColor(frame, frame, COLOR_BGR2GRAY);
			Canny(frame, frame, th, th*ratio, kernel_size);
			break;
		case 3: // corner detection
			cvtColor(frame, frame, CV_BGR2GRAY);
			detectCorner(frame);
			break;
		case 4: // line detection
			detectLines(frame, th);
			break;
		case 5: // difference image
			absdiff(frame, prev_frame, diff);
			frame.copyTo(prev_frame);
			diff.copyTo(frame);
		}

		// add threshold text to frame
		if (state == 1 || state == 2 || state == 4)
			addLabel(frame, th);

		// display frame
		imshow("Lindell", frame);

		// save frame
		VOut << frame; 

	}
	
	return 0;
}