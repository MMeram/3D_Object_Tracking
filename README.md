# 3D_Object_Tracking
Final Computer Vision Project as a part of Sensor Fusion Nanodegree

## Prerequisites

Please read the installation.md file for the requirements. This is in fact the original README file of the UDSF 3D Object Tracking final project

Here we will go through all required parts of the project

## FP.1 Match 3D Objects

The function prototype is given as 

```c++
void matchBoundingBoxes(std::vector<cv::DMatch> &matches, std::map<int, int> &bbBestMatches, DataFrame &prevFrame, DataFrame &currFrame);
```

It is expected to return the best matched bounding boxes between the current and previous data frames. For each match, first we identify the keypoints in the previous and the current frame if they are contained by the corresponding bounding boxes and if so we count these keypoints. 

In the end and another final loop we select only BB if they have more than 50 keypoints, a threshold selected arbitrarily good enough.

## FP.2 Compute Lidar-based TTC

The function prototype is given as

```c++
void computeTTCLidar(std::vector<LidarPoint> &copyPrev,
                     std::vector<LidarPoint> &copyCurr, double frameRate, double &TTC);
```

It is based on the lidar distance calculation in the current and previous frames and their corresponding minimum point in the point cloud. However, due to the outliers it turned out not to be reliable so I decided to pick the median, which proved to be more reliable solution. (See the FP.5)

$TTC = medianCurr * dT / (medianPrev - medianCurr)$

## FP.3 Associate Keypoint Correspondences with Bounding Boxes

The function prototype is given as

```c++
void clusterKptMatchesWithROI(BoundingBox &boundingBox, std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> &kptMatches)
```

The expectation is first to detect the keypoints in the current frame that fall into the bounding box ROI and then calculate the L2 distances between them and corresponding equivalents in the previous frame using the cv::DMatch data structure.

To limit the outliers the mean of this distance vector is obtained and only %30 deviation from the mean is accepted.

Note that originally I utilized keypoint match distances for keypoints that lie within the matching bounding boxes in the consecutive frames and eliminated those whose distances are bigger than the mean + stddev.

It also yields very reliable and very similar solution to the L2 approach.

## FP.4 Compute Camera-based TTC

The function prototype is given as

```c++
void computeTTCCamera(std::vector<cv::KeyPoint> &kptsPrev, std::vector<cv::KeyPoint> &kptsCurr, std::vector<cv::DMatch> kptMatches, double frameRate, double &TTC, cv::Mat *visImg)
```

Here we calculate the ratio of all relative distances between consecutive frames to compute a reliable TTC estimate in a vector.

Then we pick the median or mean value of it to calculate the final TTC:

$TTC = -dT / (1 - medianDistRatio)$

with $dT$ being the interval between measurements.

## FP.5 Performance Evaluation 1

| Ref Velocity | TTC Lidar    | Computation Method   |
| ------------ | ------------ | -------------------- |
| 0.640001     | 12.5156      | Median point method  |
| 0.609999     | 12.9722      | Minimum point method |
| 0.630002     | 12.6142      | Median point method  |
| 0.640001     | 12.264       | Minimum point method |
| 0.560002     | 14.091       | Median point method  |
| 0.559998     | 13.9161      | Minimum point method |
| 0.469999     | 16.6894      | Median point method  |
| 1.08         | 7.11572      | Minimum point method |
| 0.494998     | 15.7465      | Median point method  |
| 0.469999     | 16.2511      | Minimum point method |
| 0.604999     | 12.7835      | Median point method  |
| 0.609999     | 12.4213      | Minimum point method |
| 0.640001     | 11.9844      | Median point method  |
| 0.220003     | 34.3404      | Minimum point method |
| 0.580001     | 13.1241      | Median point method  |
| 0.799999     | 9.34376      | Minimum point method |
| 0.580001     | 13.0241      | Median point method  |
| 0.409999     | 18.1318      | Minimum point method |
| 0.669999     | 11.1746      | Median point method  |
| 0.409999     | 18.0318      | Minimum point method |
| 0.580001     | 12.8086      | Median point method  |
| 1.88         | **3.83244**  | Minimum point method |
| 0.819998     | 8.95978      | Median point method  |
| -0.669999    | **-10.8537** | Minimum point method |
| 0.73         | 9.96439      | Median point method  |
| 0.780001     | 9.22307      | Minimum point method |
| 0.750003     | 9.59863      | Median point method  |
| 0.649996     | 10.9678      | Minimum point method |
| 0.834999     | 8.52157      | Median point method  |
| 0.870004     | 8.09422      | Minimum point method |
| 0.740001     | 9.51552      | Median point method  |
| 2.15         | **3.17535**  | Minimum point method |
| 0.725        | 9.61241      | Median point method  |
| -0.689998    | **-9.99424** | Minimum point method |
| 0.819998     | 8.3988       | Median point method  |
| 0.819998     | 8.30978      | Minimum point method |

It is seen the TTC results first based on the constant velocity model with the minimum lidar points detected and then  with the median points in the respective bounding boxes. As the bold values indicate, especially the minus ones, the collision should have already happened in the past, so it's not a feasible solution to consider the minimum values but the median ones.

## FP.6 Performance Evaluation 2

All possible combination of detectors and descriptors has been collected in the *ttcCameraPerformance.ods* file. It turned out that the AKAZE/FREAK combination performs the best among all others

![](/media/mmeram/backup/Self-Driving-Cars/Sensor-Fusion/Camera/SFND_3D_Object_Tracking/project/3D_Object_Tracking/TTCCameraBestDetDes.png)

It's apparently known to yield the best result in high frame rate or high resolution images. Here an indirect supportive article can be found at: http://tulipp.eu/wp-content/uploads/2019/03/2017_TUD_HEART_kalms.pdf