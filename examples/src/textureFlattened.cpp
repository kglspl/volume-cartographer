//
// Created by Seth Parker on 6/24/15.
//

#include "common/vc_defines.h"
#include "volumepkg/volumepkg.h"

#include "common/io/plyWriter.h"
#include "common/io/ply2itk.h"
#include "common/io/objWriter.h"
#include "meshing/itk2vtk.h"

#include <vtkSmartPointer.h>
#include <vtkPLYReader.h>

#include "texturing/simpleUV.h"
#include "texturing/compositeTextureV2.h"

int main( int argc, char* argv[] ) {

  VolumePkg vpkg( argv[1] );

  // Read the mesh
  vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
  reader->SetFileName ( "decim.ply" );
  reader->Update();
  VC_MeshType::Pointer inputMesh = VC_MeshType::New();
  volcart::meshing::vtk2itk( reader->GetOutput(), inputMesh );

  // Read the uv map
  vtkSmartPointer<vtkPLYReader> reader2 = vtkSmartPointer<vtkPLYReader>::New();
  reader2->SetFileName ( "uvmap.ply" );
  reader2->Update();
  VC_MeshType::Pointer uvmap = VC_MeshType::New();
  volcart::meshing::vtk2itk( reader2->GetOutput(), uvmap );

  // UV map setup
  double min_u = uvmap->GetPoint(0)[0];
  double min_v = uvmap->GetPoint(0)[2];
  double max_u = uvmap->GetPoint(0)[0];
  double max_v = uvmap->GetPoint(0)[2];
  for (size_t n_id = 0; n_id < uvmap->GetNumberOfPoints(); ++n_id) {
    double _x = uvmap->GetPoint(n_id)[0];
    double _z = uvmap->GetPoint(n_id)[2];
    if ( _x < min_u && _z >= 0) min_u = _x;
    if ( _z < min_v && _z >= 0) min_v = _z;
    if ( _x > max_u && _z >= 0) max_u = _x;
    if ( _z > max_v && _z >= 0) max_v = _z;
  }

  // Round so that we have integer bounds
  min_u = std::floor(min_u);
  min_v = std::floor(min_v);
  max_u = std::ceil(max_u);
  max_v = std::ceil(max_v);

  double aspect_width = std::abs(max_u - min_u);
  double aspect_height = std::abs(max_v - min_v);
  double aspect = aspect_width / aspect_height;
  volcart::UVMap uvMap;
  uvMap.ratio(aspect_width, aspect_height);

  // Calculate uv coordinates
  double u, v;
  for (VC_CellIterator cell = uvmap->GetCells()->Begin(); cell != uvmap->GetCells()->End(); ++cell ) {

    for ( VC_PointsInCellIterator pt = cell.Value()->PointIdsBegin(); pt != cell.Value()->PointIdsEnd(); ++pt ) {
      double p_id = *pt;
      u = (uvmap->GetPoint(p_id)[0] - min_u) / (max_u - min_u);
      v = (uvmap->GetPoint(p_id)[2] - min_v) / (max_v - min_v);
      cv::Vec2d uv( u, v );

      // Add the uv coordinates into our map at the point index specified
      uvMap.set(p_id, uv);
    }

  }

  // Convert soft body to itk mesh
  volcart::texturing::compositeTextureV2 result(inputMesh, vpkg, uvMap, 7, (int) aspect_width, (int) aspect_height);
  volcart::io::objWriter objwriter("cloth.obj", inputMesh, result.texture().uvMap(), result.texture().getImage(0));
  objwriter.write();


  return EXIT_SUCCESS;
}