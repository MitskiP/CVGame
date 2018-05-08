#include "Physics.h"

Physics::Physics() {
}
void Physics::init(int width, int height, int mbc, bool db, bool ek, bool es, bool eb, bool game) {
	balls.clear();

	w = width;
	h = height;
	total_time = 0;
	last_ball_creation_time = 0;
	standardBallsCount = 0;
	gravity = -0.5;
	friction = 0.990; // percentage
	disappearingBalls = db;
	enableKoike = ek;
	enableSpin = es;
	enableBlood = eb;
	MAX_BALL_COUNT = mbc;
	isGame = game;
	
	botBorderHeight = 100;
	finish = false;
	
	if (isGame)
		botBorderHeight = 15;
	
	// load face image
	string paths[] = { "res/face2.png", "res/facem.png" };
	for (size_t i = 0; i < sizeof(paths)/sizeof(paths[0]); i++) {
		Mat face = imread(paths[i], IMREAD_UNCHANGED);
		float factor = (float)DEFAULT_BALL_RADIUS*2/face.rows;
		resize(face, face, Size(), factor, factor);
		faces.push_back(face);
	}
	ballTexture = imread("res/ball.png", IMREAD_UNCHANGED);
	float factor = (float)DEFAULT_BALL_RADIUS*2/ballTexture.rows;
	resize(ballTexture, ballTexture, Size(), factor, factor);
	
	updateSpikes();
}
void Physics::updateSpikes() {
	int ww = w / NUM_SPIKES;
	for (int i = 0; i < NUM_SPIKES; i++) {
		spikePoints[i][0] = Point(i*ww + ww/2, h - botBorderHeight);
		spikePoints[i][1] = Point(i*ww, h);
		spikePoints[i][2] = Point((i+1)*ww, h);
	}
}
void Physics::generateBall(bool safeMode, bool enableMP) {
	if (standardBallsCount < MAX_BALL_COUNT) {
		if (total_time - last_ball_creation_time > standardBallsCount*3000/MAX_BALL_COUNT) {
			last_ball_creation_time = total_time;
			Ball b = Ball(Point2d(rand()%(w-2*DEFAULT_BALL_RADIUS)+DEFAULT_BALL_RADIUS, rand()%(h/4-2*DEFAULT_BALL_RADIUS)+DEFAULT_BALL_RADIUS),
					Point2d(rand()%10, rand()%10));
			b.newRotationSpeed();
			if (safeMode) {
				b.setFace(ballTexture);
			} else {
				int s = 0;
				if (enableMP && faces.size() > 1 && (rand()%10) > 7) // 20%
					s = rand()%(faces.size()-1) + 1;
				b.setFace(faces[s]);
			}
			balls.push_back(b);
			standardBallsCount++;
		}
	}
}
Ball Physics::generateBlood(int i, float factor, bool safeMode) {
	Point2d dv = Point2d(rand()%BLOOD_SPREAD, rand()%BLOOD_SPREAD);
	Ball b = Ball(balls[i].getPos(), balls[i].getVel());
	b.getVel().x = b.getVel().x*factor + dv.x;
	b.getVel().y = b.getVel().y*factor + dv.y;
	b.setRadius(DEFAULT_BALL_RADIUS/3);
	if (safeMode)
		b.setColor(Scalar(255, 255, 0));
	else
		b.setColor(Scalar(0, 0, 255));
	b.setType(BallType::BLOOD);
	b.setTTL(BLOOD_TTL);
	b.setDamage(DAMAGE_PER_BLOOD);
	return b;
}
bool Physics::killBall(int i, bool safeMode) {
	if (disappearingBalls) {
		if (enableBlood && balls[i].getType() == BallType::DEFAULT) {
			for (int k = 0; k < BLOOD_AMOUNT; k++) {
				Ball b = generateBlood(i, (float)4/5, safeMode);
				balls.push_back(b);
			}
		}
		if (balls[i].getType() == BallType::DEFAULT)
			standardBallsCount--;
		balls.erase(balls.begin() + i);
		//last_ball_creation_time = total_time;
		return true;
	}
	return false;
}
void Physics::tick(double elapsedTime, Mat &labelMask, vector<Hand> &hands, bool enableMP, bool safeMode) {
	if (finish)
		return;
	total_time += elapsedTime;
	generateBall(safeMode, enableMP);
	for (unsigned int i = 0; i < balls.size(); i++) {
		if (balls[i].countTTL(elapsedTime)) {
			if (killBall(i, safeMode)) {
				i--;
				continue;
			}
		}

		balls[i].accelerate(elapsedTime, gravity);
		balls[i].friction(friction);
		balls[i].move(elapsedTime);

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
			if (killBall(i, safeMode)) {
				i--;
				continue;
			}
			balls[i].newRotationSpeed();
		} else if (balls[i].getType() == BallType::BLOOD && y - r >= h) {
			if (isGame) {
				botBorderHeight += balls[i].getDamage();
				if (botBorderHeight < 0)
					botBorderHeight = 0;
				updateSpikes();
				if (botBorderHeight >= h) {
					botBorderHeight = h;
					finish = true;
					return;
				}
			}
			balls.erase(balls.begin() + i);
			i--;
			continue;
		}
		
		// collision between balls
		for(size_t j = i + 1; j < balls.size(); j++) {
			if (norm(balls[i].getPos() - balls[j].getPos()) < balls[i].getRadius() + balls[j].getRadius()) {
				balls[i].resolveCollision(balls[j]);
				if (balls[i].getType() == BallType::DEFAULT && balls[j].getType() == BallType::DEFAULT) {
					balls[i].newRotationSpeed(5.0f);
					balls[j].newRotationSpeed(5.0f);
					if (isGame) {
						printf("GENERATE BONUS\n");
						Ball b = generateBlood(i, 0.0f, safeMode);
						b.setColor(Scalar(0, 255, 0));
						b.setDamage(DAMAGE_PER_BLOOD*-5);
						balls.push_back(b);
					}
				} else {
					balls[i].newRotationSpeed();
					balls[j].newRotationSpeed();
				}
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
			if (killBall(i, safeMode))
				i--;
			continue;
		}

		int max = findDirection(direction);
		
		// change velocity
		Point2d vel = balls[i].getVel();
		bool changed = mirrorPointAtStraightLine(vel, (float) max * 2 * CV_PI / TOTAL_DIRECTIONS);
		if (changed || (vel.x < hands[h].getVel().x && vel.y < hands[h].getVel().y)) {
			//balls[i].setVel(vel);
			balls[i].setVel(vel + Point2d( (double)hands[h].getVel().x, (double)hands[h].getVel().y  ));
			balls[i].newRotationSpeed();
		}
		
		// forcefully change position
		int forceTo = (max + TOTAL_DIRECTIONS/4) % TOTAL_DIRECTIONS;
		float forceToPI = forceTo * 2*CV_PI / TOTAL_DIRECTIONS;
		vel = balls[i].getPos();
		Point2d force = Point2d(cos(forceToPI), sin(forceToPI)) * 2*balls[i].getRadius();
		balls[i].setPos(vel + force * relevant_px / total_px);
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
Mat &Physics::draw(Mat &canvas, bool safeMode) {
	if (enableBlood /*&& !safeMode*/) {
		for (int i = 0; i < NUM_SPIKES; i++) {
			fillConvexPoly(canvas, spikePoints[i], 3, Scalar(255, 255, 255));
		}
	} else {
		line(canvas, Point(0, h-botBorderHeight), Point(w, h-botBorderHeight), Scalar(255, 255, 255));
	}

	//drawOverlay(canvas, face, Point(w/2, h/2));
	for (unsigned int i = 0; i < balls.size(); i++) {
		if (balls[i].getType() == BallType::DEFAULT && enableKoike) {
			Mat rot = balls[i].getFace();
			if (enableSpin)
				rot = rotate(rot, balls[i].getRotation());
			drawOverlay(canvas, rot, balls[i].getPos());
		} else {
			circle(canvas, balls[i].getPos(), balls[i].getRadius(), balls[i].getColor(), -1);
		}
	}
//	drawGameOverOverlay(canvas);
	return canvas;
}
Mat &Physics::drawGameOverOverlay(Mat &canvas) {
	int duration = total_time;
	string t = to_string(duration/1000);
	putText(canvas, t, Point(0, 35), FONT_HERSHEY_PLAIN, 3, Scalar::all(255), 2);
	if (finish) {
		rectangle(canvas, Point(0, h/4), Point(w, h*3/4), Scalar(0, 0, 0), -1);
		string dtext = to_string(duration/1000) + "." + to_string(duration%1000) + " seconds!";

		// https://docs.opencv.org/2.4/modules/core/doc/drawing_functions.html#gettextsize
		string text = "Game Over";
		int fontFace = FONT_HERSHEY_SCRIPT_COMPLEX;
		double fontScale = 2;
		int thickness = 3;

		int baseline1 = 0;
		int baseline2 = 0;
		Size textSize1 = getTextSize(text, fontFace, fontScale, thickness, &baseline1);
		Size textSize2 = getTextSize(dtext, fontFace, fontScale, thickness, &baseline2);
		baseline1 += thickness;
		baseline2 += thickness;

		Point textOrg1((w - textSize1.width)/2, h/2 - textSize1.height + baseline1/2);
		Point textOrg2((w - textSize2.width)/2, h/2 + textSize2.height + baseline2/2);
		//rectangle(canvas, textOrg + Point(0, baseline), textOrg + Point(textSize.width, -textSize.height), Scalar(0,0,255));
		putText(canvas, text, textOrg1, fontFace, fontScale, Scalar::all(255), thickness, 8);
		putText(canvas, dtext, textOrg2, fontFace, fontScale, Scalar::all(255), thickness, 8);

	}
	
	return canvas;
}
Mat Physics::rotate(Mat &src, double angle) {
	Mat dst;
	Point2f pt(src.cols/2.0, src.rows/2.0);    
	Mat r = getRotationMatrix2D(pt, angle, 1.0);
	warpAffine(src, dst, r, Size(src.cols, src.rows));
	return dst;
}
