#ifndef KMEANS2_HPP
#define KMEANS2_HPP

#include <opencv2/opencv.hpp>

cv::Mat kmeans2(cv::Mat input, int K, int attempts, float halt = .02, int epochs = 1000);
cv::Mat kmeans2(cv::Mat input, int K, int attempts, cv::Mat centers);
cv::Mat fastkmeans2(cv::Mat input, int K, int epochs = 100);

#endif