#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/features2d.hpp>
#include <climits>
#include "hw1.h"

using namespace std;
using namespace cv;

struct KeyPointCompare {
	bool operator() (KeyPoint i, KeyPoint j) { return (i.size>j.size); }
} keyPointCompare;

int task3() {
	Mat frame, prev_frame, diff, begin_frame;
	size_t IM_BEGIN = 34;
	size_t IM_END = 72;
	char im_path[108];
	vector<Vec3f> circles;
	int th = 40;
	int ratio = 3;
	int kernel_size = 3;

	vector<KeyPoint> best_keypoint;
	KeyPoint prev_keypoint;

	namedWindow("Lindell Task 3", WINDOW_AUTOSIZE);

	for (size_t i = IM_BEGIN; i <= IM_END; i+=2) {

		// write image path and load image 
		sprintf(im_path, "TennisBallSequence\\image%03Iu.jpg", i);

		bool converged = false;
		bool try_absdiff = false;
		bool nodata = false;
		int num_iters = 0;

		if (!begin_frame.empty())
			begin_frame.copyTo(prev_frame);

		begin_frame = imread(im_path, CV_LOAD_IMAGE_COLOR);
		// check image input
		if (!begin_frame.data)
		{
			cout << "Could not open or find the image" << std::endl;
			return -1;
		}

		// initialize prev_frame
		if (prev_frame.empty()){
			begin_frame.copyTo(prev_frame);
		}

		while (!converged) {

			begin_frame.copyTo(frame);



			// compute difference image 
			// first try normal difference, then absolute if we don't get a good answer
			if (try_absdiff)
				absdiff(frame, prev_frame, diff);
			else
				subtract(frame, prev_frame, diff);
			diff.copyTo(frame);

			// threshold image
			th = 80;
			inRange(frame, Scalar(0, 0, 0), Scalar(th, th, th), frame);

			SimpleBlobDetector::Params params;

			// Change thresholds
			params.minThreshold = 100;
			params.maxThreshold = 200;

			// Filter by Area.
			params.filterByArea = false;
			params.minArea = 1500;

			// Filter by Circularity
			params.filterByCircularity = false;
			params.minCircularity = 0.01;

			// Filter by Convexity
			params.filterByConvexity = false;
			params.minConvexity = 0.01;

			// Filter by Inertia
			params.filterByInertia = false;
			params.minInertiaRatio = 0.01;

			Ptr<SimpleBlobDetector> d = SimpleBlobDetector::create(params);
			vector<KeyPoint> keypoints;
			d->detect(frame, keypoints);

			// check to see if we found any blobs
			if (keypoints.size() < 1){
				nodata = true;
				break;
			}

			// sort by blob size
			sort(keypoints.begin(), keypoints.end(), keyPointCompare);
			//int ksize = keypoints.size();
			//for (int j = 3; j < ksize; j++){
			//	keypoints.pop_back();
			//}


			Mat test_frame;
			frame.copyTo(test_frame);
			drawKeypoints(test_frame, keypoints, test_frame, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
			imshow("Lindell Task 3", test_frame);
			waitKey(0);

			// if possible, select the blob with shortest distance from previous best blob
			// else, select max size blob
			float cur_dist;
			float min_dist = INT_MAX;
			int min_dist_ind = -1;
			if (prev_keypoint.size > 0)
			{
				float prev_x = prev_keypoint.pt.x;
				float prev_y = prev_keypoint.pt.y;
				float cur_x;
				float cur_y;
				for (int j = 0; j < keypoints.size(); j++)
				{
					cur_x = keypoints.at(j).pt.x;
					cur_y = keypoints.at(j).pt.y;
					cur_dist = sqrt(pow((prev_x - cur_x), 2) + pow((prev_y - cur_y), 2));

					if (cur_dist < min_dist)
					{
						min_dist = cur_dist;
						min_dist_ind = j;
					}
				}
			}

			if (min_dist_ind != -1)
			{
				if (min_dist < frame.rows / 15) {
					best_keypoint.push_back(keypoints.at(min_dist_ind));
					converged = true;
				}
				else {
					converged = false;
					try_absdiff = true;
				}
			}
			else
			{
				best_keypoint.push_back(keypoints.at(0));
				converged = true;
			}


			// if we've already tried absdiff, then just report the answer
			if (num_iters > 0 && try_absdiff) {
				converged = true;
			}

			num_iters++;
		}

		if (nodata)
			continue;

		prev_keypoint = best_keypoint.at(0);

		drawKeypoints(frame, best_keypoint, frame, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

		best_keypoint.pop_back();

		// edge detection

		//cvtColor(frame, frame, COLOR_BGR2GRAY);
		//Canny(frame, frame, th, th*ratio, kernel_size);

		// find circles
		// smooth it, otherwise a lot of false circles may be detected
		//GaussianBlur(frame, frame, Size(3, 3), 2, 2);
		//HoughCircles(frame, circles, CV_HOUGH_GRADIENT,
		//	2, 1, 200, 50);


		//cvtColor(frame, frame, CV_GRAY2RGB);
		//for (size_t i = 0; i < circles.size(); i++)
		//{
		//	Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
		//	int radius = cvRound(circles[i][2]);
		//	// draw the circle center
		//	circle(frame, center, 3, Scalar(0, 255, 0), -1, 8, 0);
		//	// draw the circle outline
		//	circle(frame, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		//}



		imshow("Lindell Task 3", frame);

		waitKey(0);        
	}
	
	return 0;
}

int main(int argc, char** argv){
	if (argc != 2) {
		printf("Error, need 1 arg!\n");
		return -1;
	}
	if (!strcmp(argv[1], "1"))
		task1();
	else if (!strcmp(argv[1], "2"))
		task2();
	else if (!strcmp(argv[1], "3"))
		task3();
	else
		printf("Task %s not ready yet.\n", argv[0]);

	return 0;
}