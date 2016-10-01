#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <fstream>

using namespace cv;
using namespace std;
using namespace xfeatures2d;

int task1() {
	int i = 0;
	char path[50];
	sprintf(path, "im\\T%d.jpg", i+1);
	Mat start_im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
	Ptr<SURF> surf = SURF::create();
	surf->setExtended(true);
	surf->setHessianThreshold(500);

	vector<vector<KeyPoint>> keypoints(19, vector<KeyPoint>(0));
	Mat descriptors[19];
	Mat mask = Mat::ones(Size(start_im.cols, start_im.rows), start_im.type());
	FlannBasedMatcher matcher;
	vector<vector<DMatch>> matches(19,vector<DMatch>(0));

	double ox = 320;
	double oy = 240;
	Point2f origin = Point(ox, oy);
	Mat tau = Mat::zeros(16,1,CV_32F);
	Mat A = Mat::zeros(16, 2, CV_32F);

	for (i = 0; i < 16; i++) {
		path[50];
		sprintf(path, "im\\T%d.jpg", i + 1);
		Mat im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
		surf->detectAndCompute(im, mask, keypoints[i], descriptors[i]);

		sprintf(path, "im\\T%d.jpg", i + 3);
		im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
		surf->detectAndCompute(im, mask, keypoints[i+3], descriptors[i+3]);

		matcher.match(descriptors[i], descriptors[i+3],matches[i]);

		//Mat out_img;
		//drawMatches(start_im, keypoints[i - 1], im, keypoints[i], matches[i], out_img, Scalar(0, 0, 255), Scalar(0, 255, 0));
		//start_im = im;

		double a = 0;
		int count = 0;
		for (int j = 0; j < matches[i].size(); j++) {
			if (matches[i][j].distance < .2) {
				int idx1 = matches[i][j].queryIdx;
				int idx2 = matches[i][j].trainIdx;
				KeyPoint k1 = keypoints[i][idx1];
				KeyPoint k2 = keypoints[i+3][idx2];
				double d = norm(k1.pt - origin);
				double d1 = norm(k2.pt - origin);
				a += d1 / d;
				count++;
			}
		}
		a = a / count;
		tau.at<float>(i, 0) = 2* a / (a - 1);
		A.at<float>(i, 0) = i;
		A.at<float>(i, 1) = 1;

	}

	Mat fit = (A.t()*A).inv()*A.t()*tau;
	float time_to_impact = -fit.at<float>(1, 0) / fit.at<float>(0, 0);
	ofstream ofs;

	FileStorage fs_out("task1.yml", FileStorage::WRITE);
	fs_out << "A" << A << "tau" << tau << "time" << time_to_impact << "fit" << fit;
	fs_out.release();

	ofs.open("task1.txt");
	ofs << A << endl << tau << endl << time_to_impact << endl << fit;
	ofs.close();

	return 1;
}

void task2() {
	FileStorage fs("task1.yml", FileStorage::READ);
	Mat A, tau, tau1;
	fs["A"] >> A;
	fs["tau"] >> tau;

	tau1 = tau * 15.25;
	tau1.convertTo(tau1, CV_32F);
	A.convertTo(A, CV_32F);
	Mat fit = (A.t()*A).inv()*A.t()*tau1;
	float dist_to_impact = -fit.at<float>(1, 0) / fit.at<float>(0, 0);

	FileStorage fs_out("task2.yml", FileStorage::WRITE);
	fs_out << "A" << A << "tau1" << tau1 << "dist" << dist_to_impact << "fit" << fit;
	fs_out.release();

	ofstream ofs;
	ofs.open("task2.txt");
	ofs << A << endl << tau1 << endl << dist_to_impact << endl << fit;
	ofs.close();
	return;
}

void task3() {
	vector<float> width(0);
	vector<float> dist(0);
	const int can_width = 59;
	const int f = 825;


	Mat im;
	Mat roi;
	Mat roi_edges;
	for (int i = 0; i < 18; i++) {
		char path[50];
		sprintf(path, "im\\T%d.jpg", i + 1);
		im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
		roi = im(Rect(200, 300, 300, 50));
		roi_edges;
		Mat roi_blur;
		GaussianBlur(roi, roi_blur, Size(9, 9), 2,2);
		inRange(roi_blur, 0, 85, roi_blur);
		Canny(roi_blur, roi_edges, 50, 200);
		
		vector<int> edges(0);
		for (int j = 0; j < roi_edges.cols; j++) {
			uchar val = roi_edges.at<uchar>(15, j);
			if (val > 0) {
				edges.push_back(j);
			}
		}
		int start = edges[0];
		int end = edges[edges.size()-1];
		width.push_back(end - start + 1);
	}

	for (int i = 0; i < width.size(); i++) {
		dist.push_back(f * 59 / width[i]);
	}

	ofstream ofs;
	ofs.open("task3.txt");
	for (int i = 0; i < dist.size(); i++) {
		ofs << dist[i] << ";" << endl;
	}
	ofs.close();

	return;
}

int main(int argc, char **argv) {
	task3();
	return 1;
}