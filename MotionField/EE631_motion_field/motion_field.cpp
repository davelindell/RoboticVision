#include <opencv2\core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/video/tracking.hpp>

using namespace cv;
using namespace std;

void task1(Mat *optical_flow);
void task2(Mat *optical_flow);
void task3(char *path_in, char *path_out);

int main(int argc, char **argv) {
	char path[50];
	Mat optical_flow[17];
	
	for (int i = 1; i < 17; i++) {
		sprintf(path, "Optical Flow\\O%d.jpg", i);
		optical_flow[i-1] = imread(path, IMREAD_GRAYSCALE);
	}

	task1(optical_flow);
	task2(optical_flow);

	char path_in[50];
	char path_out[50];
	sprintf(path_in, "Parallel Cube\\ParallelCube");
	sprintf(path_out, "parallel_cube");
	task3(path_in, path_out);

	sprintf(path_in, "Parallel Real\\ParallelReal");
	sprintf(path_out, "parallel_real");
	task3(path_in, path_out);

	sprintf(path_in, "Turned Cube\\TurnCube");
	sprintf(path_out, "turned_cube");
	task3(path_in, path_out);

	sprintf(path_in, "Turned Real\\TurnReal");
	sprintf(path_out, "turned_real");
	task3(path_in, path_out);
}

void task1(Mat *optical_flow) {
	VideoWriter vout; // Create a video write object.
	vout.open("task1.avi", CV_FOURCC('M', 'P', 'E', 'G'), 3, Size(640, 480), 1);

	vector<vector<Point2f>> corners_next(17, vector<Point2f>(0));
	int max_corners = 505;
	double quality_level = 0.01;
	double min_distance = 10;

	goodFeaturesToTrack(optical_flow[0], corners_next[0], max_corners, quality_level, min_distance);
	Mat prev_pts;
	Mat status, err;
	Mat out[17];
	int skip = 1;

	for (int max_i = 15; max_i > 12; max_i--) {
		for (int i = 0; i < max_i; i++){
			calcOpticalFlowPyrLK(optical_flow[i], optical_flow[i + skip], corners_next[i], corners_next[i + 1], status, err);
		}
		for (int i = 0; i < max_i; i++) {
			cvtColor(optical_flow[i], out[i], CV_GRAY2BGR);
			for (int j = 0; j < corners_next[i + 1].size(); j++) {
				line(out[i], corners_next[i][j], corners_next[i + 1][j], Scalar(0, 0, 255), 2);
			}
			for (int j = 0; j < corners_next[i].size(); j++){
				circle(out[i], corners_next[i][j], 1, Scalar(0, 255, 0),-1);
			}

			vout << out[i];
		}
		skip++;
	}
	

	vout.release();

}

void task2(Mat *optical_flow) {
	VideoWriter vout; // Create a video write object.
	vout.open("task2.avi", CV_FOURCC('M', 'P', 'E', 'G'), 3, Size(640, 480), 1);

	int max_corners = 500;
	double quality_level = 0.01;
	double min_distance = 4;

	Mat prev_pts;
	Mat status, err;
	Mat out[17];
	int skip = 1;
	int width = 15;
	int height = 15;
	int swidth = 50;
	int sheight = 50;

	for (int max_i = 15; max_i > 12; max_i--) {
		vector<vector<Point2f>> corners_next(17, vector<Point2f>(0));
		goodFeaturesToTrack(optical_flow[0], corners_next[0], max_corners, quality_level, min_distance);
		for (int i = 0; i < max_i; i++){
			printf("Current Index: %d\n", i);
			for (int j = 0; j < corners_next[i].size(); j++) {
				int x = corners_next[i][j].x - width / 2;
				int y = corners_next[i][j].y - height / 2;

				int sx = corners_next[i][j].x - swidth / 2;
				int sy = corners_next[i][j].y - sheight / 2;

				Mat match_output;
				double min_val, max_val;
				Point min_idx, max_idx;
				
				if (x + width - 1 > 639 || sx + swidth - 1 > 639){
					corners_next[i + 1].push_back(corners_next[i][j]);
					continue;
				}
				else if (x < 0 || sx < 0) {
					corners_next[i + 1].push_back(corners_next[i][j]);
					continue;
				}
				if (y + height - 1 > 479 || sy + sheight - 1 > 479) {
					corners_next[i + 1].push_back(corners_next[i][j]);
					continue;
				}
				else if (y < 0 || sy < 0) {
					corners_next[i + 1].push_back(corners_next[i][j]);
					continue;
				}

				matchTemplate(optical_flow[i+skip](Rect(sx, sy, swidth, sheight)), optical_flow[i](Rect(x, y, width, height)), match_output, CV_TM_SQDIFF);
				minMaxLoc(match_output, &min_val, &max_val, &min_idx, &max_idx);

				/*cvtColor(optical_flow[i], optical_flow[i], CV_GRAY2BGR);
				cvtColor(optical_flow[i + skip], optical_flow[i + skip], CV_GRAY2BGR);

				rectangle(optical_flow[i], Rect(x, y, width, height), Scalar(0, 0, 255));
				rectangle(optical_flow[i+skip], Rect(sx, sy, swidth, sheight), Scalar(0, 0, 255));*/

				Point match_idx;
				match_idx.x = min_idx.x + sx+width/2;
				match_idx.y = min_idx.y + sy + height / 2;
				corners_next[i + 1].push_back(match_idx);

				//cvtColor(optical_flow[i], optical_flow[i], CV_BGR2GRAY);
				//cvtColor(optical_flow[i + skip], optical_flow[i + skip], CV_BGR2GRAY);

			}
			
		}
		for (int i = 0; i < max_i; i++) {
			cvtColor(optical_flow[i], out[i], CV_GRAY2BGR);
			for (int j = 0; j < corners_next[i + 1].size(); j++) {
				line(out[i], corners_next[i][j], corners_next[i + 1][j], Scalar(0, 0, 255), 2);
			}
			for (int j = 0; j < corners_next[i].size(); j++){
				circle(out[i], corners_next[i][j], 1, Scalar(0, 255, 0), -1);
			}

			vout << out[i];
		}
		skip++;
	}


	vout.release();
}

