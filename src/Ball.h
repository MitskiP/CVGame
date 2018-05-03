#ifndef BALL_H
#define BALL_H

#include "opencv2/opencv.hpp"

#define DEFAULT_BALL_RADIUS 15
#define GEN_ROTATION_SPEED_MAX 10

using namespace cv;
using namespace std;

enum BallType {
	DEFAULT,
	BLOOD
};

class Ball {
private:
	Point2d pos;
	Point2d vel;
	float rotation;
	float rotationSpeed;
	int radius;
	Scalar color;
	
	BallType type;

	bool colliding;

	float mod360(float);

public:
	Ball(Point, Point);
	Point2d &getPos() { return pos; }
	void setPos(Point2d x) { pos = x; }
	Point2d &getVel() { return vel; }
	void setVel(Point2d x) { vel = x; }
	int getRadius() { return radius; }
	void setRadius(int r) { radius = r; }
	Scalar getColor() { return color; }
	void setColor(Scalar r) { color = r; }
	BallType getType() { return type; }
	void setType(BallType r) { type = r; }
	bool isColliding() { return colliding; }
	void setColliding(bool b) { colliding = b; }
	float getRotation() { return rotation; }
	void setRotation(float d) { rotation = d; }
	double getRotationSpeed() { return rotationSpeed; }
	void setRotationSpeed(double d) { rotationSpeed = d; }
	void newRotationSpeed() { rotationSpeed = rand()%(2*GEN_ROTATION_SPEED_MAX) - (GEN_ROTATION_SPEED_MAX-1); }
	
	double getMass() { return radius; }

	void resolveCollision(Ball&);

	void accelerate(double g) { vel.y -= g; }
	void friction(double g) { vel.x *= g; vel.y *= g; rotationSpeed *= g; }
	Point movedPos() { return pos+vel; }
	void move() { pos = movedPos(); rotation = mod360(rotation + rotationSpeed); }
};

#endif
