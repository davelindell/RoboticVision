#include <opencv2/core.hpp>
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

#define CALIB_COLS 10
#define CALIB_ROWS 7
#define CALIB_PTS 70
#define NUM_IMGS 32

using namespace std;
using namespace cv;

int getCorners(char *path, int side_opt, Size pattern_size, vector<vector<Point2f>> &corners, vector<vector<Point3f>> &corners3, vector<vector<Point2f>> &corners2 = vector<vector<Point2f>>(0)) {
	bool both_sides = false;
	int side = side_opt;
	if (side_opt == 2) {
		both_sides = true;
		side = 0;
	}

	size_t num_corners = corners.size();

	do {
		// read in all 32 images


		for (size_t i = 0; i < num_corners; i++) {

			Mat frame;
			char im_path[50];
			//if (side == 0) // Left camera
			//	sprintf(im_path, "%s\\CameraL%01d.bmp", path, i);
			//else // right camera
			//	sprintf(im_path, "%s\\CameraR%01d.bmp", path, i);
			//if (side == 0) // Left camera
			//	sprintf(im_path, "%s\\StereoL%01d.bmp", path, i);
			//else // right camera
			//	sprintf(im_path, "%s\\StereoR%01d.bmp", path, i);
			if (side == 0) // Left camera
				sprintf(im_path, "%s\\L%02d.bmp", path, i);
			else // right camera
				sprintf(im_path, "%s\\R%02d.bmp", path, i);



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
			if (side_opt == 2 && corners2.size() > 0 && side == 1) 
				pattern_was_found = findChessboardCorners(frame, pattern_size, corners2[i]);
			else
				pattern_was_found = findChessboardCorners(frame, pattern_size, corners[i]);

			if (!pattern_was_found)
				continue;

			// refine corners
			Size winSize = Size(11, 11);
			Size zeroZone = Size(-1, -1);
			TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 40, 0.001);

			if (side_opt == 2 && corners2.size() > 0 && side == 1)
				cornerSubPix(frame, corners2[i], winSize, zeroZone, criteria);
			else {
				cornerSubPix(frame, corners[i], winSize, zeroZone, criteria);
				//// fill in the 3d coordinate vector
				for (int y = 0; y < CALIB_ROWS; y++) {
					for (int x = 0; x < CALIB_COLS; x++) {
						corners3[i].push_back(Point3f(3.88 * x, 3.88 * y, 0));
						//corners3[i].push_back(Point3f(2 * x, 2 * y, 0));
					}
				}
			}

			// convert back to rgb
			// cvtColor(frame, frame, CV_GRAY2RGB);

			//// draw it
			//drawChessboardCorners(frame, pattern_size, corners[i-1], pattern_was_found);

			//namedWindow("test", WINDOW_AUTOSIZE);
			//imshow("test", frame);
			//waitKey(0);

		}

		side++;

	} while (both_sides && side<2);
	
	for (unsigned i = 0; i < corners.size(); i++){
		if (corners[i].size() != CALIB_PTS) {
			corners.erase(corners.begin() + i);
			corners3.erase(corners3.begin() + i);

			if (side_opt == 2 && corners2.size() > 0)
				corners2.erase(corners2.begin() + i);

			i--;
		}
		else if (side_opt == 2 && corners2.size() > 0 && corners2[i].size() != CALIB_PTS) {
			corners2.erase(corners2.begin() + i);
			corners.erase(corners.begin() + i);
			corners3.erase(corners3.begin() + i);
			i--;
		}
	}

	return 0;
}


