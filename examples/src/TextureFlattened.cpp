//
// Created by Seth Parker on 6/24/15.
//

#include <opencv2/imgcodecs.hpp>
#include <vtkPLYReader.h>
#include <vtkSmartPointer.h>

#include "core/io/OBJWriter.h"
#include "core/io/PLYReader.h"
#include "core/io/PLYWriter.h"
#include "core/types/VolumePkg.h"
#include "core/vc_defines.h"
#include "meshing/ITK2VTK.h"
#include "texturing/CompositeTextureV2.h"
#include "texturing/SimpleUV.h"

int main(int /*argc*/, char* argv[])
{

    VolumePkg vpkg(argv[1]);
    double radius = std::stod(argv[4]);
    std::cout << radius << std::endl;

    // Read the mesh
    vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
    reader->SetFileName(argv[2]);
    reader->Update();
    auto inputMesh = volcart::ITKMesh::New();
    volcart::meshing::VTK2ITK(reader->GetOutput(), inputMesh);

    // Read the uv map
    auto reader2 = vtkSmartPointer<vtkPLYReader>::New();
    reader2->SetFileName(argv[3]);
    reader2->Update();
    auto uvmap = volcart::ITKMesh::New();
    volcart::meshing::VTK2ITK(reader2->GetOutput(), uvmap);

    // UV map setup
    double min_u = uvmap->GetPoint(0)[0];
    double min_v = uvmap->GetPoint(0)[2];
    double max_u = uvmap->GetPoint(0)[0];
    double max_v = uvmap->GetPoint(0)[2];
    for (size_t n_id = 0; n_id < uvmap->GetNumberOfPoints(); ++n_id) {
        double _x = uvmap->GetPoint(n_id)[0];
        double _z = uvmap->GetPoint(n_id)[2];
        if (_x < min_u && _z >= 0)
            min_u = _x;
        if (_z < min_v && _z >= 0)
            min_v = _z;
        if (_x > max_u && _z >= 0)
            max_u = _x;
        if (_z > max_v && _z >= 0)
            max_v = _z;
    }

    // Round so that we have integer bounds
    min_u = std::floor(min_u);
    min_v = std::floor(min_v);
    max_u = std::ceil(max_u);
    max_v = std::ceil(max_v);

    double aspect_width = std::abs(max_u - min_u);
    double aspect_height = std::abs(max_v - min_v);
    volcart::UVMap uvMap;
    uvMap.ratio(aspect_width, aspect_height);

    // Calculate uv coordinates
    double u, v;
    for (auto cell = uvmap->GetCells()->Begin();
         cell != uvmap->GetCells()->End(); ++cell) {

        for (auto pt = cell.Value()->PointIdsBegin();
             pt != cell.Value()->PointIdsEnd(); ++pt) {
            auto p_id = *pt;
            u = (uvmap->GetPoint(p_id)[0] - min_u) / (max_u - min_u);
            v = (uvmap->GetPoint(p_id)[2] - min_v) / (max_v - min_v);
            cv::Vec2d uv(u, v);

            // Add the uv coordinates into our map at the point index specified
            uvMap.set(p_id, uv);
        }
    }

    // Convert soft body to itk mesh
    volcart::texturing::CompositeTextureV2 result(
        inputMesh, vpkg, uvMap, radius, static_cast<int>(aspect_width),
        static_cast<int>(aspect_height));
    volcart::io::OBJWriter objwriter(
        "cloth.obj", inputMesh, result.texture().uvMap(),
        result.texture().image(0));
    objwriter.write();

    if (result.texture().mask().data) {
        cv::imwrite("PerPixelMask.png", result.texture().mask());
    }

    if (result.texture().ppm().initialized()) {
        volcart::PerPixelMap::WritePPM(
            "PerPixelMapping", result.texture().ppm());
    }

    return EXIT_SUCCESS;
}