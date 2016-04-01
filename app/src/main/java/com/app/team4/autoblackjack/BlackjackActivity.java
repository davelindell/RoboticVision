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

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.PowerManager;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.SurfaceView;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfPoint;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.Point;
import org.opencv.core.RotatedRect;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;
import org.opencv.core.Core;

import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.lang.Math;

public class BlackjackActivity extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private static final String  TAG = "OCV::Activity";
    private CameraBridgeViewBase mOpenCvCameraView;

    protected PowerManager.WakeLock mWakeLock;

    private static final int VIEW_MODE_RGBA = 0;
    private static final int VIEW_MODE_GRAY = 1;

    private int mViewMode = VIEW_MODE_RGBA;

    private Mat mRgba;
    private Mat mIntermediateMat;
    private Mat mGray;
    private int divideIdx;
    private Blackjack blackjack;

    private int num_cards_detected = 0;
    private Mat current_frame;

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
        blackjack = new Blackjack();
//        ActionBar.LayoutParams layoutParams = new ActionBar.LayoutParams(700,1800);
//        mOpenCvCameraView.setLayoutParams(layoutParams);
        //playBlackjack();

        /* This code together with the one in onDestroy()
         * will make the screen be always on until this Activity gets destroyed. */
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        this.mWakeLock = pm.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK, "My Tag");
        this.mWakeLock.acquire();
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
        mRgba = inputFrame.rgba();

        List<Mat> cards = new ArrayList<>();
        List<Point> centers = new ArrayList<>();
        detectCards(mRgba, cards, centers);
        current_frame = mRgba;

        int new_num_cards_detected = cards.size();
