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

	VideoWriter VOut; // Create a video write object.
	// Initialize video write object (only done once). Change frame size to match your camera resolution.
	VOut.open("lindell_task3.avi", CV_FOURCC('M', 'P', 'E', 'G'), 5, Size(640, 480), 1);

	// some variables and constants
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
		begin_frame = imread(im_path, CV_LOAD_IMAGE_COLOR);
		
		begin_frame.copyTo(frame);

		// check image input
		if (!frame.data)
		{
			cout << "Could not open or find the image" << std::endl;
			return -1;
		}

		// initialize prev_frame
		if (prev_frame.empty()){
			frame.copyTo(prev_frame);
		}

		// compute difference image
		subtract(frame, prev_frame, diff);
		frame.copyTo(prev_frame);
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
			continue;
		}

		// sort by blob size
		sort(keypoints.begin(), keypoints.end(), keyPointCompare);

		// display all blobs
		//Mat test_frame;
		//frame.copyTo(test_frame);
		//drawKeypoints(test_frame, keypoints, test_frame, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		//imshow("Lindell Task 3", test_frame);
		//waitKey(0);

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

		if (min_dist_ind != -1) // we had a previous point to compare to
		{
			// if we didn't find a good solution close to the previous one, just stick with the previous estimate
			if (min_dist / keypoints.at(min_dist_ind).size > 10) {
				best_keypoint.push_back(prev_keypoint);
			}
			else
				best_keypoint.push_back(keypoints.at(min_dist_ind));
		}
		else // we must be on a first image
		{
			best_keypoint.push_back(keypoints.at(0));
		}

		prev_keypoint = best_keypoint.at(0);

		circle(begin_frame, best_keypoint.at(0).pt, best_keypoint.at(0).size / 2, Scalar(0, 0, 255), -1, 8, 0);
		begin_frame.copyTo(frame);

		//drawKeypoints(begin_frame, best_keypoint, frame, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

		best_keypoint.pop_back();

		imshow("Lindell Task 3", frame);

		waitKey(30); 

		// save frame
		VOut << frame;

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