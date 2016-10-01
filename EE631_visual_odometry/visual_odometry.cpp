#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>
#include <math.h>       /* pow */
#include<opencv2/core/cuda.hpp>
#include <opencv2/xfeatures2d/cuda.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include<opencv2/cudaimgproc.hpp>
#include <opencv2/cudaarithm.hpp>

using namespace cv;
using namespace std;
using namespace xfeatures2d;

void vo() {
	//Ptr<SURF> surf = SURF::create();
	//surf->setExtended(true);
	//surf->setHessianThreshold(500);

	cuda::SURF_CUDA surf;
	surf.extended = true;
	surf.hessianThreshold = 500;
	Ptr<cuda::FastFeatureDetector> fast = cuda::FastFeatureDetector::create();
	Ptr<cuda::DescriptorMatcher> bfmatcher = cuda::DescriptorMatcher::createBFMatcher(NORM_HAMMING);

	Ptr<cuda::ORB> orb = cuda::ORB::create(1000000);

	cuda::GpuMat gpu_keypoints[2];
	cuda::GpuMat gpu_descriptors[2];

	vector<vector<KeyPoint>> keypoints(2, vector<KeyPoint>(0));
	vector<float> descriptors[2];
	Mat mask_full = Mat::ones(Size(1226, 370), CV_8UC1);

	vector<DMatch> matches;
	vector<DMatch> gpu_matches;

	Mat M = (Mat_<float>(3, 3) << 7.070912000000e+02, 0.000000000000e+00, 
		6.018873000000e+02, 0.000000000000e+00, 7.070912000000e+02, 1.831104000000e+02, 
		0.000000000000e+00, 0.000000000000e+00, 1.000000000000e+00);
	double ox = M.at<float>(0,2);
	double oy = M.at<float>(1, 2);
	Point2f origin = Point(ox, oy);

	ofstream ofs;
	ofs.open("estimated_rt.txt", ofstream::out);
	
	int height_blocks = 1;
	int width_blocks = 1;
	int im_width = mask_full.cols;
	int im_height = mask_full.rows;

	vector<Rect> rects;
	int cur_x = 0;
	int cur_y = 0;
	int block_width = im_width / width_blocks;
	int block_height = im_height / height_blocks;
	int row = 1;
	for (int i = 0; i < height_blocks * width_blocks; i++) {
		int width = block_width;
		int height = block_height;
		if (i % width_blocks == width_blocks - 1) {
			width = im_width - cur_x;
		}
		if (row == height_blocks) {
			height = im_height - cur_y;
		}

		rects.push_back(Rect(cur_x,cur_y,width,height));
		cur_x = cur_x + width - 1;
		if (i % width_blocks == width_blocks - 1) {
			cur_y = cur_y + height - 1;
			cur_x = 0;
			row++;
		}
	}


	for (int i = 0; i < 701; i++) {
		printf("%03d\n", i);
		char path[50];
		vector<Point2f> points0, points1;
		Mat im0_full, im1_full;

		sprintf(path, "practice\\%06d.png", i);
		im0_full = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		sprintf(path, "practice\\%06d.png", i + 1);
		im1_full = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		// iterate over subregions to find good points over the whole image
		for (int r = 0; r < rects.size(); r++){
			Mat mask = Mat::ones(rects[r].size(), CV_8UC1);
			cuda::GpuMat gpu_mask(mask);

			Mat im0 = im0_full(rects[r]).clone();
			cuda::GpuMat gpu_im0(im0);
			
			orb->detectAndCompute(gpu_im0, gpu_mask, keypoints[0], gpu_descriptors[0]);

			Mat im1 = im1_full(rects[r]).clone();
			cuda::GpuMat gpu_im1(im1);
			orb->detectAndCompute(gpu_im1, gpu_mask, keypoints[1], gpu_descriptors[1]);

			bfmatcher->match(gpu_descriptors[0], gpu_descriptors[1], matches);

			vector<DMatch> good_matches;
			for (int j = 0; j < matches.size(); j++) {
				if (matches[j].distance < 200) {
					Point2f point0 = keypoints[0][matches[j].queryIdx].pt;
					Point2f point1 = keypoints[1][matches[j].trainIdx].pt;

					float dist = sqrt((point0.x - point1.x)*(point0.x - point1.x)
						+ (point0.y - point1.y)*(point0.y - point1.y));
					if (dist < 150) {
						good_matches.push_back(matches[j]);
					}

				}
			}
			for (int j = 0; j < good_matches.size(); j++) {
				Point point0, point1;
				point0 = keypoints[0][good_matches[j].queryIdx].pt;
				point0.x = point0.x + rects[r].x;
				point0.y = point0.y + rects[r].y;
				points0.push_back(point0);

				point1 = keypoints[1][good_matches[j].trainIdx].pt;
				point1.x = point1.x + rects[r].x;
				point1.y = point1.y + rects[r].y;
				points1.push_back(point1);
			}
		}


		Mat point_mask;
		Mat E = findEssentialMat(points0, points1,M,RANSAC,0.99999,1,point_mask);
		Mat R, T, P;

		double f = M.at<float>(0, 0);
		Point2d pp = Point2d(M.at<float>(0, 2), M.at<float>(1, 2));
		//recoverPose(E, points0, points1, R, T, f, pp, point_mask);

		//Mat u, w, vt;
		//SVD::compute(E, w, u, vt);
		//w = (Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 0);
		//E = u*w*vt;
		recoverPose(E, points0, points1, M, R, T, point_mask);

		hconcat(R, T, P);
		float sf = 2.15;
		Mat Ps = (Mat_<double>(4, 4) << P.at<double>(0, 0), P.at<double>(0, 1), P.at<double>(0, 2), sf*P.at<double>(0, 3),
			P.at<double>(1, 0), P.at<double>(1, 1), P.at<double>(1, 2), sf*P.at<double>(1, 3),
			P.at<double>(2, 0), P.at<double>(2, 1), P.at<double>(2, 2), sf*P.at<double>(2, 3),
			0, 0, 0, 1);
		Ps = Ps.inv();

		ofs << R.at<double>(0, 0) << " " << R.at<double>(0, 1) << " " << R.at<double>(0, 2) << " " << T.at<double>(0, 0) << " ";
		ofs << R.at<double>(1, 0) << " " << R.at<double>(1, 1) << " " << R.at<double>(1, 2) << " " << T.at<double>(1, 0) << " ";
		ofs << R.at<double>(2, 0) << " " << R.at<double>(2, 1) << " " << R.at<double>(2, 2) << " " << T.at<double>(2, 0);
		ofs << endl;

		// draw output
		//cvtColor(im0_full, im0_full, CV_GRAY2BGR);
		//for (int j = 0; j < points0.size(); j++) {
		//	if (point_mask.at<uchar>(j, 0) == 0)
		//		continue;

		//	circle(im0_full, points0[j], 4, Scalar(0, 255, 0), -1);
		//	line(im0_full, points0[j], points1[j], Scalar(0, 0, 255), 2);
		//}
		//cvtColor(im0_full, im0_full, CV_BGR2GRAY);

	}

	ofs.close();

	return;
}

