#include <ctime>
#include <ratio>
#include <chrono>
#include "HandTracker.h"
#include "Physics.h"

using namespace std::chrono;

static const int FRAME_DURATION = 30;

int main(int args, char** argv) {
	VideoCapture cap(0); // open the default camera
	if(!cap.isOpened())  // check if we succeeded
		return -1;

	/*
	--max-ball-count 5
	--spawn-remove-balls
	--enable-koike
	--enable-spin
	--enable-gore
	*/

	int displayWidth = 1920, displayHeight = 1080;
	bool enableSpin = false;
	bool enableKoike = false;
	bool removeCenterSkin = true;
	bool disappearingBalls = false;
	bool enableBlood = false;
	int MAX_BALL_COUNT = 1;
	
	bool withErosion = false, withDilation = true;
	
	for (int i = 1; i < args; i++) {
		printf("%s, cmp %d\n", argv[i], strcmp(argv[i], "--allow-center-skin"));
		if (strcmp(argv[i], "--allow-center-skin") == 0) {
			removeCenterSkin = false;
		} else if (strcmp(argv[i], "--hold-balls") == 0) {
			disappearingBalls = false;
		} else if (strcmp(argv[i], "--spawn-remove-balls") == 0) {
			disappearingBalls = true;
		} else if (strcmp(argv[i], "--enable-koike") == 0) {
			enableKoike = true;
		} else if (strcmp(argv[i], "--enable-spin") == 0) {
			enableSpin = true;
			enableKoike = true;
		} else if (strcmp(argv[i], "--enable-gore") == 0) {
			disappearingBalls = true;
			enableKoike = true;
			enableSpin = true;
			enableBlood = true;
			if (MAX_BALL_COUNT < 3)
				MAX_BALL_COUNT = 5;
		} else if (strcmp(argv[i], "--max-ball-count") == 0 && i+1 < args) {
			MAX_BALL_COUNT = atoi(argv[(i++)+1]);
		} else if (strcmp(argv[i], "-w") == 0 && i+1 < args) {
			displayWidth = atoi(argv[(i++)+1]);
		} else if (strcmp(argv[i], "-h") == 0 && i+1 < args) {
			displayHeight = atoi(argv[(i++)+1]);
		} else if (strcmp(argv[i], "--with-erosion") == 0) {
			withErosion = true;
		} else if (strcmp(argv[i], "--without-erosion") == 0) {
			withErosion = false;
		} else if (strcmp(argv[i], "--with-dilation") == 0) {
			withDilation = true;
		} else if (strcmp(argv[i], "--without-dilation") == 0) {
			withDilation = false;
		}
	}


	HandTracker ht = HandTracker(withErosion, withDilation);
	Physics world;

	Mat display;
	namedWindow("edges",1);
	#ifdef RELEASE
	namedWindow("presentation",1);
	#endif
	
	Mat border;
	
	int target_fps = 30;
	
	int skipFrames = 1;
	double frame_time = 1000.0/target_fps;
	int sleep;
	high_resolution_clock::time_point start_frame;
	high_resolution_clock::time_point end_frame;
	for (;;) {
		start_frame = high_resolution_clock::now();

		Mat frame, game;
		cap >> frame; // get a new frame from camera
		flip(frame, frame, 1);
		game = frame.clone();
		
		if (!world.isInitialized())
			world.init(FRAME_DURATION, frame, MAX_BALL_COUNT, disappearingBalls, enableKoike, enableSpin, enableBlood);

		ht.update(frame, removeCenterSkin);
		world.tick(frame_time, ht.getLabelMask(), ht.getTrackedHands());
		world.draw(game);
		
		//cvtColor(frame, display, COLOR_BGR2GRAY);
		//GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
		//Canny(edges, edges, 0, 30, 3);
		if (skipFrames == 1) {
			border = Mat(frame.rows, 1, frame.type());
			for (int i = 0; i < frame.rows; i++)
				border.at<Vec3b>(i, 0) = Vec3b(255, 255, 255);
		}
		
//		display = frame;
//		hconcat(display, border, display);
//		hconcat(display, ht.getSkinFrame(), display);
		display = ht.getSkinFrame();
		hconcat(display, border, display);
		hconcat(display, world.draw(ht.getConnectedComponentsFrame()), display);
//		display = ht.getConnectedComponentsFrame();
		hconcat(display, border, display);
		hconcat(display, game, display);
//		display = ht.getConnectedComponentsFrame();
//		resize(display, display, Size(), 2.0, 2.0);
		putText(display, to_string(sleep) + "ms", Point(display.cols-game.cols, 40),  FONT_HERSHEY_PLAIN, 3.0, Scalar(255, 255, 255), 2);
		imshow("edges", display);
		#ifdef RELEASE
		//resize(game, game, Size(), 2.2, 2.2);
		float displayFactor = min((float)displayHeight / game.rows, (float)displayWidth / game.cols);
		flip(game, game, 1);
		resize(game, game, Size(), displayFactor, displayFactor);
		imshow("presentation", game);
		#endif

		end_frame = high_resolution_clock::now();
		duration<double, std::milli> time_span = end_frame - start_frame;
		sleep = frame_time - time_span.count();
 		if (skipFrames > 0) {
			skipFrames--;
		} else {
			if (sleep > 0) {
				int val = waitKey(sleep);
				if(val == 32 || val == 27 || val == 113) { // space esc q
					printf("key = %d\n", val);
					break;
				}
			} else {
				printf("too many frames expected ----------------------\n");
				printf("we are behind by %dms\n", sleep);
				waitKey(1);
				//exit(1);
			}
		}
	}
	// the camera will be deinitialized automatically in VideoCapture destructor
	return 0;
}
