
#include <iostream>
#include <algorithm>
#include <numeric>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camFusion.hpp"
#include "dataStructures.h"
#include <unordered_map>

using namespace std;

static constexpr int MIN_MATCHES = 50;

// Create groups of Lidar points whose projection into the camera falls into the same bounding box
void clusterLidarWithROI(std::vector<BoundingBox> &boundingBoxes, std::vector<LidarPoint> &lidarPoints, float shrinkFactor, cv::Mat &P_rect_xx, cv::Mat &R_rect_xx, cv::Mat &RT)
{
    // loop over all Lidar points and associate them to a 2D bounding box
    cv::Mat X(4, 1, cv::DataType<double>::type);
    cv::Mat Y(3, 1, cv::DataType<double>::type);

    for (auto it1 = lidarPoints.begin(); it1 != lidarPoints.end(); ++it1)
    {
        // assemble vector for matrix-vector-multiplication
        X.at<double>(0, 0) = it1->x;
        X.at<double>(1, 0) = it1->y;
        X.at<double>(2, 0) = it1->z;
        X.at<double>(3, 0) = 1;

        // project Lidar point into camera
        Y = P_rect_xx * R_rect_xx * RT * X;
        cv::Point pt;
        // pixel coordinates
        pt.x = Y.at<double>(0, 0) / Y.at<double>(2, 0); 
        pt.y = Y.at<double>(1, 0) / Y.at<double>(2, 0); 

        vector<vector<BoundingBox>::iterator> enclosingBoxes; // pointers to all bounding boxes which enclose the current Lidar point
        for (vector<BoundingBox>::iterator it2 = boundingBoxes.begin(); it2 != boundingBoxes.end(); ++it2)
        {
            // shrink current bounding box slightly to avoid having too many outlier points around the edges
            cv::Rect smallerBox;
            smallerBox.x = (*it2).roi.x + shrinkFactor * (*it2).roi.width / 2.0;
            smallerBox.y = (*it2).roi.y + shrinkFactor * (*it2).roi.height / 2.0;
            smallerBox.width = (*it2).roi.width * (1 - shrinkFactor);
            smallerBox.height = (*it2).roi.height * (1 - shrinkFactor);

            // check wether point is within current bounding box
            if (smallerBox.contains(pt))
            {
                enclosingBoxes.push_back(it2);
            }

        } // eof loop over all bounding boxes

        // check wether point has been enclosed by one or by multiple boxes
        if (enclosingBoxes.size() == 1)
        { 
            // add Lidar point to bounding box
            enclosingBoxes[0]->lidarPoints.push_back(*it1);
        }

    } // eof loop over all Lidar points
}

/* 
* The show3DObjects() function below can handle different output image sizes, but the text output has been manually tuned to fit the 2000x2000 size. 
* However, you can make this function work for other sizes too.
* For instance, to use a 1000x1000 size, adjusting the text positions by dividing them by 2.
*/
void show3DObjects(std::vector<BoundingBox> &boundingBoxes, cv::Size worldSize, cv::Size imageSize, bool bWait)
{
    // create topview image
    cv::Scalar colorTab[] =
    {
        cv::Scalar(0, 0, 255),
        cv::Scalar(0,255,0),
        cv::Scalar(255,100,100),
        cv::Scalar(255,0,255),
        cv::Scalar(0,255,255)
    };
    
    cv::Mat topviewImg(imageSize, CV_8UC3, cv::Scalar(255, 255, 255));

    for(auto it1=boundingBoxes.begin(); it1!=boundingBoxes.end(); ++it1)
    {
        // create randomized color for current 3D object
        cv::RNG rng(it1->boxID);
        cv::Scalar currColor = cv::Scalar(rng.uniform(0,150), rng.uniform(0, 150), rng.uniform(0, 150));

        // plot Lidar points into top view image
        int top=1e8, left=1e8, bottom=0.0, right=0.0; 
        float xwmin=1e8, xwmax = 0, ywmin=1e8, ywmax=-1e8;

        std::vector<cv::Point2f> points;
        points.reserve(it1->lidarPoints.size());
        int clusterSize = 0;
        for (auto it2 = it1->lidarPoints.begin(); it2 != it1->lidarPoints.end(); ++it2)
        {
            // world coordinates
            float xw = (*it2).x; // world position in m with x facing forward from sensor
            float yw = (*it2).y; // world position in m with y facing left from sensor
            xwmin = xwmin<xw ? xwmin : xw;
            xwmax = xwmax > xw ? xwmax : xw;
            ywmin = ywmin<yw ? ywmin : yw;
            ywmax = ywmax>yw ? ywmax : yw;

            // top-view coordinates
            int y = (-xw * imageSize.height / worldSize.height) + imageSize.height;
            int x = (-yw * imageSize.width / worldSize.width) + imageSize.width / 2;

            // find enclosing rectangle
            top = top<y ? top : y;
            left = left<x ? left : x;
            bottom = bottom>y ? bottom : y;
            right = right>x ? right : x;

            cv::Point2f point(x,y);
            points.push_back(point);
            // draw individual point
          
            cv::circle(topviewImg, cv::Point(x, y), 4, currColor, -1);
        }
       
        // draw enclosing rectangle
        cv::rectangle(topviewImg, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 0, 0), 2);

        // augment object with some key data
        char str1[200], str2[200];
        sprintf(str1, "id=%d, #pts=%d", it1->boxID, (int)it1->lidarPoints.size());
        putText(topviewImg, str1, cv::Point2f(left - 250, bottom + 50), cv::FONT_ITALIC, 2, currColor);
        sprintf(str2, "xmin=%2.2f m, yw=%2.2f m", xwmin, ywmax - ywmin);
        putText(topviewImg, str2, cv::Point2f(left - 250, bottom + 125), cv::FONT_ITALIC, 2, currColor);
    }


    // plot distance markers
    float lineSpacing = 2.0; // gap between distance markers
    int nMarkers = floor(worldSize.height / lineSpacing);
    for (size_t i = 0; i < nMarkers; ++i)
    {
        int y = (-(i * lineSpacing) * imageSize.height / worldSize.height) + imageSize.height;
        cv::line(topviewImg, cv::Point(0, y), cv::Point(imageSize.width, y), cv::Scalar(255, 0, 0));
    }

    // display image
    string windowName = "3D Objects";
    cv::namedWindow(windowName, 1);
    cv::imshow(windowName, topviewImg);
  

    if(bWait)
    {
        cv::waitKey(0); // wait for key to be pressed
    }
}


