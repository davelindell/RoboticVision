//package com.app.team4.autoblackjack;
//
//import android.app.Activity;
//import android.hardware.Camera;
//import android.os.Bundle;
//import android.util.Log;
//import android.view.View;
//import android.widget.FrameLayout;
//import android.widget.ImageButton;
//
//public class BlackjackActivity extends Activity {
//    private Camera mCamera = null;
//    private CameraPreview mCameraView = null;
//
//    protected void onCreate(Bundle savedInstanceState) {
//        super.onCreate(savedInstanceState);
//        setContentView(R.layout.activity_blackjack);
//
//        try{
//            mCamera = Camera.open();//you can use open(int) to use different cameras
//        } catch (Exception e){
//            Log.d("ERROR", "Failed to get camera: " + e.getMessage());
//        }
//
//        if(mCamera != null) {
//            mCameraView = new CameraPreview(this, mCamera);//create a SurfaceView to show camera data
//            FrameLayout camera_view = (FrameLayout)findViewById(R.id.FrameLayout);
//            camera_view.addView(mCameraView);//add the SurfaceView to the layout
//        }
//
//        //btn to close the application
////        ImageButton imgClose = (ImageButton)findViewById(R.id.imgClose);
////        imgClose.setOnClickListener(new View.OnClickListener() {
////            @Override
////            public void onClick(View view) {
////                System.exit(0);
////            }
////        });
//    }
//}

package com.app.team4.autoblackjack;

import android.app.ActionBar;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ImageView;
import android.hardware.camera2.CameraManager;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfPoint;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;
import org.opencv.core.Core;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

public class BlackjackActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private static final String  TAG = "OCV::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    private static final int VIEW_MODE_RGBA = 0;
    private static final int VIEW_MODE_GRAY = 1;

    private int mViewMode = VIEW_MODE_RGBA;

    private Mat mRgba;
    private Mat mIntermediateMat;
    private Mat mGray;

    private BaseLoaderCallback  mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.i(TAG, "called onCreate");
        super.onCreate(savedInstanceState);

        //requestWindowFeature(Window.FEATURE_NO_TITLE);
        //getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_blackjack);

        mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.opencv_java_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);
        mOpenCvCameraView.setCameraIndex(0);
        mOpenCvCameraView.setMaxFrameSize(1080, 1920);
//        ActionBar.LayoutParams layoutParams = new ActionBar.LayoutParams(700,1800);
//        mOpenCvCameraView.setLayoutParams(layoutParams);
        //playBlackjack();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.d(TAG, "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_1_0, this, mLoaderCallback);
        } else {
            Log.d(TAG, "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }

    @Override
    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat(height, width, CvType.CV_8UC4);
        mIntermediateMat = new Mat(height, width, CvType.CV_8UC4);
        mGray = new Mat(height, width, CvType.CV_8UC1);
    }

    @Override
    public void onCameraViewStopped() {
        mRgba.release();
        mIntermediateMat.release();
        mGray.release();
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        final int viewMode = mViewMode;

        Mat out;
        switch (viewMode){
            case VIEW_MODE_RGBA:
                mRgba = inputFrame.rgba();
                Imgproc.cvtColor(mRgba, mRgba, Imgproc.COLOR_RGBA2GRAY);
                //Mat mRgbaT = mRgba.t();
                //Core.flip(mRgba.t(), mRgbaT, 1);
                //Imgproc.resize(mRgbaT, mRgbaT, mRgba.size());

//                Core.flip(mRgba, out, 1);
//                out = out.t();
                return mRgba;

            case VIEW_MODE_GRAY:
                out = new Mat();
                mGray = inputFrame.gray();
                mGray = mGray.t();
                return out;
        }
        return new Mat();
    }


    public void playBlackjack() {

        for (int i = 1; i < 53; i++) {

            String path = String.format("template/%02d.bmp", i);
            Mat im = new Mat();

            try {
                InputStream is = getAssets().open(path);
                Bitmap bm = BitmapFactory.decodeStream(is);
                int width = bm.getWidth();
                int height = bm.getHeight();

                Utils.bitmapToMat(bm, im);
            } catch (java.io.IOException e) {
                System.out.println("Error reading image file");
            }

            Mat gray = new Mat();
            Mat blur = new Mat();
            Imgproc.cvtColor(im, gray, Imgproc.COLOR_BGR2GRAY);
            Imgproc.GaussianBlur(gray, blur, new Size(9, 9), 3);
            Imgproc.threshold(blur, blur, 120, 255, Imgproc.THRESH_BINARY);

            List<MatOfPoint> contours = new ArrayList<MatOfPoint>();
            Mat hierarchy = new Mat();
            Imgproc.findContours(blur, contours, hierarchy, Imgproc.RETR_LIST, Imgproc.CHAIN_APPROX_NONE);

        }
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }
}