//        if (new_num_cards_detected == num_cards_detected) {
//            return current_frame;
//        }

        num_cards_detected = new_num_cards_detected;
        List<String> player_cards = new ArrayList<>();
        List<String> dealer_cards = new ArrayList<>();
        List<Card> player_values = new ArrayList<>();
        List<Card> dealer_values = new ArrayList<>();
        identifyCards(cards, centers, player_cards, player_values, dealer_cards, dealer_values);


        Card dealer_card = dealer_values.get(0);
        blackjack.decide(player_values,dealer_card);

        int x_pos = divideIdx + 100;
        for (int i = 0; i < player_cards.size(); i++) {
            Imgproc.putText(mRgba,player_cards.get(i),new Point(x_pos+i*100,100),Core.FONT_HERSHEY_PLAIN,2,new Scalar(255,0,0),3);
        }
        x_pos = 100;
        for (int i = 0; i < dealer_cards.size(); i++) {
            Imgproc.putText(mRgba,dealer_cards.get(i),new Point(x_pos+i*100,100),Core.FONT_HERSHEY_PLAIN,2,new Scalar(255,0,0),3);
        }
        return mRgba;

    }

    public void detectCards(Mat mRgba, List<Mat> cards, List<Point> centers) {
        // for testing, load in a static image
//        String path = String.format("template/test2.png");
//
//        try {
//            InputStream is = getAssets().open(path);
//            Bitmap bm = BitmapFactory.decodeStream(is);
//            Utils.bitmapToMat(bm, mRgba);
//        } catch (java.io.IOException e) {
//            System.out.println("Error reading image file");
//        }
        // end testing

        Mat gray = new Mat();
        Imgproc.cvtColor(mRgba, gray, Imgproc.COLOR_RGBA2GRAY);

        int x_mid = mRgba.cols()/2;
        int y_end = mRgba.rows()-1;
        divideIdx = x_mid;

        // Draw a line across the screen for dealer/player
        Point line_pt1 = new Point(x_mid,0);
        Point line_pt2 = new Point(x_mid,y_end);
        Imgproc.line(mRgba, line_pt1, line_pt2, new Scalar(255, 0, 0), 20);

        // Some image preprocessing
        Mat blur = new Mat();
        Mat th = new Mat();
        Imgproc.GaussianBlur(gray,blur,new Size(11,11),5);
        Imgproc.threshold(blur, th, 150, 255, Imgproc.THRESH_BINARY);
        List<MatOfPoint> contours = new ArrayList<>();

        // Find the contours
        Mat tmp = new Mat();
        Mat hierarchy = new Mat();
        th.copyTo(tmp);

        // find contours (hopefully each one is a card)
        Imgproc.findContours(tmp, contours, hierarchy,Imgproc.RETR_EXTERNAL, Imgproc.CHAIN_APPROX_SIMPLE);
        Imgproc.drawContours(mRgba,contours,-1,new Scalar(0,0,255),10);

        // iterate through the contours
        for (int i = 0; i < contours.size(); i++) {
            MatOfPoint2f cur_contour = new MatOfPoint2f();
            contours.get(i).convertTo(cur_contour, CvType.CV_32FC2);
            double perim = Imgproc.arcLength(cur_contour,true);

            MatOfPoint2f card_pts_2f = new MatOfPoint2f();
            Imgproc.approxPolyDP(cur_contour, card_pts_2f, 0.02 * perim, true);
            Mat card_pts = matOfPtToMat(card_pts_2f);

            if (card_pts.rows() != 4) {
                continue;
            }

            RotatedRect rect = Imgproc.minAreaRect(card_pts_2f);
            Mat boundbox_pts = new Mat();
            Imgproc.boxPoints(rect, boundbox_pts);

            // assign center points to Mat for ease of processing
            Mat center_pts = new Mat(4,2,card_pts.type());
            for (int j = 0; j < 4; j++) {
                center_pts.put(j,0,rect.center.x);
                center_pts.put(j,1,rect.center.y);
            }
            // translate the card points to center abt origin
            Mat translated_pts = new Mat();
            Core.subtract(card_pts,center_pts,translated_pts);

            // rotate card points
            double ang;
            if (rect.size.width > rect.size.height)
                ang = -(rect.angle+90)*3.14159/180;
            else
                ang = -rect.angle*3.14159/180;

            Mat r = new Mat(2,2,card_pts.type());
            r.put(0, 0, Math.cos(ang));
            r.put(0, 1, -Math.sin(ang));
            r.put(1, 0, Math.sin(ang));
            r.put(1, 1, Math.cos(ang));

            Mat transformed_pts = new Mat(2,4,card_pts.type());
            Core.gemm(r, translated_pts, 1, new Mat(), 0, transformed_pts, Core.GEMM_2_T);
            transformed_pts = transformed_pts.t();

            // align the corner points with the desired corner points
            int[] px = new int[4];
            int[] py = new int[4];
            for (int j = 0; j < 4; j++) {
                if (transformed_pts.get(j,0)[0] > 0 && transformed_pts.get(j,1)[0] > 0) {
                    px[j] = 449;
                    py[j] = 449;
                }
                else if (transformed_pts.get(j,0)[0] > 0 && transformed_pts.get(j,1)[0] < 0) {
                    px[j] = 449;
                    py[j] = 0;
                }
                else if (transformed_pts.get(j,0)[0] < 0 && transformed_pts.get(j,1)[0] > 0) {
                    px[j] = 0;
                    py[j] = 449;
                }
                else if (transformed_pts.get(j,0)[0] < 0 && transformed_pts.get(j,1)[0] < 0) {
                    px[j] = 0;
                    py[j] = 0;
                }
                else {
                    System.out.println("Error setting points.");
                }
            }

            Mat truth_pts = new Mat(4,2,card_pts.type());
            for (int j = 0; j < 4; j++) {
                truth_pts.put(j,0,px[j]);
                truth_pts.put(j,1,py[j]);
            }

            Mat transform = Imgproc.getPerspectiveTransform(card_pts,truth_pts);
            Mat out = new Mat();
            Imgproc.warpPerspective(th, out, transform, new Size(450, 450));
            cards.add(out);
            Point center = new Point(rect.center.x,rect.center.y);
            centers.add(center);
        }

        return;
    }

    public void identifyCards(List<Mat> cards, List<Point> centers, List<String> player_cards,
              List<Card>player_values,List<String> dealer_cards, List<Card> dealer_values) {

        double min = 1e10;
        int idx= 0;
        for (int c = 0; c < cards.size(); c++) {
            for (int m = 0; m < 2; m++) {
                for (int i = 1; i < 53; i++) {
                    Mat diff = new Mat();
                    Mat template = new Mat();
                    String path = String.format("template/%02d.bmp", i);
                    try {
                        InputStream is = getAssets().open(path);
                        Bitmap bm = BitmapFactory.decodeStream(is);
                        Utils.bitmapToMat(bm, template);
                    } catch (java.io.IOException e) {
                        System.out.println("Error reading image file");
                    }
                    Imgproc.cvtColor(template,template,Imgproc.COLOR_RGBA2GRAY);
                    Core.absdiff(cards.get(c), template, diff);
                    Scalar pixel_diff = Core.sumElems(diff);
                    if (pixel_diff.val[0] < min) {
                        min = pixel_diff.val[0];
                        idx = i;
                    }
                }
                Mat unflipped = cards.get(c);
                Mat flipped = new Mat();
                Core.flip(unflipped, flipped, -1);
                cards.set(c, flipped);
            }
            if (centers.get(c).x <= divideIdx) { //dealer
                dealer_cards.add(getCardIDString(idx));
                dealer_values.add(Card.valueOf(getCardIDString(idx)));
            }
            else { // player
                player_cards.add(getCardIDString(idx));
                player_values.add(Card.valueOf(getCardIDString(idx)));
            }
        }
    }

    public String getCardIDString(int id) {
        String str = new String();
        switch(id) {
            case 1: str = "C2"; break;
            case 2: str = "C3"; break;
            case 3: str = "C4"; break;
            case 4: str = "C5"; break;
            case 5: str = "C6"; break;
            case 6: str = "C7"; break;
            case 7: str = "C8"; break;
            case 8: str = "C9"; break;
            case 9: str = "C10"; break;
            case 10: str = "CJ"; break;
            case 11: str = "CQ"; break;
            case 12: str = "CK"; break;
            case 13: str = "CA"; break;
            case 14: str = "S2"; break;
            case 15: str = "S3"; break;
            case 16: str = "S4"; break;
            case 17: str = "S5"; break;
            case 18: str = "S6"; break;
            case 19: str = "S7"; break;
            case 20: str = "S8"; break;
            case 21: str = "S9"; break;
            case 22: str = "S10"; break;
            case 23: str = "SJ"; break;
            case 24: str = "SQ"; break;
            case 25: str = "SK"; break;
            case 26: str = "SA"; break;
            case 27: str = "D2"; break;
            case 28: str = "D3"; break;
            case 29: str = "D4"; break;
            case 30: str = "D5"; break;
            case 31: str = "D6"; break;
            case 32: str = "D7"; break;
            case 33: str = "D8"; break;
            case 34: str = "D9"; break;
            case 35: str = "D10"; break;
            case 36: str = "DJ"; break;
            case 37: str = "DQ"; break;
            case 38: str = "DK"; break;
            case 39: str = "DA"; break;
            case 40: str = "H2"; break;
            case 41: str = "H3"; break;
            case 42: str = "H4"; break;
            case 43: str = "H5"; break;
            case 44: str = "H6"; break;
            case 45: str = "H7"; break;
            case 46: str = "H8"; break;
            case 47: str = "H9"; break;
            case 48: str = "H10"; break;
            case 49: str = "HJ"; break;
            case 50: str = "HQ"; break;
            case 51: str = "HK"; break;
            case 52: str = "HA"; break;
        }
        return str;
    }

    public Mat matOfPtToMat(MatOfPoint2f mat) {
        Mat out = new Mat(mat.rows(), 2, CvType.CV_32FC1);
        for (int i = 0; i < mat.rows(); i++) {
            double x = mat.get(i,0)[0];
            double y = mat.get(i,0)[1];
            out.put(i,0,x);
            out.put(i,1,y);
        }
        return out;
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
        this.mWakeLock.release();
    }



    public class Blackjack {

        private TextToSpeech tts = new TextToSpeech(getApplicationContext(), new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) {
            }
        });

        public void main(String[] args) {
            List<Card> cards = new ArrayList<Card>();

            cards.add(Card.S7);
            cards.add(Card.DA);
            cards.add(Card.C5);
            //cards.add(Card.HA);
            //cards.add(Card.SA);

            decide(cards,Card.S3);
        }

        public void decide(List<Card> cards, Card dealer){
            int card_count=0;
            int aces=0;
            for (Card c : cards){
                if(c.equals(Card.SA) || c.equals(Card.CA) || c.equals(Card.HA) || c.equals(Card.DA)){
                    aces++;
                    card_count=card_count+c.value1;
                }
                else card_count=card_count+c.value1;
            }
            if(aces==0){
                no_ace(card_count,dealer);
            }
            else{
                if(card_count-aces*11>10) {
                    card_count=card_count-aces*10;
                    no_ace(card_count,dealer);
                }
                else {
                    card_count=card_count-(aces-1)*10;
                    ace(card_count,dealer);
                }
            }

        }

        public void no_ace(int card_count,Card dealer){
            if(card_count<12) hit();
            else if(card_count==12){
                if(dealer.value1<4 || dealer.value1>6) hit();
                else stand();
            }
            else if(card_count>12 && card_count<17){
                if(dealer.value1<7) stand();
                else hit();
            }
            else stand();
        }

        public void ace(int card_count,Card dealer){
            if(card_count<18) hit();
            else if(card_count==18){
                if(dealer.value1<9) stand();
                else hit();
            }
            else stand();
        }

        public void hit(){
            System.out.println("hit");
            tts.speak("I would like to hit, please.",TextToSpeech.QUEUE_FLUSH, null);
        }

        public void stand(){
            System.out.println("stand");
            tts.speak("I would like to stand, please.", TextToSpeech.QUEUE_FLUSH, null);
        }

    }



}