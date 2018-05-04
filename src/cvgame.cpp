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

	int displayWidth = 1920, displayHeight = 1080;
	bool enableSpin = false;
	bool enableKoike = false;
	bool removeCenterSkin = true;
	bool disappearingBalls = false;
	bool enableBlood = false;
	int MAX_BALL_COUNT = 1;
	bool isGame = false;
	
	bool withErosion = false, withDilation = true;
	
	// parse command line options
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


	HandTracker ht = HandTracker();
	Physics world;

	Mat display;
	namedWindow("edges",1);
	#ifdef RELEASE
	namedWindow("presentation",1);
	#endif
	
	Mat border;
	bool is_first_frame = true;
	
	int sleep;
	high_resolution_clock::time_point start_time;
	high_resolution_clock::time_point end_time;
	duration<double, std::milli> time_span;

	high_resolution_clock::time_point last_frame_time = high_resolution_clock::now();
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
		
		// initialize world
		if (!world.isInitialized())
			world.init(frame, MAX_BALL_COUNT, disappearingBalls, enableKoike, enableSpin, enableBlood, isGame);
		// handle tick
		ht.update(frame, withErosion, withDilation, removeCenterSkin);
		world.tick(frame_duration.count(), ht.getLabelMask(), ht.getTrackedHands());
		// draw world onto game
		world.draw(game);

		// initialize mat border
		if (border.rows == 0) {
			border = Mat(frame.rows, 1, frame.type());
			for (int i = 0; i < frame.rows; i++)
				border.at<Vec3b>(i, 0) = Vec3b(255, 255, 255);
		}
		
		// create mat to show
		//display = frame;
		display = ht.getSkinFrame();
		hconcat(display, border, display);
		hconcat(display, world.draw(ht.getConnectedComponentsFrame()), display);
		hconcat(display, border, display);
		hconcat(display, game, display);
		//resize(display, display, Size(), 2.0, 2.0);
		// show the frame duration for both camera thread and this thread (of last time)
		putText(display, to_string((int)time_span.count()) + " vs " + to_string((int)frame_duration.count()), Point(display.cols-game.cols, 40),  FONT_HERSHEY_PLAIN, 3.0, Scalar(255, 255, 255), 2);
		imshow("edges", display);

		// create presentation mat
		#ifdef RELEASE
		// resize presentation Mat to screen height or width
		float displayFactor = min((float)displayHeight / game.rows, (float)displayWidth / game.cols);
		flip(game, game, 1);
		world.drawGameOverOverlay(game);
		resize(game, game, Size(), displayFactor, displayFactor);
		imshow("presentation", game);
		#endif

		end_time = high_resolution_clock::now();
		time_span = end_time - start_time;
		// calculate remaining time to sleep in order to match camera's fps
		sleep = frame_duration.count() - time_span.count() - 1; // substract 1 so we are always faster than camera
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
				case 32:  // space
				case 27:  // esc
				case 113: // q
					cap.release();
					break;
				case 114: // r
					world.init(frame, MAX_BALL_COUNT, disappearingBalls, enableKoike, enableSpin, enableBlood, isGame);
					break;
				case 99: // c
					removeCenterSkin = !removeCenterSkin;
					break;
				case 101: // e
					withErosion = !withErosion;
					break;
				case 100: // d
					withDilation = !withDilation;
					break;
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
