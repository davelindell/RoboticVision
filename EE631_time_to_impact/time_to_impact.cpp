#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>


using namespace cv;
using namespace std;
using namespace xfeatures2d;

int task1() {
	int i = 1;
	char path[50];
	sprintf(path, "im\\T%d.jpg", i);
	Mat start_im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
	Ptr<SURF> surf = SURF::create();
	surf->setExtended(true);
	surf->setHessianThreshold(500);

	vector<vector<KeyPoint>> keypoints(18, vector<KeyPoint>(0));
	Mat descriptors[18];
	Mat mask = Mat::ones(Size(start_im.cols, start_im.rows), start_im.type());
	
	surf->detectAndCompute(start_im, mask, keypoints[0], descriptors[0]);

	FlannBasedMatcher matcher;
	vector<vector<DMatch>> matches(18,vector<DMatch>(0));

	for (i = 2; i < 19; i++) {
		path[50];
		sprintf(path, "im\\T%d.jpg", i);
		Mat im = imread(path, CV_LOAD_IMAGE_GRAYSCALE);
		surf->detectAndCompute(im, mask, keypoints[i-1], descriptors[i-1]);
		matcher.match(descriptors[i-2], descriptors[i-1],matches[i-1]);

		Mat out_img;
		drawMatches(start_im, keypoints[i - 2], im, keypoints[i - 1], matches[i - 1], out_img, Scalar(0, 0, 255), Scalar(0, 255, 0));

	}



	return 1;
}


int main(int argc, char **argv) {
	task1();
	return 1;
}