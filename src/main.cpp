/**
 * @file objectDetection.cpp
 * @author A. Huaman ( based in the classic facedetect.cpp in samples/c )
 * @brief A simplified version of facedetect.cpp, show how to load a cascade classifier and how to find objects (Face + eyes) in a video stream
 */
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <queue>
#include <stdio.h>

#include "constants.h"
#include "findEyeCenter.h"
#include "findEyeCorner.h"

/** Constants **/


/** Function Headers */
void detectAndDisplay( cv::Mat frame );

/** Global variables */
//-- Note, either copy these two files from opencv/data/haarscascades to your current folder, or change these locations
//cv::String face_cascade_name = "../../../res/haarcascade_frontalface_alt.xml";
cv::String face_cascade_name = "/Users/pelle/dev/opencv_eyetrack/eyeLike/res/haarcascade_frontalface_alt.xml";

cv::CascadeClassifier face_cascade;
std::string main_window_name = "Capture - Face detection";
std::string face_window_name = "Capture - Face";
cv::RNG rng(12345);
cv::Mat debugImage;
cv::Mat skinCrCbHist = cv::Mat::zeros(cv::Size(256, 256), CV_8UC1);

/**
 * @function main
 */
int main( int argc, const char** argv ) {
  CvCapture* capture;
  cv::Mat frame;
  
  //-- 1. Load the cascades
  if( !face_cascade.load( face_cascade_name ) ){ printf("--(!)Error loading\n"); return -1; };
  
  cv::namedWindow(main_window_name,CV_WINDOW_NORMAL);
  cv::moveWindow(main_window_name, 400, 100);
  cv::namedWindow(face_window_name,CV_WINDOW_NORMAL);
  cv::moveWindow(face_window_name, 10, 100);
  cv::namedWindow("Right Eye",CV_WINDOW_NORMAL);
  cv::moveWindow("Right Eye", 10, 600);
  cv::namedWindow("Left Eye",CV_WINDOW_NORMAL);
  cv::moveWindow("Left Eye", 10, 800);
  
  createCornerKernels();
  ellipse(skinCrCbHist, cv::Point(113, 155.6), cv::Size(23.4, 15.2),
          43.0, 0.0, 360.0, cv::Scalar(255, 255, 255), -1);
  
  //-- 2. Read the video stream
  capture = cvCaptureFromCAM( -1 );
  if( capture ) {
    while( true ) {
      frame = cvQueryFrame( capture );
      //printf("Dimensions: %ix%i\n",frame.cols,frame.rows);
      // mirror it
      cv::flip(frame, frame, 1);
      frame.copyTo(debugImage);
      
      //-- 3. Apply the classifier to the frame
      if( !frame.empty() ) {
        detectAndDisplay( frame );
      }
      else {
        printf(" --(!) No captured frame -- Break!");
        break;
      }
      
      imshow(main_window_name,debugImage);
      
      int c = cv::waitKey(10);
      if( (char)c == 'c' ) { break; }
      if( (char)c == 'f' ) {
        imwrite("frame.png",frame);
      }
      
    }
  }
  
  releaseCornerKernels();
  
  return 0;
}

