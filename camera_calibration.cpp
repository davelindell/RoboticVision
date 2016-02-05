#include <opencv2/core.hpp>A
#include <string.h>
#include <Windows.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <iostream>
#include <fstream>
#include <time.h>

#ifdef far
#undef far
#endif

using namespace std;
using namespace cv;

int task1() {
	// write image path and load image 
	Mat frame;
	char im_path[100];
	sprintf(im_path, "calibration_jpg\\AR1.jpg");
	frame = imread(im_path, CV_LOAD_IMAGE_COLOR);

	if (frame.data == NULL) {
		printf("Error reading file.");
		return -1;
	}



	// color to grayscale
	cvtColor(frame, frame, CV_RGB2GRAY);

	// get chessboard corners
	Size pattern_size(10, 7);
	vector<Point2f> centers;
	bool pattern_was_found;
	pattern_was_found = findChessboardCorners(frame, pattern_size, centers);

	// refine corners
	Size winSize = Size(5, 5); 
	Size zeroZone = Size(-1, -1); 
	TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
	cornerSubPix(frame, centers, winSize, zeroZone, criteria);

	// convert back to rgb
	cvtColor(frame, frame, CV_GRAY2RGB);

	// draw it
	drawChessboardCorners(frame, pattern_size, centers, pattern_was_found);
	
	// display it
	namedWindow("test", WINDOW_AUTOSIZE);
	imshow("test", frame);
	waitKey(0);

	// save sample image
	imwrite("out_task1.jpg", frame);

	return 0;
}


int task2(){

	// declare storage array
	vector<vector<Point2f>> corners(40);
	vector<vector<Point3f>> corners3(40);
	Size pattern_size(10, 7);

	// read in all 40 images
	for (int i = 1; i < 41; i++) {
		Mat frame;
		char im_path[100];
		sprintf(im_path, "calibration_jpg\\AR%d.jpg",i);
		frame = imread(im_path, CV_LOAD_IMAGE_COLOR);

		if (frame.data == NULL) {
			printf("Error reading file.");
			return -1;
		}

		// process corners
		// color to grayscale
		cvtColor(frame, frame, CV_RGB2GRAY);

		// get chessboard corners
		bool pattern_was_found;
		pattern_was_found = findChessboardCorners(frame, pattern_size, corners[i-1]);

		// refine corners
		Size winSize = Size(11, 11);
		Size zeroZone = Size(-1, -1);
		TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
		cornerSubPix(frame, corners[i-1], winSize, zeroZone, criteria);

		// convert back to rgb
		cvtColor(frame, frame, CV_GRAY2RGB);

		//// draw it
		//drawChessboardCorners(frame, pattern_size, corners[i-1], pattern_was_found);

		//namedWindow("test", WINDOW_AUTOSIZE);
		//imshow("test", frame);
		//waitKey(0);

		//// fill in the 3d coordinate vector
		for (int y = 0; y < 7; y++) {
			for (int x = 0; x < 10; x++) {
				corners3[i - 1].push_back(Point3f(25*x, 25*y, 0));
			}
		}

		//for (int j = 0; j < corners[i - 1].size(); j++) {
		//	corners3[i - 1].push_back(Point3f(corners[i - 1][j].x, corners[i - 1][j].y,0));
		//}
	}

	// run the calibrate camera command
	Mat camera_matrix;
	Mat dist_coeffs;
	vector<Mat> rvecs;
	vector<Mat> tvecs;
	calibrateCamera(corners3, corners, Size(640,480), camera_matrix, dist_coeffs, rvecs, tvecs);

	// write out extrinsic and intrinsic parameters
	ofstream ofs;
	ofs.open("out_task2.txt", ofstream::out);

	ofs << "Extrinsic Params = " << endl << " " << dist_coeffs << endl << endl;
	ofs << "Intrinsic Params = " << endl << " " << camera_matrix << endl << endl;

	ofs.close();

	FileStorage fs("out_task2.yml", FileStorage::WRITE);

	fs << "frameCount" << 40;
	time_t rawtime; time(&rawtime);
	fs << "calibrationDate" << asctime(localtime(&rawtime));
	fs << "cameraMatrix" << camera_matrix << "distCoeffs" << dist_coeffs;
	fs.release();

	return 0;
}


