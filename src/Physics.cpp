#include "Physics.h"

Physics::Physics() {
	initialized = false;
}
void Physics::init(int frameDuration, Mat frame, int mbc, bool db, bool ek, bool es, bool eb) {
	w = frame.cols;
	h = frame.rows;
//	Ball b = Ball(Point2d(90, h/2), Point2d(0.0, 10.0));
//	Ball b2 = Ball(Point2d(180, h/2), Point2d(0.0, 10.0));
//	balls.push_back(b);
//	balls.push_back(b2);
	initialized = true;
	total_time = 0;
	last_ball_creation_time = 0;
	standardBallsCount = 0;
	gravity = -0.5;
	friction = 0.995; // percentage
	disappearingBalls = db;
	enableKoike = ek;
	enableSpin = es;
	enableBlood = eb;
	MAX_BALL_COUNT = mbc;
	
	botBorderHeight = 100;
	
	// load face image
	face = imread("res/face.png", IMREAD_UNCHANGED);
	float factor = (float)DEFAULT_BALL_RADIUS*2/face.rows;
	resize(face, face, Size(), factor, factor);
}
void Physics::generateBall() {
	if (standardBallsCount < MAX_BALL_COUNT) {
		if (total_time - last_ball_creation_time > standardBallsCount*3000/MAX_BALL_COUNT) {
			last_ball_creation_time = total_time;
			Ball b = Ball(Point2d(rand()%(w-2*DEFAULT_BALL_RADIUS)+DEFAULT_BALL_RADIUS, rand()%(h/2-2*DEFAULT_BALL_RADIUS)+DEFAULT_BALL_RADIUS),
					Point2d(rand()%10, rand()%10));
			b.newRotationSpeed();
			balls.push_back(b);
			standardBallsCount++;
		}
	}
}
bool Physics::killBall(int i) {
	if (balls[i].getType() != BallType::DEFAULT)
		return false;
	if (disappearingBalls) {
		if (enableBlood) {
			for (int k = 0; k < BLOOD_AMOUNT; k++) {
				Point2d dv = Point2d(rand()%BLOOD_SPREAD, rand()%BLOOD_SPREAD);
				Ball b = Ball(balls[i].getPos(), balls[i].getVel() + dv);
				b.setRadius(DEFAULT_BALL_RADIUS/2);
				b.setColor(Scalar(0, 0, 255));
				b.setType(BallType::BLOOD);
				balls.push_back(b);
			}
		}
		balls.erase(balls.begin() + i);
		standardBallsCount--;
		//last_ball_creation_time = total_time;
		return true;
	}
	return false;
}
void Physics::tick(double elapsedTime, Mat &labelMask, vector<Hand> &hands) {
	total_time += elapsedTime;
	generateBall();
	for (unsigned int i = 0; i < balls.size(); i++) {
		balls[i].accelerate(gravity);
		balls[i].friction(friction);
		balls[i].move();
		
		// collision on borders
		double x = balls[i].getPos().x;
		double y = balls[i].getPos().y;
		int r = balls[i].getRadius();
		if (x - r < 0) {
			balls[i].getPos().x = r+1;
			balls[i].getVel().x = max(balls[i].getVel().x, -balls[i].getVel().x);
			balls[i].newRotationSpeed();
		} else if (x + r >= w) {
			balls[i].getPos().x = w-r-1;
			balls[i].getVel().x = min(balls[i].getVel().x, -balls[i].getVel().x);
			balls[i].newRotationSpeed();
		}
		if (y - r < 0) {
			balls[i].getPos().y = r+1;
			balls[i].getVel().y = max(balls[i].getVel().y, -balls[i].getVel().y);
			balls[i].newRotationSpeed();
		} else if (balls[i].getType() == BallType::DEFAULT && y + r >= h - botBorderHeight) {
			balls[i].getPos().y = h-r-botBorderHeight-1;
			balls[i].getVel().y = min(balls[i].getVel().y, -balls[i].getVel().y);
			if (killBall(i)) {
				i--;
				continue;
			}
			balls[i].newRotationSpeed();
		} else if (balls[i].getType() == BallType::BLOOD && y - r >= h) {
			balls.erase(balls.begin() + i);
			i--;
			continue;
		}
		
		// collision between balls
		for(size_t j = i + 1; j < balls.size(); j++) {
			if (norm(balls[i].getPos() - balls[j].getPos()) < balls[i].getRadius() + balls[j].getRadius()) {
				balls[i].resolveCollision(balls[j]);
				balls[i].newRotationSpeed();
				balls[j].newRotationSpeed();
			}
		}
		
		// collision with human
		int label = labelMask.at<int>(y, x);
		if (label == 0)
			label = labelMask.at<int>(y+r, x);
		if (label == 0)
			label = labelMask.at<int>(y, x+r);
		if (label == 0)
			label = labelMask.at<int>(y-r, x);
		if (label == 0)
			label = labelMask.at<int>(y, x-r);
		if (label == 0)
			label = labelMask.at<int>(y+r*0.71, x+r*0.71);
		if (label == 0)
			label = labelMask.at<int>(y-r*0.71, x+r*0.71);
		if (label == 0)
			label = labelMask.at<int>(y+r*0.71, x-r*0.71);
		if (label == 0)
			label = labelMask.at<int>(y-r*0.71, x-r*0.71);

		if (label == 0) {
			balls[i].setColliding(false);
			continue;
		} //else if (balls[i].isColliding())
		//	continue;

		// find correct hand
		unsigned int h;
		for (h = 0; h < hands.size(); h++) {
			if (hands[h].getLabel() == label)
				break;
		}
		if (h >= hands.size())
			continue;

		balls[i].setColliding(true);

		int direction[TOTAL_DIRECTIONS];
		for (int k = 0; k < TOTAL_DIRECTIONS; k++)
			direction[k] = 0;
		int total_px = 0;
		int relevant_px = 0;
		for (int ly = y-r; ly < y+r; ly++) {
			for (int lx = x-r; lx < x+r; lx++) {
				if (dist(x, y, lx, ly) <= r) {
					total_px++;
					if (labelMask.at<int>(ly, lx) == label) {
						int index = (int) (((int)fastAtan2(ly-y, lx-x)+90)%360)*TOTAL_DIRECTIONS/360;
//		printf("%d\n", index);
						direction[index]++;
						relevant_px++;
					}
				}
			}
		}
		//if (relevant_px > 0.9 * total_px)
		if (relevant_px == total_px) {
			if (killBall(i))
				i--;
			continue;
		}

		int max = findDirection(direction);
		
		//int count = 0;
		//for (int k = 0; k < TOTAL_DIRECTIONS; k++) {
		//	if (direction[k] == direction[max])
		//		count++;
		//}
		//printf("direction: %d with %d,  %d\n", max*360/TOTAL_DIRECTIONS, direction[max], count);
		//exit(0);
		Point2d vel = balls[i].getVel();
		bool changed = mirrorPointAtStraightLine(vel, (float) max * 2 * CV_PI / TOTAL_DIRECTIONS);
		if (changed || (vel.x < hands[h].getVel().x && vel.y < hands[h].getVel().y)) {
			//balls[i].setVel(vel);
			balls[i].setVel(vel + Point2d( (double)hands[h].getVel().x, (double)hands[h].getVel().y  ));
			balls[i].newRotationSpeed();
		}
	}
}
int Physics::findDirection(int direction[]) {
	// find the center of biggest 'collection'
	int sweepWidth = 1;
	bool found = false;
	int maxSum;
	int maxIndex;
	while (!found && sweepWidth <= TOTAL_DIRECTIONS) {
		int sum = 0;
		for (int i = 0; i < sweepWidth; i++)
			sum += direction[(0+i)];
		maxSum = sum;
		maxIndex = 0;
		
		for (int pivot = 1; pivot < TOTAL_DIRECTIONS; pivot++) {
			sum += direction[(pivot+sweepWidth-1)%TOTAL_DIRECTIONS] - direction[(pivot-1)];
			if (sum > maxSum) {
				maxSum = sum;
				maxIndex = pivot;
				found = true;
			} else if (sum == maxSum) {
				found = false;
			}
		}
		sweepWidth++;
	}
	sweepWidth--;
	//printf("found: %s, pivot: %d, swWidth: %d, res.angle: %d, maxSum: %d\n", found?"true":"false", maxIndex, sweepWidth, maxIndex + sweepWidth/2, maxSum);
	return maxIndex + sweepWidth/2;
}
float Physics::mod2PI(float a) {
	float b = a;
	while (b >= 2*CV_PI)
		b -= 2*CV_PI;
	while (b < 0)
		b += 2*CV_PI;
	return b;
}
bool Physics::inRange(float a, float b, float delta) {
	float diff = a - b;
	diff = max(diff, -diff);
	if (diff >= CV_PI) {
		diff -= 2*CV_PI;
		diff = -diff;
	}
	return diff <= delta;
}
bool Physics::mirrorPointAtStraightLine(Point2d &v, float a) {
	float direction = (float)(((int)fastAtan2(v.y, v.x)+90)%360)*2*CV_PI/360;
	direction += CV_PI;
	if (inRange(direction, a, CV_PI/2)) {
		return false;
	}
	// https://de.wikipedia.org/wiki/Spiegelungsmatrix
	Point2d p;
	p.x = cos(2*a) * v.x + sin(2*a) * v.y;
	p.y = sin(2*a) * v.x - cos(2*a) * v.y;
	v = p;
	return true;
}
void Physics::drawOverlay(Mat &orig, Mat &img, Point c) {
	int origcn = orig.channels();
	int imgcn = img.channels();
	Scalar_<uint8_t> bgrPixel;
	c.x -= img.cols/2;
	c.y -= img.rows/2;
	for (int y = 0; y < img.rows; y++) {
	    uint8_t* imgRow = img.row(y).data;
	    uint8_t* origRow = orig.row(c.y+y).data;
		for (int x = 0; x < img.cols; x++) {
			if (imgRow[x*imgcn + 3] > 128) { // check alpha channel
				origRow[(x+c.x)*origcn + 0] = imgRow[x*imgcn + 0]; // B
				origRow[(x+c.x)*origcn + 1] = imgRow[x*imgcn + 1]; // G
				origRow[(x+c.x)*origcn + 2] = imgRow[x*imgcn + 2]; // R
			}
		}
	}
}
Mat &Physics::draw(Mat &canvas) {
	//drawOverlay(canvas, face, Point(w/2, h/2));
	for (unsigned int i = 0; i < balls.size(); i++) {
		if (balls[i].getType() == BallType::DEFAULT && enableKoike) {
			Mat rot;
			if (enableSpin) rot = rotate(face, balls[i].getRotation());
			else rot = face;
			drawOverlay(canvas, rot, balls[i].getPos());
		} else {
			circle(canvas, balls[i].getPos(), balls[i].getRadius(), balls[i].getColor(), -1);
		}
	}
	line(canvas, Point(0, h-botBorderHeight), Point(w, h-botBorderHeight), Scalar(255, 255, 255));
	return canvas;
}
Mat Physics::rotate(Mat &src, double angle) {
	Mat dst;
	Point2f pt(src.cols/2.0, src.rows/2.0);    
	Mat r = getRotationMatrix2D(pt, angle, 1.0);
	warpAffine(src, dst, r, Size(src.cols, src.rows));
	return dst;
}
