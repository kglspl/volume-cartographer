#pragma once

#include <deque>
#include <iostream>
#include <utility>

#include <opencv2/core.hpp>

#include "segmentation/lrps/Common.h"

namespace volcart
{
namespace segmentation
{
/**
 * @class IntensityMap
 * @brief A class representing the intensity map generated from a row of a
 * matrix
 *        normalized to the range [0, 1]
 * @ingroup lrps
 */
class IntensityMap
{
public:
    /** @brief Initializes the class and sets parameters
     * @param stepSize How many points to move by each time you move
     * @param peakDistanceWeight How much the distance between points
     *                           should be taken into account
     * @param shouldIncludeMiddle Whether or not to include points
     *                            in the middle
     */
    IntensityMap(
        cv::Mat,
        int32_t stepSize,
        int32_t peakDistanceWeight,
        bool shouldIncludeMiddle);

    /**
     * @brief Creates the intensity map
     */
    cv::Mat draw();

    /**
     * @brief The top maxima in the row being processed, number of maxima
     * determined by the peak radius
     */
    std::deque<std::pair<int32_t, double>> sortedMaxima();

    /**
     * @brief Sets the Maxima Index
     * @param index What you want the Maxima Index to be
     */
    void setChosenMaximaIndex(int32_t index) { chosenMaximaIndex_ = index; }

    /** @brief Returns the current Maxima Index*/
    int32_t chosenMaximaIndex() const { return chosenMaximaIndex_; }

    /** @brief Increases the Maxima Index by 1 */
    void incrementMaximaIndex() { chosenMaximaIndex_++; }

    /** @brief Returns the Peak Radius  */
    int32_t peakRadius() const { return peakRadius_; }

private:
    /**@brief Overwrites output operator to write the intensities  */
    friend std::ostream& operator<<(std::ostream& s, const IntensityMap& m)
    {
        return s << m.intensities_;
    }
    /** How much you want to move each time you leave an element */
    int32_t stepSize_;
    /** How much the distance between two points should be taken
     * into account */
    int32_t peakDistanceWeight_;
    /** List of Intensities in the Image*/
    cv::Mat_<double> intensities_;
    /** Image of the data from when the imaged was resliced */
    cv::Mat_<uint8_t> resliceData_;
    /** Width of the image when it's displayed*/
    int32_t displayWidth_;
    /** Height of the image when it's displayed*/
    int32_t displayHeight_;
    /** Where the Intensity map is saved*/
    cv::Mat drawTarget_;
    /** Width of the bin to hold the image*/
    int32_t binWidth_;
    /** Width of the intensity map*/
    int32_t mapWidth_;
    /** Maxima Index*/
    int32_t chosenMaximaIndex_;
    /** Determines whether or not to include pixels in the middle*/
    bool shouldIncludeMiddle_;
    /** Largest radius when searching a neighborhood of pixel*/
    const int32_t peakRadius_ = 5;
};
}
}
