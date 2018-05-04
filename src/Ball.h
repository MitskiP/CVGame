#ifndef BALL_H
#define BALL_H

#include "opencv2/opencv.hpp"

#define DEFAULT_BALL_RADIUS 25
#define GEN_ROTATION_SPEED_MAX 20

#define TTL_DISABLED 999999999999

#define RELATIVE_ET 30.0 // if elapsed time == RELATIVE_ET, then move exact pixels

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
	
	double ttl;
	int damage;

	float mod360(float);

	Point2d movedPos(double et) { return pos+vel*et/RELATIVE_ET; }
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
	void newRotationSpeed(float f) { newRotationSpeed(); rotationSpeed *= f; }
	bool countTTL(double d) { if (ttl == TTL_DISABLED) return false; ttl -= d; return ttl <= 0; }
	void setTTL(double d) { ttl = d; }
	int getDamage() { return damage; }
	void setDamage(int r) { damage = r; }
	
	double getMass() { return radius; }

	void resolveCollision(Ball&);

	void accelerate(double et, double g) { vel.y -= g*et/RELATIVE_ET; }
	void friction(double g) { vel.x *= g; vel.y *= g; rotationSpeed *= g; }
	void move(double et) { pos = movedPos(et); rotation = mod360(rotation + rotationSpeed*et/RELATIVE_ET); }
};

#endif
