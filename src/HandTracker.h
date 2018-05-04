#ifndef HANDTRACKER_H
#define HANDTRACKER_H

#include <stdio.h>
#include <vector>
#include "opencv2/opencv.hpp"
#include "Hand.h"

using namespace cv;
using namespace std;

class HandTracker {
private:
	static const int TRACKING_DISTANCE2_THRESHOLD = 20*20;

	vector<Hand> appearedHands;
	vector<Hand> trackedHands;

	Mat frame;
	void calculateSkinMask(bool, bool);
	void removeLabel(int);
	void findConnectedComponents(bool);
	void generateAppearedHands();
	void findExistingHands();
	void removeDisappearedHands();
	void addAppearedHands();

	int nLabels;
	Mat labelMask, stats, centroids;
	Mat skinMask;

	Mat debugFrame;
	Mat frameHSV;
	Mat skinMaskBGR, skinMaskHSV;
public:
	HandTracker();
	void update(Mat&, bool, bool, bool);
	Mat &getSkinFrame();
	Mat &getConnectedComponentsFrame();
	
	Mat &getLabelMask() { return labelMask; }
	vector<Hand> &getTrackedHands() { return trackedHands; }
};

#endif
