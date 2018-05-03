#ifndef PHYSICS_H
#define PHYSICS_H

#include "opencv2/opencv.hpp"
#include "Ball.h"
#include "Hand.h"

#define TOTAL_DIRECTIONS 60
#define BLOOD_AMOUNT 5
#define BLOOD_SPREAD 6

using namespace cv;
using namespace std;

class Physics {
private:
	bool initialized;
	bool disappearingBalls;
	bool enableKoike, enableSpin, enableBlood;
	int MAX_BALL_COUNT;
	int standardBallsCount;
	vector<Ball> balls;
	double total_time;
	double last_ball_creation_time;
	
	int w, h;
	double gravity, friction;
	int botBorderHeight;

	Mat face;

	void generateBall();
	bool killBall(int);
	void drawOverlay(Mat&, Mat&, Point);
	Mat rotate(Mat&, double);

	int findDirection(int[]);
	float mod2PI(float);
	bool inRange(float, float, float);
	bool mirrorPointAtStraightLine(Point2d&, float);

public:
	Physics();
	void init(int, Mat, int, bool, bool, bool, bool);
	bool isInitialized() { return initialized; }
	void tick(double, Mat&, vector<Hand>&);
	Mat &draw(Mat&);
	
	int dist(int x1, int y1, int x2, int y2) { return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)); }
};

#endif