int task1() {

	Size pattern_size(CALIB_COLS, CALIB_ROWS);

	for (int side = 0; side < 2; side++) {
		// declare storage array
		vector<vector<Point2f>> corners(NUM_IMGS);
		vector<vector<Point3f>> corners3(NUM_IMGS);

		getCorners("4-2", side, pattern_size, corners, corners3);
		//getCorners("single_test", side, pattern_size, corners, corners3);

		// run the calibrate camera command
		Mat camera_matrix;
		Mat dist_coeffs;
		vector<Mat> rvecs;
		vector<Mat> tvecs;
		calibrateCamera(corners3, corners, Size(640, 480), camera_matrix, dist_coeffs, rvecs, tvecs);

		// write out extrinsic and intrinsic parameters
		ofstream ofs; 

		if (side == 0) {
			ofs.open("out_task1L.txt", ofstream::out);
		}
		else {
			ofs.open("out_task1R.txt", ofstream::out);
		}


		ofs << "Extrinsic Params = " << endl << " " << dist_coeffs << endl << endl;
		ofs << "Intrinsic Params = " << endl << " " << camera_matrix << endl << endl;

		ofs.close();

		char fs_path[50];
		if (side == 0)
			sprintf(fs_path, "out_task1%s.yml", "L");
		else
			sprintf(fs_path, "out_task1%s.yml", "R");

		FileStorage fs(fs_path, FileStorage::WRITE);

		fs << "frameCount" << 32;
		time_t rawtime; time(&rawtime);
		fs << "calibrationDate" << asctime(localtime(&rawtime));
		fs << "cameraMatrix" << camera_matrix << "distCoeffs" << dist_coeffs;
		fs.release();
	}

	return 0;
}

int task2() {
	// read yml file
	FileStorage L_fs("out_task1L.yml", FileStorage::READ);
	FileStorage R_fs("out_task1R.yml", FileStorage::READ);

	Mat L_camera_matrix, L_dist_coeffs;
	Mat R_camera_matrix, R_dist_coeffs;

	L_fs["cameraMatrix"] >> L_camera_matrix;
	L_fs["distCoeffs"] >> L_dist_coeffs;
	R_fs["cameraMatrix"] >> R_camera_matrix;
	R_fs["distCoeffs"] >> R_dist_coeffs;

	L_fs.release();
	R_fs.release();

	// stereo calibrate
	Size pattern_size(CALIB_COLS, CALIB_ROWS);

	// declare storage array
	vector<vector<Point2f>> L_corners(NUM_IMGS), R_corners(NUM_IMGS);
	vector<vector<Point3f>> L_corners3(NUM_IMGS), R_corners3(NUM_IMGS);

	getCorners("4-2", 2, pattern_size, L_corners, L_corners3, R_corners);

	Mat R, T, E, F;

	stereoCalibrate(L_corners3, L_corners, R_corners, L_camera_matrix, L_dist_coeffs, R_camera_matrix, R_dist_coeffs, Size(640, 480), R, T, E, F, CV_CALIB_FIX_INTRINSIC);

	// write out cal parameters
	ofstream ofs;

	ofs.open("cal_params.txt", ofstream::out);
	ofs << "R = " << endl << " " << R << endl << endl;
	ofs << "T = " << endl << " " << T << endl << endl;
	ofs << "E = " << endl << " " << E << endl << endl;
	ofs << "F = " << endl << " " << F << endl << endl;
	ofs.close();

	FileStorage fs("cal_params.yml", FileStorage::WRITE);

	fs << "R" << R << "T" << T << "E" << E << "F" << F;
	fs.release();
	return 0;
}