void findEyes(cv::Mat frame_gray, cv::Rect face) {
  cv::Mat faceROI = frame_gray(face);
  
  if (kSmoothFaceImage) {
    double sigma = kSmoothFaceFactor * face.width;
    GaussianBlur( faceROI, faceROI, cv::Size( 0, 0 ), sigma);
  }
  //-- Find eye regions and draw them
  int eye_region_width = face.width * (kEyePercentWidth/100.0);
  int eye_region_height = face.width * (kEyePercentHeight/100.0);
  int eye_region_top = face.height * (kEyePercentTop/100.0);
  cv::Rect leftEyeRegion(face.width*(kEyePercentSide/100.0),
                         eye_region_top,eye_region_width,eye_region_height);
  cv::Rect rightEyeRegion(face.width - eye_region_width - face.width*(kEyePercentSide/100.0),
                          eye_region_top,eye_region_width,eye_region_height);
  
  //-- Find Eye Centers
  cv::Point leftPupil = findEyeCenter(faceROI,leftEyeRegion,"Left Eye");
  cv::Point rightPupil = findEyeCenter(faceROI,rightEyeRegion,"Right Eye");
  // get corner regions
  cv::Rect leftCornerRegion(leftEyeRegion);
  leftCornerRegion.width -= leftPupil.x;
  leftCornerRegion.x += leftPupil.x;
  leftCornerRegion.height /= 2;
  leftCornerRegion.y += leftCornerRegion.height / 2;
  cv::Rect rightCornerRegion(rightEyeRegion);
  rightCornerRegion.width = rightPupil.x;
  rightCornerRegion.height /= 2;
  rightCornerRegion.y += rightCornerRegion.height / 2;
  rectangle(faceROI,leftCornerRegion,200);
  rectangle(faceROI,rightCornerRegion,200);
  // change eye centers to face coordinates
  rightPupil.x += rightEyeRegion.x;
  rightPupil.y += rightEyeRegion.y;
  leftPupil.x += leftEyeRegion.x;
  leftPupil.y += leftEyeRegion.y;
  // draw eye centers
  circle(faceROI, rightPupil, 3, 1234);
  circle(faceROI, leftPupil, 3, 1234);
  
  //-- Find Eye Corners
  if (kEnableEyeCorner) {
    cv::Point leftCorner = findEyeCorner(faceROI(leftCornerRegion), false);
    leftCorner.x += leftCornerRegion.x;
    leftCorner.y += leftCornerRegion.y;
    cv::Point rightCorner = findEyeCorner(faceROI(leftCornerRegion), false);
    rightCorner.x += rightCornerRegion.x;
    rightCorner.y += rightCornerRegion.y;
    circle(faceROI, rightCorner, 3, 200);
    circle(faceROI, leftCorner, 3, 200);
  }
  
  imshow(face_window_name, faceROI);
}


cv::Mat findSkin (cv::Mat &frame) {
  cv::Mat input;
  cv::Mat output = cv::Mat(frame.rows,frame.cols, CV_8U);
  
  cvtColor(frame, input, CV_BGR2YCrCb);
  
  for (int y = 0; y < input.rows; ++y) {
    const cv::Vec3b *Mr = input.ptr<cv::Vec3b>(y);
//    uchar *Or = output.ptr<uchar>(y);
    cv::Vec3b *Or = frame.ptr<cv::Vec3b>(y);
    for (int x = 0; x < input.cols; ++x) {
      cv::Vec3b ycrcb = Mr[x];
//      Or[x] = (skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) > 0) ? 255 : 0;
      if(skinCrCbHist.at<uchar>(ycrcb[1], ycrcb[2]) == 0) {
        Or[x] = cv::Vec3b(0,0,0);
      }
    }
  }
  return output;
}

/**
 * @function detectAndDisplay
 */
void detectAndDisplay( cv::Mat frame ) {
  std::vector<cv::Rect> faces;
//  cv::Mat frame_gray;
  
  std::vector<cv::Mat> rgbChannels(3);
  cv::split(frame, rgbChannels);
  cv::Mat frame_gray = rgbChannels[2];
  
//  cvtColor( frame, frame_gray, CV_BGR2GRAY );
  //equalizeHist( frame_gray, frame_gray );
  //cv::pow(frame_gray, 0.5, frame_gray);
  //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT, cv::Size(150, 150) );
//  findSkin(debugImage);
  
  for( int i = 0; i < faces.size(); i++ )
  {
    rectangle(debugImage, faces[i], 1234);
  }
  //-- Show what you got
  if (faces.size() > 0) {
    findEyes(frame_gray, faces[0]);
  }
}
