#include "opencv2/opencv.hpp"
#include <chrono>
 
using namespace cv;
using namespace std;

using namespace std::chrono;

int main(int argc, char** argv) {
	// Start default camera
	VideoCapture video(0);

	// With webcam get(CAP_PROP_FPS) does not work.
	// Let's see for ourselves.

	double fps = video.get(CAP_PROP_FPS);
	// If you do not care about backward compatibility
	// You can use the following instead for OpenCV 3
	// double fps = video.get(CAP_PROP_FPS);
	cout << "Frames per second using video.get(CAP_PROP_FPS) : " << fps << endl;


	// Number of frames to capture
	int num_frames = 120;

	// Start and end times
	high_resolution_clock::time_point start_frame;
	high_resolution_clock::time_point end_frame;

	// Variable for storing video frames
	Mat frame;

	cout << "Capturing " << num_frames << " frames" << endl ;

	// Start time
	start_frame = high_resolution_clock::now();

	// Grab a few frames
	for(int i = 0; i < num_frames; i++) {
		video >> frame;
	}

	// End Time
	end_frame = high_resolution_clock::now();

	// Time elapsed
	duration<double, std::milli> time_span = end_frame - start_frame;
	cout << "Time taken : " << time_span.count()/1000 << " seconds" << endl;

	// Calculate frames per second
	fps  = num_frames*1000 / time_span.count();
	cout << "Estimated frames per second : " << fps << endl;

	// Release video
	video.release();
	return 0;
}