int task3() {
	// read yml file
	FileStorage L_fs("out_task1L.yml", FileStorage::READ);
	FileStorage R_fs("out_task1R.yml", FileStorage::READ);
	FileStorage fs("cal_params.yml", FileStorage::READ);

	Mat L_camera_matrix, L_dist_coeffs;
	Mat R_camera_matrix, R_dist_coeffs;
	Mat R, T, E, F;

	L_fs["cameraMatrix"] >> L_camera_matrix;
	L_fs["distCoeffs"] >> L_dist_coeffs;
	R_fs["cameraMatrix"] >> R_camera_matrix;
	R_fs["distCoeffs"] >> R_dist_coeffs;
	fs["R"] >> R;
	fs["T"] >> T;
	fs["E"] >> E;
	fs["F"] >> F;

	fs.release();
	L_fs.release();
	R_fs.release();
	
	Mat L_ctd, R_ctd, L_im, R_im;
	//L_im = imread("stereo_test\\StereoL1.bmp", CV_LOAD_IMAGE_COLOR);
	//R_im = imread("stereo_test\\StereoR1.bmp", CV_LOAD_IMAGE_COLOR);
	L_im = imread("4-2\\L01.bmp", CV_LOAD_IMAGE_COLOR);
	R_im = imread("4-2\\R01.bmp", CV_LOAD_IMAGE_COLOR);

	undistort(L_im, L_ctd, L_camera_matrix, L_dist_coeffs);
	undistort(R_im, R_ctd, R_camera_matrix, R_dist_coeffs);

	Size pattern_size(CALIB_COLS, CALIB_ROWS);
	vector<Point2f> L_corners;
	vector<Point2f> R_corners;
	findChessboardCorners(L_ctd, pattern_size, L_corners);
	findChessboardCorners(R_ctd, pattern_size, R_corners);

	circle(L_ctd, L_corners[3], 5, Scalar(0, 0, 255));
	circle(L_ctd, L_corners[22], 5, Scalar(0, 0, 255));
	circle(L_ctd, L_corners[35], 5, Scalar(0, 0, 255));
	circle(L_ctd, L_corners[19], 5, Scalar(0, 255, 0));
	circle(L_ctd, L_corners[26], 5, Scalar(0, 255, 0));
	circle(L_ctd, L_corners[31], 5, Scalar(0, 255, 0));
	circle(R_ctd, R_corners[19], 5, Scalar(0, 0, 255));
	circle(R_ctd, R_corners[26], 5, Scalar(0, 0, 255));
	circle(R_ctd, R_corners[31], 5, Scalar(0, 0, 255));
	circle(R_ctd, R_corners[3], 5, Scalar(0, 255, 0));
	circle(R_ctd, R_corners[22], 5, Scalar(0, 255, 0));
	circle(R_ctd, R_corners[35], 5, Scalar(0, 255, 0));

	vector<Point2f> L_pts;
	vector<Point2f> R_pts;
	Mat L_lines, R_lines;
	L_pts.push_back(L_corners[35]);
	L_pts.push_back(L_corners[22]);
	L_pts.push_back(L_corners[3]);
	R_pts.push_back(R_corners[31]);
	R_pts.push_back(R_corners[26]);
	R_pts.push_back(R_corners[19]);

	//// draw it
	//namedWindow("test", WINDOW_AUTOSIZE);
	//imshow("test", L_ctd);
	//imshow("test", R_ctd);
	//waitKey(0);

	computeCorrespondEpilines(L_pts, 1, F, L_lines);
	computeCorrespondEpilines(R_pts, 2, F, R_lines);
	
	for (int i = 0; i < 3; i++) {
		//cout << Point2f(0, -L_lines.at<Vec3f>(i, 0)[2] / L_lines.at<Vec3f>(i, 0)[1]) << endl;
		//cout << Point2f(639, (L_lines.at<Vec3f>(i, 0)[0]*639 +
		//	L_lines.at<Vec3f>(i, 0)[2]) / (-1 * L_lines.at<Vec3f>(i, 0)[1])) << endl;
		line(R_ctd, Point2f(0, -L_lines.at<Vec3f>(i, 0)[2] / L_lines.at<Vec3f>(i, 0)[1]), Point2f(639, (L_lines.at<Vec3f>(i, 0)[0]*639 + 
			L_lines.at<Vec3f>(i, 0)[2]) / (-1 * L_lines.at<Vec3f>(i, 0)[1])), Scalar(0, 0, 255));
		line(L_ctd, Point2f(0, -R_lines.at<Vec3f>(i, 0)[2] / R_lines.at<Vec3f>(i, 0)[1]), Point2f(639, (R_lines.at<Vec3f>(i, 0)[0] * 639 +
			R_lines.at<Vec3f>(i, 0)[2]) / (-1 * R_lines.at<Vec3f>(i, 0)[1])), Scalar(0, 0, 255));
	}

	imwrite("task3L.bmp", L_ctd);
	imwrite("task3R.bmp", R_ctd);


	return 0;
}