int task3() {
	// read yml file
	FileStorage fs("out_task2.yml", FileStorage::READ);

	// first method: use (type) operator on FileNode.
	int frameCount = (int)fs["frameCount"];

	std::string date;
	// second method: use FileNode::operator >>
	fs["calibrationDate"] >> date;

	Mat camera_matrix, dist_coeffs;
	fs["cameraMatrix"] >> camera_matrix;
	fs["distCoeffs"] >> dist_coeffs;

	fs.release();

	Mat far, far_ctd, far_diff;
	Mat close, close_ctd, close_diff;
	Mat turn, turn_ctd, turn_diff;

	far = imread("calibration_jpg\\Far.jpg", CV_LOAD_IMAGE_COLOR);
	close = imread("calibration_jpg\\Close.jpg", CV_LOAD_IMAGE_COLOR);
	turn = imread("calibration_jpg\\Turn.jpg", CV_LOAD_IMAGE_COLOR);

	undistort(far, far_ctd, camera_matrix, dist_coeffs);
	absdiff(far, far_ctd, far_diff);

	undistort(close, close_ctd, camera_matrix, dist_coeffs);
	absdiff(close, close_ctd, close_diff);

	undistort(turn, turn_ctd, camera_matrix, dist_coeffs);
	absdiff(turn, turn_ctd, turn_diff);

	imwrite("out_task3_far.jpg", far_diff);
	imwrite("out_task3_close.jpg", close_diff);
	imwrite("out_task3_turn.jpg", turn_diff);

	//namedWindow("test", WINDOW_AUTOSIZE);
	//imshow("test",far_ctd);
	//waitKey(0);

	
	return 0;
}

int task4() {
	// Read in points from provided data file
	ifstream data("data_task4.txt");

	char buff[100];

	vector<Point2f> image_points(0);
	vector<Point3f> object_points(0);
	float x, y, z;

	for (int i = 0; i < 20; i++){
		data.getline(buff, 100);
		stringstream ss;
		ss << buff;
		ss >> x;
		ss << buff;
		ss >> y;
		image_points.push_back(Point2f(x, y));
		memset(buff, 0, 100);
	}
	for (int i = 0; i < 20; i++){
		data.getline(buff, 100);
		stringstream ss;
		ss << buff;
		ss >> x;
		ss << buff;
		ss >> y;
		ss << buff;
		ss >> z;
		object_points.push_back(Point3f(x, y, z));
		memset(buff, 0, 100);
	}

	// read yml file
	FileStorage fs("out_task2.yml", FileStorage::READ);

	// first method: use (type) operator on FileNode.
	int frameCount = (int)fs["frameCount"];

	std::string date;
	// second method: use FileNode::operator >>
	fs["calibrationDate"] >> date;

	Mat camera_matrix, dist_coeffs;
	fs["cameraMatrix"] >> camera_matrix;
	fs["distCoeffs"] >> dist_coeffs;

	fs.release();
	
	Mat rvec, tvec;
	solvePnP(object_points, image_points, camera_matrix, dist_coeffs, rvec, tvec);

	Rodrigues(rvec, rvec);

	//rvec = rvec.diag(rvec);

	// write out rotation/translationa vectors
	ofstream ofs;
	ofs.open("out_task4.txt", ofstream::out);

	ofs << "Rotation Matrix = " << endl << " " << rvec << endl << endl;
	ofs << "Translation Vector = " << endl << " " << tvec << endl << endl;

	ofs.close();

	return 0;

}

