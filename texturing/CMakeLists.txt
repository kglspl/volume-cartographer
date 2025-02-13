project(libvc_texturing VERSION ${VC_VERSION} LANGUAGES CXX)

set(srcs
    src/CompositeTexture.cpp
    src/AngleBasedFlattening.cpp
    src/PPMGenerator.cpp
    src/IntersectionTexture.cpp
    src/IntegralTexture.cpp
    src/LayerTexture.cpp
    src/FlatteningAlgorithm.cpp
    src/ScaleMarkerGenerator.cpp
    src/OrthographicProjectionFlattening.cpp
    src/ProjectMesh.cpp
    src/AlignmentMarkerGenerator.cpp
    src/ThicknessTexture.cpp
    src/FlatteningError.cpp
)
set(public_deps
    VC::core
)
set(private_deps
    opencv_imgproc
    VC::meshing
    VTK::CommonCore
    VTK::CommonDataModel
    VTK::FiltersGeneral
    Eigen3::Eigen
)
set(defs "")

add_library(vc_texturing ${srcs})
add_library(VC::texturing ALIAS vc_texturing)
target_compile_definitions(vc_texturing PUBLIC ${defs})
target_include_directories(vc_texturing
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${openabf_SOURCE_DIR}/single_include>
        $<BUILD_INTERFACE:${bvh_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(vc_texturing
    PUBLIC ${public_deps}
    PRIVATE ${private_deps}
)
target_compile_features(vc_texturing PUBLIC cxx_std_17)
set_target_properties(vc_texturing PROPERTIES
    VERSION "${PROJECT_VERSION}"
    EXPORT_NAME texturing
)

if(VC_INSTALL_LIBS)
install(
    TARGETS vc_texturing
    EXPORT ${targets_export_name}
    COMPONENT "Libraries"
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}/texturing"
)
install(
    DIRECTORY "${include_install_dir}/texturing"
    DESTINATION "${include_install_dir}"
    COMPONENT "Libraries"
    FILES_MATCHING REGEX ".*\.(h|hpp)$"
)
endif()

### Testing ###
if(VC_BUILD_TESTS)

# Set source files
set(test_srcs
    test/ABFTest.cpp
    test/FlatteningErrorTest.cpp
    test/PPMGeneratorTest.cpp
)

# Add a test executable for each src
foreach(src ${test_srcs})
    get_filename_component(filename ${src} NAME_WE)
    set(testname vc_texturing_${filename})
    add_executable(${testname} ${src})
    target_link_libraries(${testname}
        VC::texturing
        VC::meshing
        VC::testing
        gtest_main
        gmock_main
        ${public_deps}
    )
    add_test(
        NAME ${testname}
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        COMMAND ${testname}
    )
endforeach()

# Set test resource files
set(VC_TEXTURING_TEST_RES
    test/res/abf_Plane.obj
    test/res/abf_Plane_LSCMOnly.obj
    test/res/abf_Arch.obj
    test/res/abf_Arch_LSCMOnly.obj
    test/res/PPMGenerator_100x100.ppm
    test/res/PPMGenerator_100x100_mask.png
    test/res/PPMGenerator_100x100_cellmap.tif
)

# Copy each test resource file
foreach(r ${VC_TEXTURING_TEST_RES})
    file(COPY ${r} DESTINATION ${EXECUTABLE_OUTPUT_PATH})
endforeach()

endif(VC_BUILD_TESTS)