int task4() {
	// read yml file
	FileStorage L_fs("out_task1L.yml", FileStorage::READ);
	FileStorage R_fs("out_task1R.yml", FileStorage::READ);
	FileStorage fs("cal_params.yml", FileStorage::READ);

	Mat L_camera_matrix, L_dist_coeffs;
	Mat R_camera_matrix, R_dist_coeffs;
	Mat R, T, E, F;

	L_fs["cameraMatrix"] >> L_camera_matrix;
	L_fs["distCoeffs"] >> L_dist_coeffs;
	R_fs["cameraMatrix"] >> R_camera_matrix;
	R_fs["distCoeffs"] >> R_dist_coeffs;
	fs["R"] >> R;
	fs["T"] >> T;
	fs["E"] >> E;
	fs["F"] >> F;

	fs.release();
	L_fs.release();
	R_fs.release();
	
	Mat L_ctd, R_ctd, L_im, R_im, L_im2, R_im2, L_m1, L_m2, R_m1, R_m2;
	//L_im = imread("stereo_test\\StereoL1.bmp", CV_LOAD_IMAGE_COLOR);
	//R_im = imread("stereo_test\\StereoR1.bmp", CV_LOAD_IMAGE_COLOR);
	L_im = imread("4-2\\L01.bmp", CV_LOAD_IMAGE_COLOR);
	R_im = imread("4-2\\R01.bmp", CV_LOAD_IMAGE_COLOR);

	Mat R1, R2, P1, P2, Q;
	stereoRectify(L_camera_matrix, L_dist_coeffs, R_camera_matrix, R_dist_coeffs, Size(640, 480), R, T, R1, R2, P1, P2, Q);

	initUndistortRectifyMap(L_camera_matrix, L_dist_coeffs, R1, P1, Size(640, 480), CV_32FC1, L_m1, L_m2);
	initUndistortRectifyMap(R_camera_matrix, R_dist_coeffs, R2, P2, Size(640, 480), CV_32FC1, R_m1, R_m2);
	remap(L_im, L_ctd, L_m1, L_m2, INTER_LINEAR);
	remap(R_im, R_ctd, R_m1, R_m2, INTER_LINEAR);

	for (int i = 0; i < 480; i += 20) {
		line(L_ctd, Point2f(0, i), Point2f(639, i), Scalar(0,255,0));
		line(R_ctd, Point2f(0, i), Point2f(639, i), Scalar(0, 255, 0));
	}

	Mat diff;
	absdiff(L_ctd, R_ctd, diff);

	ofstream ofs;

	ofs.open("rectify_params.txt", ofstream::out);
	ofs << "R1 = " << endl << " " << R1 << endl << endl;
	ofs << "R2 = " << endl << " " << R2 << endl << endl;
	ofs << "P1 = " << endl << " " << P1 << endl << endl;
	ofs << "P2 = " << endl << " " << P2 << endl << endl;
	ofs << "Q = " << endl << " " << Q << endl << endl;

	ofs.close();

	FileStorage fs_out("rectify_params.yml", FileStorage::WRITE);

	fs_out << "R1" << R1 << "R2" << R2 << "P1" << P1 << "P2" << P2 << "Q" << Q;
	fs_out.release();

	imwrite("task4L.bmp", L_ctd);
	imwrite("task4R.bmp", R_ctd);
	imwrite("task4diff.bmp", diff);
	return 0;
}

