#include "HandTracker.h"

HandTracker::HandTracker() {
}
void HandTracker::update(Mat &frame, bool we, bool wd, bool removeCenterSkin) {
	this->frame = frame;
	calculateSkinMask(we, wd);
	findConnectedComponents(removeCenterSkin);

	generateAppearedHands();
	//printf("\nNEW ROUND appeared %d\n", (int)appearedHands.size());
	findExistingHands();
	removeDisappearedHands();
	addAppearedHands();
}
void HandTracker::addAppearedHands() {
	for (unsigned int p = 0; p < appearedHands.size(); p++) {
		trackedHands.push_back(appearedHands[p]);
		printf("adding hand %d\n", appearedHands[p].id);
	}
}
void HandTracker::removeDisappearedHands() {
	for (unsigned int c = 0; c < trackedHands.size(); c++) {
		if (trackedHands[c].isTracked() == false) {
			printf("lost %d\n", trackedHands[c].id);
			trackedHands.erase(trackedHands.begin() + c);
			c--;
		}
	}
}
void HandTracker::findExistingHands() {
	for (unsigned int c = 0; c < trackedHands.size(); c++)
		trackedHands[c].setTracked(false);
	if (appearedHands.size() == 0)
		return;

	for (unsigned int c = 0; c < trackedHands.size(); c++) {
		Point predictedPos = trackedHands[c].movedPos();
		int bestMatch = 0;
		for (unsigned int p = 1; p < appearedHands.size(); p++) {
			if (appearedHands[p].distance2(predictedPos) < appearedHands[bestMatch].distance2(predictedPos)) {
				bestMatch = p;
			}
		}
		if (appearedHands[bestMatch].distance2(predictedPos) > TRACKING_DISTANCE2_THRESHOLD) {
			// prediction might have failed. try guessing without prediction
			predictedPos = trackedHands[c].getPos();
			int bestMatch = 0;
			for (unsigned int p = 1; p < appearedHands.size(); p++) {
				if (appearedHands[p].distance2(predictedPos) < appearedHands[bestMatch].distance2(predictedPos)) {
					bestMatch = p;
				}
			}
		}

		if (appearedHands[bestMatch].distance2(predictedPos) <= TRACKING_DISTANCE2_THRESHOLD) {
			// found tracking hand
			trackedHands[c].update(appearedHands[bestMatch]);
			//printf("found hand %d\n", trackedHands[c].id);
			appearedHands.erase(appearedHands.begin() + bestMatch);
			if (appearedHands.size() == 0)
				break;
		}
	}
}
void HandTracker::generateAppearedHands() {
	//appearedHands = vector<Hand>();
	appearedHands.clear();
	for (int i = 1; i < nLabels; i++) {
		double x = centroids.at<double>(i, 0);
		double y = centroids.at<double>(i, 1);
		if (x < 0 || y < 0 || x > frame.cols || y > frame.rows)
			continue;
		Hand h = Hand(Point(x, y), i);
		appearedHands.push_back(h);
	}
}
void HandTracker::findConnectedComponents(bool removeCenter) {
	labelMask = Mat(skinMask.size(), CV_32S);
	nLabels = connectedComponentsWithStats(skinMask, labelMask, stats, centroids);
	
	vector<double> centerDist = vector<double>(nLabels);
	//printf("labels: %i\n", nLabels);
	for (int i = 1; i < nLabels; i++) {
		if (stats.at<int>(i, CC_STAT_AREA) < 100*100) {
			removeLabel(i);
			centerDist[i] = numeric_limits<double>::max();
		} else {
			double x = centroids.at<double>(i, 0);
			double y = centroids.at<double>(i, 1);
			centerDist[i] = norm(Point2d(x, y) - Point2d(frame.cols/2, frame.rows/2));
		}
	}

	if (removeCenter) {
		int min = 1;
		for (int i = 2; i < nLabels; i++) {
			if (centerDist[i] < centerDist[min])
				min = i;
		}
		if (nLabels > 1 && centerDist[min] < (labelMask.rows+labelMask.cols)/2/3) {
			removeLabel(min);
		}
	}
}
void HandTracker::removeLabel(int i) {
	int x1 = stats.at<int>(i, CC_STAT_LEFT);
	int x2 = x1 + stats.at<int>(i, CC_STAT_WIDTH);
	int y1 = stats.at<int>(i, CC_STAT_TOP);
	int y2 = y1 + stats.at<int>(i, CC_STAT_HEIGHT);
	int removed = 0;
	int max = stats.at<int>(i, CC_STAT_AREA);
	for (int y = y1; y < y2; y++) {
		for (int x = x1; x < x2; x++) {
			if (labelMask.at<int>(y, x) == i) {
				labelMask.at<int>(y, x) = 0;
				centroids.at<double>(i, 0) = -1; // x
				centroids.at<double>(i, 1) = -1; // y
				removed++;
				if (removed == max)
					break;
			}
		}
		if (removed == max)
			break;
	}
}
//*
Mat &HandTracker::getConnectedComponentsFrame() {
	debugFrame = frame.clone();

	vector<bool> drawLabel(nLabels);
	drawLabel[0] = true;
	for (size_t i = 1; i < drawLabel.size(); i++)
		drawLabel[i] = false;
	for (size_t i = 0; i < trackedHands.size(); i++) {
		//if (norm(trackedHands[i].getVel()) > 2 )
			drawLabel[trackedHands[i].getLabel()] = true;
	}

	vector<Vec3b> colors(nLabels);
	colors[0] = Vec3b(0, 0, 0);
	int c = 1;
	for (int i = 1; i < nLabels; i++) {
		if (drawLabel[i])
			c++;
		int val = c*255/(trackedHands.size()+2); //i*255/nLabels
		colors[i] = Vec3b(val, val, val);
		//colors[i] = Vec3b(rand()&255, rand()&255, rand()&255);
	}
	for (int r = 0; r < skinMask.rows; r++) {
		for (int c = 0; c < skinMask.cols; c++) {
			int label = labelMask.at<int>(r, c);
			Vec3b &pixel = debugFrame.at<Vec3b>(r, c);
			if (drawLabel[label]) {
				pixel = colors[label];
			} else {
				pixel = colors[0];
			}
		}
	}
//	printf("w/h  %i / %i\n", frame.cols, frame.rows);
	for (int i = 1; i < nLabels; i++) {
		if (drawLabel[i]) {
			double x = centroids.at<double>(i, 0);
			double y = centroids.at<double>(i, 1);
			if (x >= 0 && y >= 0) {
				circle(debugFrame, Point(x, y), 3, Scalar(255, 255, 255), -1);
				//Vec3b &pixel = debugFrame.at<Vec3b>(y, x);
				//pixel = Vec3b(255, 255, 255);
			}
		}
	}
	return debugFrame;
}
//*/
/*
Mat &HandTracker::getConnectedComponentsFrame() {
	debugFrame = frame.clone();
	vector<Vec3b> colors(nLabels);
	colors[0] = Vec3b(0, 0, 0);
	for (int i = 1; i < nLabels; i++) {
		colors[i] = Vec3b(i*255/nLabels, i*255/nLabels, i*255/nLabels);
		//colors[i] = Vec3b(rand()&255, rand()&255, rand()&255);
	}
	for (int r = 0; r < skinMask.rows; r++) {
		for (int c = 0; c < skinMask.cols; c++) {
			int label = labelMask.at<int>(r, c);
			Vec3b &pixel = debugFrame.at<Vec3b>(r, c);
			pixel = colors[label];
		}
	}
//	printf("w/h  %i / %i\n", frame.cols, frame.rows);
	for (int i = 1; i < nLabels; i++) {
		double x = centroids.at<double>(i, 0);
		double y = centroids.at<double>(i, 1);
		if (x >= 0 && y >= 0) {
			circle(debugFrame, Point(x, y), 3, Scalar(255, 255, 255), -1);
			//Vec3b &pixel = debugFrame.at<Vec3b>(y, x);
			//pixel = Vec3b(255, 255, 255);
		}
	}
	return debugFrame;
}
//*/
Mat &HandTracker::getSkinFrame() {
	debugFrame.release();
	bitwise_and(frame, frame, debugFrame, skinMask);
	return debugFrame;
}
void HandTracker::calculateSkinMask(bool withErosion, bool withDilation) {
    Scalar lowerBGR = Scalar(20, 40, 95);
    Scalar upperBGR = Scalar(255, 255, 255);

//	skinFrame.release();

//	skinFrame = frame.clone();

    // work on BGR first
    inRange(frame, lowerBGR, upperBGR, skinMaskBGR);

    int h = frame.rows;
    int w = frame.cols;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
        	Vec3b intensity = frame.at<Vec3b>(y, x);
			uchar b = intensity.val[0];
			uchar g = intensity.val[1];
			uchar r = intensity.val[2];
            if ( r <= g || r <= b || abs(r - g) <= 15) {
                skinMaskBGR.at<uchar>(y, x) = 0;
            }
        }
	}
	// HSV
	// OpenCV uses H = 0 - 180, S = 0 - 255, V = 0 - 255
	// 0.0 <= H <= 50.0 and 0.23 <= S <= 0.68 and
	Scalar lowerHSV = Scalar(0, 255 * 0.23, 0);
	Scalar upperHSV = Scalar(0.5 * 50, 255 * 0.68, 255);

	// convert frame to the HSV color space,
	cvtColor(frame, frameHSV, COLOR_BGR2HSV);
	inRange(frameHSV, lowerHSV, upperHSV, skinMaskHSV);

	bitwise_and(skinMaskBGR, skinMaskHSV, skinMask);

	// might help
	//rectangle(skinMask, Point(skinMask.cols/2-20, 0), Point(skinMask.cols/2+20, skinMask.rows), Scalar(0), -1);

	// apply a series of erosions and dilations to the mask
	// using an elliptical kernel
	Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(11, 11));
	if (withErosion)
		erode(skinMask, skinMask, kernel, Point(-1,-1), 1);
	if (withDilation)
		dilate(skinMask, skinMask, kernel, Point(-1,-1), 2);


	// blur the mask to help remove noise, then apply the
	// mask to the frame
	GaussianBlur(skinMask, skinMask, Size(3, 3), 0);
	//GaussianBlur(skinMask, skinMask, Size(51, 51), 0);
}