// associate a given bounding box with the keypoints it contains
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
{
    std::vector<float> distance;
    distance.reserve(kptMatches.size());
    
    // iterate through matches if the matched points in the ROI then assign it
    for (auto const &match : kptMatches)
    {
        if (boundingBox.roi.contains(kptsCurr[match.trainIdx].pt))
        {
            // to calculate the mean and stddev
            distance.push_back(cv::norm(kptsCurr[match.trainIdx].pt - kptsPrev[match.queryIdx].pt));
        }
    }


    // Calculate the mean and std deviation and limit the matching score between [0, mean+stdDev]
    cv::Scalar mean, stdDev;
    cv::meanStdDev(distance, mean, stdDev);
    // set the threshold, not more than %30 deviation from the mean
    auto threshold = mean[0] + (stdDev[0] > mean[0]*0.3 ? mean[0]*0.3 : stdDev[0]);
    //cout << "thres1, thres2: "<< threshold << ", " << mean[0] + stdDev[0] << ", " << stdDev[0]<<'\n';
    for (auto const &match : kptMatches)
    {
        if (boundingBox.roi.contains(kptsCurr[match.trainIdx].pt) && 
            cv::norm(kptsCurr[match.trainIdx].pt - kptsPrev[match.queryIdx].pt) < threshold)
        {
            // only those whose distances not far than threshold
            boundingBox.keypoints.push_back(kptsCurr[match.trainIdx]);
            boundingBox.kptMatches.push_back(match);
        }
    }      
}


// Compute time-to-collision (TTC) based on keypoint correspondences in successive images
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, 
                      std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
{
    // compute distance ratios between all matched keypoints
    vector<double> distRatios; // stores the distance ratios for all keypoints between curr. and prev. frame
    for (auto it1 = kptMatches.begin(); it1 != kptMatches.end() - 1; ++it1)
    { // outer keypoint loop

        // get current keypoint and its matched partner in the prev. frame
        cv::KeyPoint kpOuterCurr = kptsCurr.at(it1->trainIdx);
        cv::KeyPoint kpOuterPrev = kptsPrev.at(it1->queryIdx);

        for (auto it2 = kptMatches.begin() + 1; it2 != kptMatches.end(); ++it2)
        { // inner keypoint loop

            double minDist = 100.0; // min. required distance

            // get next keypoint and its matched partner in the prev. frame
            cv::KeyPoint kpInnerCurr = kptsCurr.at(it2->trainIdx);
            cv::KeyPoint kpInnerPrev = kptsPrev.at(it2->queryIdx);

            // compute distances and distance ratios
            double distCurr = cv::norm(kpOuterCurr.pt - kpInnerCurr.pt);
            double distPrev = cv::norm(kpOuterPrev.pt - kpInnerPrev.pt);

            if (distPrev > std::numeric_limits<double>::epsilon() && distCurr >= minDist)
            { // avoid division by zero

                double distRatio = distCurr / distPrev;
                distRatios.push_back(distRatio);
            }
        } // eof inner loop over all matched kpts
    }     // eof outer loop over all matched kpts

    // only continue if list of distance ratios is not empty
    if (distRatios.size() == 0)
    {
        TTC = NAN;
        return;
    }

    // compute camera-based TTC from distance ratios
    //double meanDistRatio = std::accumulate(distRatios.begin(), distRatios.end(), 0.0) / distRatios.size();
    std::sort(distRatios.begin(), distRatios.end());
    long medIndex = std::floor(distRatios.size() / 2.0);
    double medianDistRatio = distRatios.size() %2 == 0 ? (distRatios[medIndex - 1] + distRatios[medIndex])/2.0 : distRatios[medIndex];
    medianDistRatio = 1 == medianDistRatio ? std::numeric_limits<double>::epsilon() : medianDistRatio;
   
    double dT = 1 / frameRate;
    TTC = -dT / (1 - medianDistRatio); // instead of meanDistRatio
    //cout << " medianDistRatio: "<< medianDistRatio  << ", TTC Camera: "<< TTC << '\n';
    
}


