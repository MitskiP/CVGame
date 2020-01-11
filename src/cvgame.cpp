#include <ctime>
#include <ratio>
#include <chrono>
#include <thread>
#include "HandTracker.h"
#include "Physics.h"
#include "SharedQueue.h"
#include "SharedVideoCapture.h"

using namespace std::chrono;

class Page {
public:
	Page(Mat m, high_resolution_clock::time_point t) { frame = m; time = t; }
	Mat frame;
	high_resolution_clock::time_point time;
};

SharedVideoCapture cap(0); // open the default camera
SharedQueue<Page> frames;

void cameraThread() {
	Mat frame;
	while (cap.isOpened()) {
		cap.read(frame);
		frames.push_back(Page(frame.clone(), high_resolution_clock::now()));
	}
	cap.release();
}

int main(int args, char** argv) {
	if(!cap.isOpened())  // check if we succeeded
		return -1;

	/*
	--max-ball-count 5
	--spawn-remove-balls
	--enable-koike
	--enable-spin
	--enable-gore
	*/

	#ifdef RELEASE
	int displayWidth = 1920, displayHeight = 1080;
	#endif
	bool enableSpin = false;
	bool enableKoike = false;
	bool removeCenterSkin = true;
	bool disappearingBalls = false;
	bool enableBlood = false;
	int MAX_BALL_COUNT = 1;
	bool isGame = false;
	
	bool withErosion = false, withDilation = true;

	bool trackHands = true;
	bool trackingDemo = false;
	
	bool enableMP = false;
	bool safeMode = true;
	
	// parse command line options
	for (int i = 1; i < args; i++) {
		printf("%s, cmp %d\n", argv[i], strcmp(argv[i], "--allow-center-skin"));
		if (strcmp(argv[i], "--allow-center-skin") == 0) {
			removeCenterSkin = false;
		} else if (strcmp(argv[i], "--tracking-demo") == 0) {
			removeCenterSkin = false;
			trackHands = false;
			trackingDemo = true;
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
		#ifdef RELEASE
		} else if (strcmp(argv[i], "-w") == 0 && i+1 < args) {
			displayWidth = atoi(argv[(i++)+1]);
		} else if (strcmp(argv[i], "-h") == 0 && i+1 < args) {
			displayHeight = atoi(argv[(i++)+1]);
		#endif
		} else if (strcmp(argv[i], "--with-erosion") == 0) {
			withErosion = true;
		} else if (strcmp(argv[i], "--without-erosion") == 0) {
			withErosion = false;
		} else if (strcmp(argv[i], "--with-dilation") == 0) {
			withDilation = true;
		} else if (strcmp(argv[i], "--without-dilation") == 0) {
			withDilation = false;
		} else if (strcmp(argv[i], "--enable-mp") == 0) {
			enableMP = true;
		} else if (strcmp(argv[i], "--game") == 0) {
			disappearingBalls = true;
			enableKoike = true;
			enableSpin = true;
			enableBlood = true;
			if (MAX_BALL_COUNT < 3)
				MAX_BALL_COUNT = 5;
			isGame = true;
		}
	}

	int w = cap.get(CAP_PROP_FRAME_WIDTH);
	int h = cap.get(CAP_PROP_FRAME_HEIGHT);
	
    // initialize mat border
	Mat border = Mat(h, 1, cap.get(CAP_PROP_FORMAT));
	for (int i = 0; i < h; i++)
		border.at<Vec3b>(i, 0) = Vec3b(255, 255, 255);
	
	// initialize world
	HandTracker ht = HandTracker();
	Physics world;
	world.init(w, h, MAX_BALL_COUNT, disappearingBalls, enableKoike, enableSpin, enableBlood, isGame);

	#ifdef RELEASE
	float displayFactor = min((float)displayHeight / h, (float)displayWidth / w);
	#endif

	Mat display;
	namedWindow("edges",1);
	#ifdef RELEASE
	if (!trackingDemo)
		namedWindow("presentation",1);
	#endif

	// prepare required variables for the main loop
	bool is_first_frame = true;
	int sleep;
	high_resolution_clock::time_point start_time;
	high_resolution_clock::time_point end_time;
	duration<double, std::milli> time_span;
	high_resolution_clock::time_point last_frame_time = high_resolution_clock::now();

	// start camera thread
    thread t1(cameraThread);

	while (cap.isOpened()) {

		if (frames.size() == 0) {
			//printf("waiting for new frame\n");
			continue;
		}

		// start timer and read next frame with timestamp
		start_time = high_resolution_clock::now();
		if (frames.size() > 1) {
			printf("SKIPPING %d frames\n", frames.size()-1);
			while (frames.size() > 2)
				frames.pop_front();
			last_frame_time = frames.front().time;
			frames.pop_front();
		}
		Page p = frames.front();
		frames.pop_front();
		duration<double, std::milli> frame_duration = p.time - last_frame_time;
		last_frame_time = p.time;

		// prepare frame
		Mat frame = p.frame;
		flip(frame, frame, 1);
		Mat game = frame.clone();
		
		// handle tick
		ht.update(frame, withErosion, withDilation, removeCenterSkin);
		world.tick(frame_duration.count(), ht.getLabelMask(), ht.getTrackedHands(), enableMP, safeMode);
		// draw world onto game
		if (!trackingDemo)
			world.draw(game, safeMode);

		// create mat to show
		//display = frame;
		display = ht.getSkinFrame();
		hconcat(display, border, display);
		Mat componentsFrame = ht.getConnectedComponentsFrame(trackHands);
		if (!trackingDemo)
			componentsFrame = world.draw(componentsFrame, safeMode);
		hconcat(display, componentsFrame, display);
		if (!trackingDemo) {
			hconcat(display, border, display);
			hconcat(display, game, display);
			//resize(display, display, Size(), 2.0, 2.0);
			// show the frame duration for both camera thread and this thread (of last time)
			putText(display, to_string((int)time_span.count()) + " vs " + to_string((int)frame_duration.count()), Point(display.cols-game.cols, 40),  FONT_HERSHEY_PLAIN, 3.0, Scalar(255, 255, 255), 2);
		}
		imshow("edges", display);

		// create presentation mat
		#ifdef RELEASE
		if (!trackingDemo) {
			// resize presentation Mat to screen height or width
			flip(game, game, 1);
			world.drawGameOverOverlay(game);
			resize(game, game, Size(), displayFactor, displayFactor);
			imshow("presentation", game);
		}
		#endif

		end_time = high_resolution_clock::now();
		time_span = end_time - start_time;
		// calculate remaining time to sleep in order to match camera's fps
		sleep = frame_duration.count() - time_span.count() - 1; // substract 1 so we are always ahead of the camera
		//printf("sleep = %d ms\n", sleep);
		if (is_first_frame) {
			sleep = 1;
			is_first_frame = false;
		}
		if (sleep > 0) {
			int val = waitKey(sleep);
			if (val > 0) {
				printf("key = %d\n", val);
				switch (val) {
				case ' ':
				case 27:  // esc
				case 'q':
					cap.release();
					break;
				case 'r':
					world.init(w, h, MAX_BALL_COUNT, disappearingBalls, enableKoike, enableSpin, enableBlood, isGame);
					break;
				case 'c':
					removeCenterSkin = !removeCenterSkin;
					break;
				case 'e':
					withErosion = !withErosion;
					break;
				case 'd':
					withDilation = !withDilation;
					break;
				case 't':
					trackHands = !trackHands;
					break;
				case 'm':
					enableMP = !enableMP;
					break;
				case 's':
					safeMode = !safeMode;
				}
			}
		} else {
			printf("too many frames expected ----------------------\n");
			printf("we are behind by %d ms\n", sleep);
			waitKey(1);
			//exit(1);
		}
	}
	// the camera will be deinitialized automatically in VideoCapture destructor
	t1.join();
	return 0;
}