int task5() {
	Mat L_im, R_im;
	L_im = imread("4-2\\L01.bmp", CV_LOAD_IMAGE_COLOR);
	R_im = imread("4-2\\R01.bmp", CV_LOAD_IMAGE_COLOR);

	// read yml file
	FileStorage L_fs("out_task1L.yml", FileStorage::READ);
	FileStorage R_fs("out_task1R.yml", FileStorage::READ);
	FileStorage fs("rectify_params.yml", FileStorage::READ);

	Mat L_camera_matrix, L_dist_coeffs;
	Mat R_camera_matrix, R_dist_coeffs;
	Mat R1, R2, P1, P2, Q;

	L_fs["cameraMatrix"] >> L_camera_matrix;
	L_fs["distCoeffs"] >> L_dist_coeffs;
	R_fs["cameraMatrix"] >> R_camera_matrix;
	R_fs["distCoeffs"] >> R_dist_coeffs;
	fs["R1"] >> R1;
	fs["R2"] >> R2;
	fs["P1"] >> P1;
	fs["P2"] >> P2;
	fs["Q"] >> Q;

	fs.release();
	L_fs.release();
	R_fs.release();

	Size pattern_size(CALIB_COLS, CALIB_ROWS);
	vector<Point2f> L_corners;
	vector<Point2f> R_corners;
	findChessboardCorners(L_im, pattern_size, L_corners);
	findChessboardCorners(R_im, pattern_size, R_corners);

	vector<Point2f> L_pts, L_ctd;
	vector<Point2f> R_pts, R_ctd;

	// points of interest
	L_pts.push_back(L_corners[35]);
	L_pts.push_back(L_corners[27]);
	L_pts.push_back(L_corners[22]);
	L_pts.push_back(L_corners[3]);

	R_pts.push_back(R_corners[35]);
	R_pts.push_back(R_corners[27]);
	R_pts.push_back(R_corners[22]);
	R_pts.push_back(R_corners[3]);

	circle(L_im, L_corners[35], 5, Scalar(0, 0, 255));
	circle(L_im, L_corners[27], 5, Scalar(0, 0, 255));
	circle(L_im, L_corners[22], 5, Scalar(0, 0, 255));
	circle(L_im, L_corners[3], 5, Scalar(0, 255, 0));
	circle(R_im, R_corners[35], 5, Scalar(0, 0, 255));
	circle(R_im, R_corners[27], 5, Scalar(0, 0, 255));
	circle(R_im, R_corners[22], 5, Scalar(0, 0, 255));
	circle(R_im, R_corners[3], 5, Scalar(0, 255, 0));

	imwrite("task5L.bmp", L_im);
	imwrite("task5R.bmp", R_im);

	undistortPoints(L_pts, L_ctd, L_camera_matrix, L_dist_coeffs, R1, P1);
	undistortPoints(R_pts, R_ctd, R_camera_matrix, R_dist_coeffs, R2, P2);

	vector<Point3f> L_3d, R_3d;
	for (int i = 0; i < 4; i++) {
		L_3d.push_back(Point3f(L_ctd[i].x, L_ctd[i].y, L_ctd[i].x - R_ctd[i].x));
		R_3d.push_back(Point3f(R_ctd[i].x, R_ctd[i].y, L_ctd[i].x - R_ctd[i].x));
	}

	//vector<Point3f> all_world_pts;
	//vector<Point3f> world_pts;
	//for (int y = 0; y < CALIB_ROWS; y++) {
	//	for (int x = 0; x < CALIB_COLS; x++) {
	//		all_world_pts.push_back(Point3f(3.88 * x, 3.88 * y, 0));
	//		//corners3[i].push_back(Point3f(2 * x, 2 * y, 0));
	//	}
	//}

	//world_pts.push_back(all_world_pts[35]);
	//world_pts.push_back(all_world_pts[27]);
	//world_pts.push_back(all_world_pts[22]);
	//world_pts.push_back(all_world_pts[3]);

	Mat L_coord, R_coord;
	perspectiveTransform(L_3d, L_coord, Q);
	perspectiveTransform(R_3d, R_coord, Q);


	ofstream ofs;

	ofs.open("3dpts.txt", ofstream::out);
	ofs << "Left = " << endl;
	for (int i = 0; i < 4; i++) {
		ofs << L_coord.at<Point3f>(i) << endl;
	}
	
	ofs << endl;
	ofs << "Right = " << endl;
	for (int i = 0; i < 4; i++) {
		ofs << R_coord.at<Point3f>(i) << endl;
	}
	ofs.close();

	return 0;
}



int main(int argc, char **argv){
	int task = 5;

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
	}

	return 0;
}