//
// Created by Media Team on 6/24/15.
//

#include "objWriter.h"

namespace volcart {
    namespace io {

    ///// Constructors /////
    objWriter::objWriter() {};

    objWriter::objWriter( std::string outputPath, itk::Mesh<PixelType, 3>::Pointer mesh ) {
        _outputPath = outputPath;
        _mesh = mesh;
    };

    objWriter::objWriter( std::string outputPath, itk::Mesh<PixelType, 3>::Pointer mesh, std::map<double, cv::Vec2d> uvMap, cv::Mat uvImg ) {
        _outputPath = outputPath;
        _mesh = mesh;
        _textCoords = uvMap;
        _texture = uvImg;
    };

    ///// Validation /////
    // Make sure that all required parameters have been set and are okay
    bool objWriter::validate() {

        // Make sure the output path has a file extension for the OBJ
        bool hasExt = ( _outputPath.extension() == ".OBJ" || _outputPath.extension() == ".obj" );
        // Make sure the output directory exists
        bool pathExists = boost::filesystem::is_directory( boost::filesystem::canonical( _outputPath.parent_path() ) );
        // Check that the mesh exists and has points
        bool meshHasPoints = ( _mesh.IsNotNull() && _mesh->GetNumberOfPoints() != 0 ) ;

        return ( hasExt && pathExists && meshHasPoints ) ;
    };

    ///// Output Methods /////
    // Write everything (OBJ, MTL, and PNG) to disk
    int objWriter::write() {
        if ( !validate() ) return EXIT_FAILURE; // Must pass validation test

        // Write the OBJ
        writeOBJ();

        // Write texture stuff if we have a UV coordinate map
        if ( !_textCoords.empty() ) {
            writeMTL();
            writeTexture();
        }

        return EXIT_SUCCESS;
    };

    // Write the OBJ file to disk
    int objWriter::writeOBJ() {
        _outputMesh.open( _outputPath.string() ); // Open the file stream
        if(!_outputMesh.is_open()) return EXIT_FAILURE; // Return error if we can't open the file

        _writeHeader();
        _writeVertices();
        if ( !_textCoords.empty() ) _writeTextureCoordinates(); // Only write texture information if we have a UV map
        _writeFaces();

        _outputMesh.close(); // Close the file stream

        return EXIT_SUCCESS;
    };

    // Write the MTL file to disk
    // See http://paulbourke.net/dataformats/mtl/ for more options
    int objWriter::writeMTL() {
        _outputMTL.open( _outputPath.stem().string() + ".mtl" ); // Open the file stream
        if(!_outputMTL.is_open()) return EXIT_FAILURE; // Return error if we can't open the file

        std::cerr << "Writing MTL..." << std::endl;

        _outputMTL << "newmtl default" << std::endl;

        _outputMTL << "Ka 1.0 1.0 1.0" << std::endl;   // Ambient light color
        _outputMTL << "Kd 1.0 1.0 1.0" << std::endl;   // Diffuse light color
        _outputMTL << "Ks 0.0 0.0 0.0" << std::endl;   // Specular light color
        _outputMTL << "illum 2" << std::endl;          // Illumination mode
        _outputMTL << "d 1.0" << std::endl;            // Dissolve. 1.0 == opaque

        if ( !_texture.empty() )
            _outputMTL << "map_Kd " << _outputPath.stem().string() + ".png" << std::endl; // Path to the texture file, relative to the MTL file

        _outputMTL.close(); // Close the file stream
        return EXIT_SUCCESS;
    };

    // Write the PNG texture file to disk
    int objWriter::writeTexture() {
        if ( _texture.empty() ) return EXIT_FAILURE;

        std::cerr << "Writing texture image..." << std::endl;
        imwrite( _outputPath.stem().string() + ".png", _texture );
        return EXIT_SUCCESS;
    };

