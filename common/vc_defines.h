//
// Created by Media Team on 7/7/15.
//

#ifndef VC_DEFINES_H
#define VC_DEFINES_H

#include "getMemorySize.h"

///// GLOBAL VARIABLES /////
// This define determines the default dictionary used when creating a new volumepkg
#define VOLPKG_VERSION 2.0

// VC standard component access positions for XYZ
#define VC_INDEX_X 0
#define VC_INDEX_Y 1
#define VC_INDEX_Z 2

///// VC - ITK Mesh Defaults /////
#include <itkMesh.h>
#include "itkPointsLocator.h"
#include <itkTriangleCell.h>

typedef itk::Vector< double, 3 >                      VC_PixelType;
typedef itk::Mesh< VC_PixelType, 3 >                  VC_MeshType;
typedef VC_MeshType::PointType                        VC_PointType;
typedef VC_MeshType::CellType                         VC_CellType;
typedef itk::TriangleCell< VC_CellType >              VC_TriangleType;

typedef VC_MeshType::PointsContainer                  VC_PointsContainerType;
typedef itk::PointsLocator<VC_PointsContainerType>    VC_PointsLocatorType;

typedef VC_MeshType::PointsContainer::ConstIterator   VC_PointsInMeshIterator;
typedef VC_MeshType::CellsContainer::Iterator         VC_CellIterator;
typedef VC_CellType::PointIdIterator                  VC_PointsInCellIterator;


///// ERROR MESSAGES /////
#define VC_ERR_READONLY()       { std::cerr << "ERROR: Volume Package is set to Read-Only. Cannot write to file." << std::endl; return EXIT_FAILURE; }
#define VC_ERR_SLICE_ANALYZE    "ERROR: Slice file does not exist/isn't a regular file and cannot be analyze."

///// UV Maps /////
#include <opencv2/opencv.hpp>

typedef cv::Vec2d VC_Origin;
#define VC_ORIGIN_TOP_LEFT     VC_Origin(0,0)
#define VC_ORIGIN_TOP_RIGHT    VC_Origin(1,0)
#define VC_ORIGIN_BOTTOM_LEFT  VC_Origin(0,1)
#define VC_ORIGIN_BOTTOM_RIGHT VC_Origin(1,1)

#define VC_UVMAP_NULL_MAPPING cv::Vec2d(-1, -1)

///// Texture Compositing /////

#define VC_TEXTURE_NO_VALUE -1.0

enum VC_Composite_Option {
    CompositeOptionIntersection = 0,
    CompositeOptionNonMaximumSuppression,
    CompositeOptionMax,
    CompositeOptionMin,
    CompositeOptionMedianAverage,
    CompositeOptionMedian,
    CompositeOptionMean
};

enum VC_Direction_Option {
    DirectionOptionBoth = 0,
    DirectionOptionPositive,
    DirectionOptionNegative
};

///// Time Helper /////
#include <time.h>

inline std::string VC_DATE_TIME() {
    time_t now = time( 0 );
    struct tm tstruct;
    char buf[ 80 ];
    tstruct = *localtime( &now );
    strftime( buf, sizeof( buf ), "%Y%m%d%H%M%S", &tstruct );
    return std::string(buf);
}

#endif //VC_DEFINES_H