void hallway() {
	//Ptr<SURF> surf = SURF::create();
	//surf->setExtended(true);
	//surf->setHessianThreshold(500);

	cuda::SURF_CUDA surf;
	surf.extended = true;
	surf.hessianThreshold = 400;
	Ptr<cuda::FastFeatureDetector> fast = cuda::FastFeatureDetector::create();
	Ptr<cuda::DescriptorMatcher> bfmatcher = cuda::DescriptorMatcher::createBFMatcher(NORM_HAMMING);

//	Ptr<cuda::ORB> orb = cuda::ORB::create(1000000,1.2,6,21,0,2,0,21,8);
	Ptr<cuda::ORB> orb = cuda::ORB::create(1000000);

	cuda::GpuMat gpu_keypoints[3];
	cuda::GpuMat gpu_descriptors[3];

	vector<vector<KeyPoint>> keypoints(3, vector<KeyPoint>(0));
	vector<float> descriptors[3];
	Mat mask_full = Mat::ones(Size(640, 480), CV_8UC1);

	vector<vector<DMatch>> matches(2,vector<DMatch>(0));
	vector<DMatch> gpu_matches;

	Mat M = (Mat_<float>(3, 3) << 6.7741251774486568e+02, 0.0000000000000000e+00, 3.2312557438767283e+02, 
		0.0000000000000000e+00, 6.8073800850564749e+02, 2.2477413395670021e+02,
		0.000000000000e+00, 0.000000000000e+00, 1.000000000000e+00);

	double ox = M.at<float>(0, 2);
	double oy = M.at<float>(1, 2);
	Point2f origin = Point(ox, oy);

	ofstream ofs;
	ofs.open("task2_rt.txt", ofstream::out);


	for (int i = 0; i < 2237; i++) {
		printf("%03d\n", i);
		char path[50];
		vector<Point2f> points0, points1, points2;
		Mat im0, im1, im2;

		sprintf(path, "hallway\\%06d.png", i);
		im0 = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		sprintf(path, "hallway\\%06d.png", i + 1);
		im1 = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		sprintf(path, "hallway\\%06d.png", i + 3);
		im2 = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		// iterate over subregions to find good points over the whole image
		Mat mask = Mat::ones(im1.size(), CV_8UC1);
		cuda::GpuMat gpu_mask(mask);

		cuda::GpuMat gpu_im0(im0);
		//surf(gpu_im0, gpu_mask, gpu_keypoints[0], gpu_descriptors[0]);
		orb->detectAndCompute(gpu_im0, gpu_mask, keypoints[0], gpu_descriptors[0]);

		cuda::GpuMat gpu_im1(im1);
		//surf(gpu_im1, gpu_mask, gpu_keypoints[1], gpu_descriptors[1]);
		orb->detectAndCompute(gpu_im1, gpu_mask, keypoints[1], gpu_descriptors[1]);

		cuda::GpuMat gpu_im2(im2);
		//surf(gpu_im2, gpu_mask, gpu_keypoints[2], gpu_descriptors[2]);
		orb->detectAndCompute(gpu_im2, gpu_mask, keypoints[2], gpu_descriptors[2]);

		//bfmatcher->match(gpu_descriptors[0], gpu_descriptors[1], matches[0]);
		//bfmatcher->match(gpu_descriptors[0], gpu_descriptors[2], matches[1]);
		//surf.downloadKeypoints(gpu_keypoints[0], keypoints[0]);
		//surf.downloadKeypoints(gpu_keypoints[1], keypoints[1]);
		//surf.downloadKeypoints(gpu_keypoints[2], keypoints[2]);
		bfmatcher->match(gpu_descriptors[0], gpu_descriptors[1], matches[0]);
		bfmatcher->match(gpu_descriptors[0], gpu_descriptors[2], matches[1]);

		vector<vector<DMatch>> good_matches(2, vector<DMatch>(0));
		for (int j = 0; j < matches[0].size(); j++) {
			Point2f point0 = keypoints[0][matches[0][j].queryIdx].pt;
			Point2f point1 = keypoints[1][matches[0][j].trainIdx].pt;
			Point2f point2 = keypoints[2][matches[1][j].trainIdx].pt;

			double dist1 = sqrt((point0.x - point1.x)*(point0.x - point1.x) + (point0.y - point1.y)*(point0.y - point1.y));
			double dist2 = sqrt((point0.x - point2.x)*(point0.x - point2.x) + (point0.y - point2.y)*(point0.y - point2.y));

			if (matches[0][j].queryIdx == matches[1][j].queryIdx && dist1 < 20 && dist2 < 20){
					good_matches[0].push_back(matches[0][j]);
					good_matches[1].push_back(matches[1][j]);
			}
		}

		for (int j = 0; j < good_matches[0].size(); j++) {
			Point point0, point1, point2;
			point0 = keypoints[0][good_matches[0][j].queryIdx].pt;
			points0.push_back(point0);

			point1 = keypoints[1][good_matches[0][j].trainIdx].pt;
			points1.push_back(point1);

			point2 = keypoints[2][good_matches[1][j].trainIdx].pt;
			points2.push_back(point2);
		}

		if (points0.size() < 6) {
			printf("Skipping!\n");
			continue;
		}

		Mat R, T, R1, T1, R2, T2;
		Mat point1_mask, point2_mask, point_mask;
		Mat E1 = findEssentialMat(points0, points1, M, RANSAC, 0.999, 1, point1_mask);
		Mat E2 = findEssentialMat(points1, points2, M, RANSAC, 0.999, 1, point2_mask);
		recoverPose(E1, points0, points1, M, R1, T1, point1_mask);
		recoverPose(E2, points1, points2, M, R2, T2, point2_mask);

		vector<Point> points1e, points2e, points1m, points2m;
		for (int j = 0; j < points0.size(); j++) {
			Mat pt = Mat(Point3d(points0[j].x, points0[j].y, 0));
			Mat rot = R1*pt;
			Mat trot = T1 + rot;
			trot.convertTo(trot, CV_32S);
			points1e.push_back(Point(trot.at<int>(0, 0), trot.at<int>(1, 0)));

			rot = R2*Mat(Point3d(points1[j].x, points1[j].y, 0));
			trot = T2 + rot;
			trot.convertTo(trot, CV_32S);
			points2e.push_back(Point(trot.at<int>(0, 0), trot.at<int>(1, 0)));

			double dist1 = sqrt((float)(points1[j].x - points1e[j].x)*(points1[j].x - points1e[j].x) + (float)(points1[j].y - points1e[j].y)*(points1[j].y - points1e[j].y));
			double dist2 = sqrt((float)(points2[j].x - points2e[j].x)*(points1[j].x - points2e[j].x) + (float)(points2[j].y - points2e[j].y)*(points2[j].y - points2e[j].y));
			if (dist1 < 30 && dist2 < 30) {
				points1m.push_back(Point(0.5*(points1e[j].x + points1[j].x), 0.5*(points1e[j].y + points1[j].y)));
				points2m.push_back(Point(0.5*(points2e[j].x + points2[j].x), 0.5*(points2e[j].y + points2[j].y)));
			}
		}

		if (points1m.size() < 6){
			printf("Skipping!\n");
			continue;
		}
		Mat u, w, vt;
		SVD::compute(E2, w, u, vt);
		w = (Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 0);
		E2 = u*w*vt;
		recoverPose(E2, points1m, points2m, M, R, T, point_mask);

		ofs << R.at<double>(0, 0) << " " << R.at<double>(0, 1) << " " << R.at<double>(0, 2) << " " << T.at<double>(0, 0) << " ";
		ofs << R.at<double>(1, 0) << " " << R.at<double>(1, 1) << " " << R.at<double>(1, 2) << " " << T.at<double>(1, 0) << " ";
		ofs << R.at<double>(2, 0) << " " << R.at<double>(2, 1) << " " << R.at<double>(2, 2) << " " << T.at<double>(2, 0);
		ofs << endl;

		// draw output
		cvtColor(im0, im0, CV_GRAY2BGR);
		for (int j = 0; j < points1m.size(); j++) {
			circle(im0, points1m[j], 4, Scalar(0, 255, 0), -1);
			line(im0, points1m[j], points2m[j], Scalar(0, 0, 255), 2);
		}
		cvtColor(im0, im0, CV_BGR2GRAY);

	}



	ofs.close();

	return;
}


