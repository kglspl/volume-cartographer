//
// Created by Seth Parker on 10/22/15.
//

#include "texturing/SimpleUV.h"

namespace volcart
{
namespace texturing
{
volcart::UVMap simpleUV(ITKMesh::Pointer mesh, int width, int height)
{

    volcart::UVMap uvMap;
    unsigned long pointID, ArrayX, ArrayY;
    double u, v;

    // Account for zero indexing of points
    auto maxIndexX = static_cast<double>(width - 1);
    auto maxIndexY = static_cast<double>(height - 1);

    // Generate UV coord for each point in mesh
    ITKPointIterator point = mesh->GetPoints()->Begin();
    while (point != mesh->GetPoints()->End()) {

        pointID = point.Index();

        // Assume that the input vertices can be ordered into a 2D array of size
        // width * height
        // Calculate the point's 2D array position [ArrayX, ArrayY] based on its
        // pointID
        ArrayX = pointID % width;
        ArrayY = (pointID - ArrayX) / width;

        // Calculate the point's UV position
        u = static_cast<double>(ArrayX) / maxIndexX;
        v = static_cast<double>(ArrayY) / maxIndexY;

        cv::Vec2d uv(u, v);

        // Add the uv coordinates into our map at the point index specified
        uvMap.set(pointID, uv);

        ++point;
    }

    return uvMap;
};
};  // texturing
};  // volcart