void computeTTCLidar(std::vector<LidarPoint> &copyPrev,
                     std::vector<LidarPoint> &copyCurr, double frameRate, double &TTC)
{
    // auxiliary variables
    double dT = 1.0/frameRate;        // time between two measurements in seconds

    // take the sorted list and hold the median only for the ego lane
    std::sort(copyPrev.begin(), copyPrev.end(), [&](LidarPoint const &lpt1, LidarPoint const &lpt2)
            { 
                return (lpt1.x < lpt2.x); 
            });
    long medIndex = std::floor(copyPrev.size() / 2.0);
    auto medianPrev = copyPrev.size() %2 == 0 ? (copyPrev[medIndex].x+ copyPrev[medIndex - 1].x)/2.0 : copyPrev[medIndex].x;

    // take the sorted list and hold the median
    std::sort(copyCurr.begin(), copyCurr.end(), [&](LidarPoint const &lpt1, LidarPoint const &lpt2)
            { 
                return (lpt1.x < lpt2.x); 
            });
    medIndex = std::floor(copyCurr.size() / 2.0);
    auto medianCurr = copyCurr.size() %2 == 0 ? (copyCurr[medIndex].x+ copyCurr[medIndex - 1].x)/2.0 : copyCurr[medIndex].x;

    // compute TTC from both measurements
    TTC = medianCurr * dT / (medianPrev - medianCurr);
    double VRef = (medianPrev - medianCurr)/dT;
    //cout << "|"<< VRef << "|"<< TTC <<"|Median point method|"<< '\n';
    //cout << "|"<< (copyPrev.begin()->x - copyCurr.begin()->x)/dT  << "|"<< (copyCurr.begin()->x)*dT/ (copyPrev.begin()->x - copyCurr.begin()->x)<<"|Minimum point method|"<< '\n';
}


void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame)
{
    // holds the number of points per mathing BB
    std::map<std::tuple<int, int>, int> matchedBBCount;
    // Loop through the matches
    for (auto const &match : matches)
    {
        auto const findPrevIt = std::find_if(prevFrame.boundingBoxes.begin(), prevFrame.boundingBoxes.end(), [&](auto const &prevBBox) {
            
            // true if the match point within ROI
            return prevBBox.roi.contains(prevFrame.keypoints[match.queryIdx].pt);
        });
        
        auto const findCurrIt = std::find_if(currFrame.boundingBoxes.begin(), currFrame.boundingBoxes.end(), [&](auto const &currBBox) {
            
            // true if the match point within ROI
            return currBBox.roi.contains(currFrame.keypoints[match.trainIdx].pt);
        });
        // corresponding match point found, only if iterators not pointing to the end

        if (findPrevIt != prevFrame.boundingBoxes.end() && findCurrIt != currFrame.boundingBoxes.end())
        {
            auto const matchingBB = make_tuple(findPrevIt->boxID, findCurrIt->boxID);
            matchedBBCount[matchingBB] = ++matchedBBCount[matchingBB];
        }
    }

    for (auto const& stat : matchedBBCount)
    {
        auto [prevFBoxId, currFBoxId ] = stat.first;
        if (stat.second > MIN_MATCHES)
        {
            //cout << "MatchBox: " << prevFBoxId <<":" << currFBoxId <<" --> " << stat.second<< '\n';
            bbBestMatches[prevFBoxId] = currFBoxId;
        }
            
    }

    
}