void hallway2() {
	//Ptr<SURF> surf = SURF::create();
	//surf->setExtended(true);
	//surf->setHessianThreshold(500);

	cuda::SURF_CUDA surf;
	surf.extended = true;
	surf.hessianThreshold = 500;
	Ptr<cuda::FastFeatureDetector> fast = cuda::FastFeatureDetector::create();
	Ptr<cuda::DescriptorMatcher> bfmatcher = cuda::DescriptorMatcher::createBFMatcher(NORM_HAMMING);

	Ptr<cuda::ORB> orb = cuda::ORB::create(1000000);

	cuda::GpuMat gpu_keypoints[2];
	cuda::GpuMat gpu_descriptors[2];

	vector<vector<KeyPoint>> keypoints(2, vector<KeyPoint>(0));
	vector<float> descriptors[2];
	Mat mask_full = Mat::ones(Size(640, 480), CV_8UC1);

	vector<DMatch> matches;
	vector<DMatch> gpu_matches;

	Mat M = (Mat_<float>(3, 3) << 6.7741251774486568e+02, 0.0000000000000000e+00, 3.2312557438767283e+02,
		0.0000000000000000e+00, 6.8073800850564749e+02, 2.2477413395670021e+02,
		0.000000000000e+00, 0.000000000000e+00, 1.000000000000e+00);
	double ox = M.at<float>(0, 2);
	double oy = M.at<float>(1, 2);
	Point2f origin = Point(ox, oy);

	ofstream ofs;
	ofs.open("task2_rt.txt", ofstream::out);

	int height_blocks = 1;
	int width_blocks = 1;
	int im_width = mask_full.cols;
	int im_height = mask_full.rows;

	vector<Rect> rects;
	int cur_x = 0;
	int cur_y = 0;
	int block_width = im_width / width_blocks;
	int block_height = im_height / height_blocks;
	int row = 1;
	for (int i = 0; i < height_blocks * width_blocks; i++) {
		int width = block_width;
		int height = block_height;
		if (i % width_blocks == width_blocks - 1) {
			width = im_width - cur_x;
		}
		if (row == height_blocks) {
			height = im_height - cur_y;
		}

		rects.push_back(Rect(cur_x, cur_y, width, height));
		cur_x = cur_x + width - 1;
		if (i % width_blocks == width_blocks - 1) {
			cur_y = cur_y + height - 1;
			cur_x = 0;
			row++;
		}
	}


	for (int i = 0; i < 2230; i++) {
		printf("%03d\n", i);
		char path[50];
		vector<Point2f> points0, points1;
		Mat im0_full, im1_full;

		sprintf(path, "hallway\\%06d.png", i);
		im0_full = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		sprintf(path, "hallway\\%06d.png", i + 3);
		im1_full = imread(path, CV_LOAD_IMAGE_GRAYSCALE);

		// iterate over subregions to find good points over the whole image
		for (int r = 0; r < rects.size(); r++){
			Mat mask = Mat::ones(rects[r].size(), CV_8UC1);
			cuda::GpuMat gpu_mask(mask);

			Mat im0 = im0_full(rects[r]).clone();
			cuda::GpuMat gpu_im0(im0);

			orb->detectAndCompute(gpu_im0, gpu_mask, keypoints[0], gpu_descriptors[0]);

			Mat im1 = im1_full(rects[r]).clone();
			cuda::GpuMat gpu_im1(im1);
			orb->detectAndCompute(gpu_im1, gpu_mask, keypoints[1], gpu_descriptors[1]);

			bfmatcher->match(gpu_descriptors[0], gpu_descriptors[1], matches);

			vector<DMatch> good_matches;
			for (int j = 0; j < matches.size(); j++) {
				if (matches[j].distance < 200) {
					Point2f point0 = keypoints[0][matches[j].queryIdx].pt;
					Point2f point1 = keypoints[1][matches[j].trainIdx].pt;

					float dist = sqrt((point0.x - point1.x)*(point0.x - point1.x)
						+ (point0.y - point1.y)*(point0.y - point1.y));
					if (dist < 30) {
						good_matches.push_back(matches[j]);
					}

				}
			}
			for (int j = 0; j < good_matches.size(); j++) {
				Point point0, point1;
				point0 = keypoints[0][good_matches[j].queryIdx].pt;
				point0.x = point0.x + rects[r].x;
				point0.y = point0.y + rects[r].y;
				points0.push_back(point0);

				point1 = keypoints[1][good_matches[j].trainIdx].pt;
				point1.x = point1.x + rects[r].x;
				point1.y = point1.y + rects[r].y;
				points1.push_back(point1);
			}
		}


		Mat point_mask;

		if (points0.size() < 6) {
			printf("Skipping!\n");
			continue;
		}

		Mat E = findEssentialMat(points0, points1, M, RANSAC, 0.999999, 1, point_mask);
		Mat R, T, P;

		double f = M.at<float>(0, 0);
		Point2d pp = Point2d(M.at<float>(0, 2), M.at<float>(1, 2));
		//recoverPose(E, points0, points1, R, T, f, pp, point_mask);

		//Mat u, w, vt;
		//SVD::compute(E, w, u, vt);
		//w = (Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 0);
		//E = u*w*vt;
		recoverPose(E, points0, points1, M, R, T, point_mask);

		hconcat(R, T, P);
		float sf = 2.15;
		Mat Ps = (Mat_<double>(4, 4) << P.at<double>(0, 0), P.at<double>(0, 1), P.at<double>(0, 2), sf*P.at<double>(0, 3),
			P.at<double>(1, 0), P.at<double>(1, 1), P.at<double>(1, 2), sf*P.at<double>(1, 3),
			P.at<double>(2, 0), P.at<double>(2, 1), P.at<double>(2, 2), sf*P.at<double>(2, 3),
			0, 0, 0, 1);
		Ps = Ps.inv();

		ofs << R.at<double>(0, 0) << " " << R.at<double>(0, 1) << " " << R.at<double>(0, 2) << " " << T.at<double>(0, 0) << " ";
		ofs << R.at<double>(1, 0) << " " << R.at<double>(1, 1) << " " << R.at<double>(1, 2) << " " << T.at<double>(1, 0) << " ";
		ofs << R.at<double>(2, 0) << " " << R.at<double>(2, 1) << " " << R.at<double>(2, 2) << " " << T.at<double>(2, 0);
		ofs << endl;

		// draw output
		//cvtColor(im0_full, im0_full, CV_GRAY2BGR);
		//for (int j = 0; j < points0.size(); j++) {
		//	if (point_mask.at<uchar>(j, 0) == 0)
		//		continue;

		//	circle(im0_full, points0[j], 4, Scalar(0, 255, 0), -1);
		//	line(im0_full, points0[j], points1[j], Scalar(0, 0, 255), 2);
		//}
		//cvtColor(im0_full, im0_full, CV_BGR2GRAY);

	}

	ofs.close();

	return;
}

int main(int argc, char **argv) {
	hallway2();
	return 1;
}