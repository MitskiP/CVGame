#include "Ball.h"

Ball::Ball(Point p, Point v) {
	setPos(p);
	setVel(v);
	setRadius(DEFAULT_BALL_RADIUS);
	setColor(Scalar(255, 255, 255));
	rotation = 0;
	rotationSpeed = 0;
	
	ttl = TTL_DISABLED;
	
	type = BallType::DEFAULT;
}

void Ball::resolveCollision(Ball &ball) {
	// get the mtd
	Point2d delta = pos - ball.pos;
	double d = norm(delta);

	if (d == 0.0) {
		// Special case. Balls are exactly on top of eachother.  Don't want to divide by zero.
		d = ball.radius + radius - 1.0;
		delta = Point2d(ball.getRadius() + getRadius(), 0.0);
	}
	double tmp = ((radius + ball.radius) - d) / d;
	Point2d mtd;
	mtd.x = delta.x * tmp; // minimum translation distance to push balls apart after intersecting
	mtd.y = delta.y * tmp;

	// resolve intersection
	double im1 = 1 / getMass(); // inverse mass quantities
	double im2 = 1 / ball.getMass();

	// push-pull them apart
	tmp = im1 / (im1 + im2);
	Point2d tmpP;
	tmpP.x = mtd.x * tmp;
	tmpP.y = mtd.y * tmp;
	pos += tmpP;
	
	tmp = im2 / (im1 + im2);
	tmpP.x = mtd.x * tmp;
	tmpP.y = mtd.y * tmp;
	ball.pos -= tmpP;

	// impact speed
	Point2d v = vel - ball.vel;
	mtd.x = mtd.x / norm(mtd);
	mtd.y = mtd.y / norm(mtd);
	double vn = v.dot(mtd);

	// sphere intersecting but moving away from each other already
	if (vn > 0.0) return;

	// collision impulse
	//double restitution = 0.85;
	double restitution = 0.75;
	double i = (-(1.0 + restitution) * vn) / (im1 + im2);
	Point2d impulse;
	impulse.x = mtd.x * i;
	impulse.y = mtd.y * i;

	// change in momentum
	tmpP.x = impulse.x * im1;
	tmpP.y = impulse.y * im2;
	vel      =      vel + tmpP;
	ball.vel = ball.vel - tmpP;
}
float Ball::mod360(float a) {
	float b = a;
	while (b >= 360)
		b -= 360;
	while (b < 0)
		b += 360;
	return b;
}