int task5(){
	VideoCapture video(0); // get a camera object
	Mat frame; // allocate an image buffer object
	namedWindow("videoout", CV_WINDOW_AUTOSIZE);
	Size pattern_size(9,7);
	vector<vector<Point2f>> image_points(40);
	vector<vector<Point3f>> object_points(40);

	for (int i = 0; i < 40; i++){
		video >> frame;
		imshow("videoout", frame);
		int key_in = waitKey(30);
		if (key_in == VK_SPACE) {
			char out[100];
			sprintf(out,"my_calibration\\%d.jpg",i);
			imwrite(out, frame);
			continue;
		}
		i--;
	}

	vector<int> erased_idx;

	for (int i = 0; i < 40; i++) {
		char file_path[100];
		sprintf(file_path, "my_calibration/%d.jpg", i);
		frame = imread(file_path);
		
		//waitKey(0);

		// color to grayscale
		cvtColor(frame, frame, CV_RGB2GRAY);

		bool pattern_was_found;
		pattern_was_found = findChessboardCorners(frame, pattern_size, image_points[i]);

		if (!pattern_was_found) {
			erased_idx.push_back(i);
			continue;
		}

		// refine corners
		Size winSize = Size(11, 11);
		Size zeroZone = Size(-1, -1);
		TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);
		cornerSubPix(frame, image_points[i], winSize, zeroZone, criteria);

		// convert back to rgb
		cvtColor(frame, frame, CV_GRAY2RGB);

		//// draw it
		//drawChessboardCorners(frame, pattern_size, corners[i], pattern_was_found);

		//namedWindow("test", WINDOW_AUTOSIZE);
		//imshow("test", frame);
		//waitKey(0);

		// fill in the 3d coordinate vector
		for (int y = 0; y < 7; y++) {
			for (int x = 0; x < 9; x++) {
				object_points[i].push_back(Point3f(25 * x, 25 * y, 0));
			}
		}
	}

	// remove the image points for images that weren't useful
	for (int i = 0; i < image_points.size(); i++) {
		if (image_points[i].size() < 63) {
			image_points.erase(image_points.begin() + i);
			object_points.erase(object_points.begin() + i);
		}
	}

	// run the calibrate camera command
	Mat camera_matrix;
	Mat dist_coeffs;
	vector<Mat> rvecs;
	vector<Mat> tvecs;
	cv::calibrateCamera(object_points, image_points, Size(640, 480), camera_matrix, dist_coeffs, rvecs, tvecs);

	// write out extrinsic and intrinsic parameters
	ofstream ofs;
	ofs.open("out_task5.txt", ofstream::out);

	ofs << "Extrinsic Params = " << endl << " " << dist_coeffs << endl << endl;
	ofs << "Intrinsic Params = " << endl << " " << camera_matrix << endl << endl;

	ofs.close();

	FileStorage fs("out_task5.yml", FileStorage::WRITE);

	fs << "frameCount" << 40;
	time_t rawtime; time(&rawtime);
	fs << "calibrationDate" << asctime(localtime(&rawtime));
	fs << "cameraMatrix" << camera_matrix << "distCoeffs" << dist_coeffs;
	fs.release();

	return 0;
}

int task6()  {
	VideoCapture video(0); // get a camera object
	Mat frame, frame_ctd, frame_diff; // allocate an image buffer object
	namedWindow("videoout", CV_WINDOW_AUTOSIZE);

	Size pattern_size(9, 7);
	vector<Point2f> image_points;
	vector<Point3f> object_points;
	
	bool pattern_was_found = false;

	// read yml file
	FileStorage fs("out_task5.yml", FileStorage::READ);

	// first method: use (type) operator on FileNode.
	int frameCount = (int)fs["frameCount"];

	std::string date;
	// second method: use FileNode::operator >>
	fs["calibrationDate"] >> date;

	Mat camera_matrix, dist_coeffs;
	fs["cameraMatrix"] >> camera_matrix;
	fs["distCoeffs"] >> dist_coeffs;

	fs.release();

	while (!pattern_was_found) {
		video >> frame;
		cvtColor(frame, frame, CV_RGB2GRAY);
		pattern_was_found = findChessboardCorners(frame, pattern_size, image_points);
	}

	undistort(frame, frame_ctd, camera_matrix, dist_coeffs);
	absdiff(frame, frame_ctd, frame_diff);

	imwrite("out_task6_orig.jpg", frame);
	imwrite("out_task6_ctd.jpg", frame_ctd);
	imwrite("out_task6_diff.jpg", frame_diff);

	return 0;
}

int main(int argc, char **argv) {
	int task = 4;

	switch (task) {
	case 1: task1();
		break;
	case 2: task2();
		break;
	case 3: task3();
		break;
	case 4: task4();
		break;
	case 5: task5();
		break;
	case 6: task6();
		break;
	}

	return 0;

}