void task3(char *path_in, char *path_out) {
	char path[50];
	Mat parallel_cube[17];

	for (int i = 10; i < 16; i++) {
		sprintf(path, "%s%d.jpg", path_in,i);
		parallel_cube[i - 10] = imread(path, IMREAD_GRAYSCALE);
	}


	int max_corners = 300;
	double quality_level = 0.01;
	double min_distance = 4;

	Mat prev_pts;
	Mat status, err;
	Mat out[2];
	int skip = 1;
	int width = 15;
	int height = 15;
	int swidth = 50;
	int sheight = 50;
	Mat pt_status[5];

	vector<vector<Point2f>> corners_next(17, vector<Point2f>(300));
	goodFeaturesToTrack(parallel_cube[0], corners_next[0], max_corners, quality_level, min_distance);
	for (int i = 0; i < 5; i++){
		printf("Current Index: %d\n", i);
		for (int j = 0; j < corners_next[i].size(); j++) {
			int x = corners_next[i][j].x - width / 2;
			int y = corners_next[i][j].y - height / 2;

			int sx = corners_next[i][j].x - swidth / 2;
			int sy = corners_next[i][j].y - sheight / 2;

			Mat match_output;
			double min_val, max_val;
			Point min_idx, max_idx;

			if (x + width - 1 > 639 || sx + swidth - 1 > 639){
				corners_next[i + 1][j] = Point(-1,-1);
				continue;
			}
			else if (x < 0 || sx < 0) {
				corners_next[i + 1][j] = Point(-1, -1);
				continue;
			}
			if (y + height - 1 > 479 || sy + sheight - 1 > 479) {
				corners_next[i + 1][j] = Point(-1, -1);
				continue;
			}
			else if (y < 0 || sy < 0) {
				corners_next[i + 1][j] = Point(-1, -1);
				continue;
			}

			matchTemplate(parallel_cube[i + 1](Rect(sx, sy, swidth, sheight)), parallel_cube[i](Rect(x, y, width, height)), match_output, CV_TM_SQDIFF);
			minMaxLoc(match_output, &min_val, &max_val, &min_idx, &max_idx);

			Point match_idx;
			match_idx.x = min_idx.x + sx + width / 2;
			match_idx.y = min_idx.y + sy + height / 2;
			corners_next[i + 1][j] = match_idx;
		}
		findFundamentalMat(corners_next[i], corners_next[i + 1], CV_FM_RANSAC,3.,0.99,pt_status[i]);
	}

	Mat keepers;
	bitwise_and(pt_status[0], pt_status[1], keepers);
	for (int i = 2; i < 5; i++) {
		bitwise_and(pt_status[i], keepers, keepers);
	}


	cvtColor(parallel_cube[0], out[0], CV_GRAY2BGR);
	cvtColor(parallel_cube[5], out[1], CV_GRAY2BGR);

	uchar *kpt = keepers.ptr<uchar>(0);

	for (int j = 0; j < corners_next[5].size(); j++) {
		if (kpt[j] == 0)
			continue;
		line(out[0], corners_next[0][j], corners_next[5][j], Scalar(0, 0, 255), 2);
	}
	for (int j = 0; j < corners_next[0].size(); j++){
		if (kpt[j] == 0)
			continue;
		circle(out[0], corners_next[0][j], 1, Scalar(0, 255, 0), -1);
	}

	
	for (int j = 0; j < corners_next[5].size(); j++){
		if (kpt[j] == 0)
			continue;
		circle(out[1], corners_next[5][j], 1, Scalar(0, 255, 0), -1);
	}

	char full_path_out[50];
	sprintf(full_path_out,"%s1.bmp", path_out);
	imwrite(full_path_out, out[0]);
	sprintf(full_path_out, "%s2.bmp", path_out);
	imwrite(full_path_out, out[1]);
}