    // Write our custom header
    int objWriter::_writeHeader() {
        if(!_outputMesh.is_open()) return EXIT_FAILURE;

        _outputMesh << "# VolCart OBJ File" << std::endl;
        _outputMesh << "# VC OBJ Exporter v1.0" << std::endl;

        return EXIT_SUCCESS;
    };

    // Write the vertex information: 'v x y z'
    //                               'vn nx ny nz'
    int objWriter::_writeVertices() {
        if( !_outputMesh.is_open() || _mesh->GetNumberOfPoints() == 0 ) return EXIT_FAILURE;
        std::cerr << "Writing vertices..." << std::endl;

        _outputMesh << "# Vertices: " << _mesh->GetNumberOfPoints() << std::endl;

        // Iterate over all of the points
        PointsInMeshIterator point = _mesh->GetPoints()->Begin();
        while ( point != _mesh->GetPoints()->End() ) {

            // Get the point's normal
            MeshType::PixelType normal;
            _mesh->GetPointData(point.Index(), &normal);

            // Write the point position components and its normal components.
            _outputMesh << "v "  << point.Value()[0] << " " << point.Value()[1] << " " << point.Value()[2] << std::endl;
            _outputMesh << "vn " << normal[0] << " " << normal[1] << " " << normal[2] << std::endl;

            ++point;
        }

        return EXIT_SUCCESS;
    };

    // Write the UV coordinates that will be attached to points: 'vt u v'
    // To-Do: Separate out the mtllib and mtl assignment
    int objWriter::_writeTextureCoordinates() {
        if( !_outputMesh.is_open() || _textCoords.empty() ) return EXIT_FAILURE;
        std::cerr << "Writing texture coordinates..." << std::endl;

        _outputMesh << "# Texture information" << std::endl;
        _outputMesh << "mtllib " << _outputPath.stem().string() << ".mtl" << std::endl;  // The path of the MTL file, relative to the obj
        _outputMesh << "usemtl default" << std::endl;  // Use the material named 'default' in the MTL file

        // Iterate over all of the saved coordinates in our coordinate map
        auto coordinate = _textCoords.begin(); // The map iterator
        while ( coordinate != _textCoords.end() ) {
            // Map iterators return pairs: {first = key, second = cv::Vec2d}
            // [u, v] == [ second[0], second[1] ]
            _outputMesh << "vt " << coordinate->second[0] << " " << coordinate->second[1] << std::endl;
            ++coordinate;
        }

        return EXIT_SUCCESS;
    };

    // Write the face information: 'f v/vt/vn'
    // Note: This method currently assumes that *every* point in the mesh has an associated normal and texture map
    // This will definitely not always be the case and should be fixed. - SP
    int objWriter::_writeFaces() {
        if( !_outputMesh.is_open() || _mesh->GetNumberOfCells() == 0 ) return EXIT_FAILURE;
        std::cerr << "Writing faces..." << std::endl;

        _outputMesh << "# Faces: " << _mesh->GetNumberOfCells() << std::endl;

        // Iterate over the faces of the mesh
        PointsInCellIterator point;

        for ( CellIterator cell = _mesh->GetCells()->Begin(); cell != _mesh->GetCells()->End(); ++cell ) {
            _outputMesh << "f "; // Starts a new face line

            // Iterate over the points of this face
            for ( point = cell.Value()->PointIdsBegin(); point != cell.Value()->PointIdsEnd(); ++point ) {
                double pointIndex = *point + 1; // OBJ elements are indexed from 1, not 0
                std::string textureIndex = "";

                // Set the texture index if we have texture coordinates
                // To-Do: This assumes that textureIndex == pointIndex, which may not be the case
                if ( !_textCoords.empty() && _textCoords.count( *point ) > 0 )
                    textureIndex = boost::lexical_cast<std::string>(pointIndex);

                _outputMesh << boost::lexical_cast<std::string>(pointIndex) << "/" << textureIndex << "/" << boost::lexical_cast<std::string>(pointIndex) << " ";
            }
            _outputMesh << std::endl;
        }

        return EXIT_SUCCESS;
    };

    } // namespace io
} //namespace volcart