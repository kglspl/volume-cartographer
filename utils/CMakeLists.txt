# vc_area
add_executable(vc_area src/SurfArea.cpp)
target_link_libraries(vc_area VC::core VC::meshing ${VC_FS_LIB} VTK::FiltersCore)
list(APPEND utils_install_list vc_area)

# vc_invert_cloud
add_executable(vc_invert_cloud src/InvertCloud.cpp)
target_link_libraries(vc_invert_cloud VC::core ${VC_FS_LIB})

# vc_volpkg_upgrade
add_executable(vc_volpkg_upgrade src/VolpkgUpgrade.cpp)
target_link_libraries(vc_volpkg_upgrade VC::core)
list(APPEND utils_install_list vc_volpkg_upgrade)

# vc_flip_mesh
add_executable(vc_flip_mesh src/FlipMesh.cpp)
target_link_libraries(vc_flip_mesh VC::core ${VC_FS_LIB} Boost::program_options)

# vc_transform_uv
add_executable(vc_transform_uv src/TransformUV.cpp)
target_link_libraries(vc_transform_uv
    VC::core
    VC::meshing
    VC::app_support
    ${VC_FS_LIB}
    Boost::program_options
)
list(APPEND utils_install_list vc_transform_uv)

# vc_retexture_mesh
add_executable(vc_retexture_mesh src/RetextureMesh.cpp)
target_link_libraries(vc_retexture_mesh VC::core ${VC_FS_LIB} Boost::program_options)
list(APPEND utils_install_list vc_retexture_mesh)

# vc_ppm_to_pointset
add_executable(vc_ppm_to_pointset src/PPMToPointSet.cpp)
target_link_libraries(vc_ppm_to_pointset VC::core ${VC_FS_LIB} Boost::program_options)

# vc_uv2mesh
add_executable(vc_uv2mesh src/UVMaptoFlatMesh.cpp)
target_link_libraries(vc_uv2mesh
    VC::core
    VC::meshing
    ${VC_FS_LIB}
    Boost::program_options
)

# vc_volume_bump
add_executable(vc_volume_bump src/VolumeBump.cpp)
target_link_libraries(vc_volume_bump
    VC::app_support
    VC::core
    ${VC_FS_LIB}
    Boost::program_options
    opencv_core
    opencv_imgcodecs
)

# vc_transform_mesh
add_executable(vc_transform_mesh src/TransformMesh.cpp)
target_link_libraries(vc_transform_mesh
    VC::core
    ${VC_FS_LIB}
    Boost::program_options
    ITKIOTransformBase
    ITKIOTransformInsightLegacy
)
list(APPEND utils_install_list vc_transform_mesh)

# vc_add_alignment_markers
add_executable(vc_add_alignment_markers src/AlignmentMarkers.cpp)
target_link_libraries(vc_add_alignment_markers
    VC::core
    VC::texturing
    ${VC_FS_LIB}
    Boost::program_options
)

# vc_convert_pointset
add_executable(vc_convert_pointset src/ConvertPointSet.cpp)
target_link_libraries(vc_convert_pointset
    VC::app_support
    VC::core
    VC::texturing
    ${VC_FS_LIB}
    Boost::program_options
)
list(APPEND utils_install_list vc_convert_pointset)

# vc_merge_pointsets
add_executable(vc_merge_pointset src/MergePointSets.cpp)
target_link_libraries(vc_merge_pointset
    VC::core
    ${VC_FS_LIB}
    Boost::program_options
)
list(APPEND utils_install_list vc_merge_pointset)

# vc_convert_mask
add_executable(vc_convert_mask src/ConvertMask.cpp)
target_link_libraries(vc_convert_mask
    VC::core
    VC::app_support
    opencv_core
    opencv_imgcodecs
    ${VC_FS_LIB}
    Boost::program_options
)

# vc_seg_to_pointmask
add_executable(vc_seg_to_pointmask src/SegToPointMask.cpp)
target_link_libraries(vc_seg_to_pointmask
    VC::core
    VC::segmentation
    VC::app_support
    opencv_core
    ${VC_FS_LIB}
    Boost::program_options
)

# vc_visualize_ppm
add_executable(vc_visualize_ppm src/VisualizePPM.cpp)
target_link_libraries(vc_visualize_ppm
    VC::core
    VC::app_support
    ${VC_FS_LIB}
    Boost::program_options
)
list(APPEND utils_install_list vc_visualize_ppm)

# vc_image_stats
add_executable(vc_image_stats src/ImageStatistics.cpp)
target_link_libraries(vc_image_stats
    VC::core
    VC::app_support
    opencv_core
    opencv_imgcodecs
    ${VC_FS_LIB}
    Boost::program_options
)

# vc_flatten_mesh
add_executable(vc_flatten_mesh src/FlattenMesh.cpp)
target_link_libraries(vc_flatten_mesh
    VC::core
    VC::texturing
    ${VC_FS_LIB}
    Boost::program_options
)
list(APPEND utils_install_list vc_flatten_mesh)

# vc_color_map
add_executable(vc_color_map src/ApplyColorMap.cpp)
target_link_libraries(vc_color_map VC::core Boost::program_options)
list(APPEND utils_install_list vc_color_map)

# vc_cmap_bar
# Used to generate example colormap bars for documentation
add_executable(vc_cmap_bar src/GenerateColorMapBar.cpp)
target_link_libraries(vc_cmap_bar VC::core Boost::program_options)

# vc_visualize_graph
add_executable(vc_visualize_graph src/VisualizeRenderGraph.cpp)
target_link_libraries(vc_visualize_graph VC::core VC::graph Boost::program_options smgl::smgl)
list(APPEND utils_install_list vc_visualize_graph)

# vc_fix_pointset_duplicates
add_executable(vc_repair_pointsets src/RepairPointSets.cpp)
target_link_libraries(vc_repair_pointsets VC::core Boost::program_options)
list(APPEND utils_install_list vc_repair_pointsets)

# vc_fix_pointset_duplicates
add_executable(vc_version src/ReportVersion.cpp)
target_link_libraries(vc_version VC::core)
list(APPEND utils_install_list vc_version)

# Install targets
if(VC_INSTALL_UTILS)
install(
    TARGETS ${utils_install_list}
    RUNTIME DESTINATION bin
    COMPONENT Utilities
)
endif()
