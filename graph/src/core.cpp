#include "vc/graph/core.hpp"

#include <nlohmann/json.hpp>

#include "vc/core/io/ImageIO.hpp"
#include "vc/core/io/PointSetIO.hpp"
#include "vc/core/io/UVMapIO.hpp"
#include "vc/core/util/FloatComparison.hpp"

using namespace volcart;
namespace fs = volcart::filesystem;

// Enum conversions
namespace volcart
{
// clang-format off
using FlipAxis = FlipUVMapNode::FlipAxis;
NLOHMANN_JSON_SERIALIZE_ENUM(FlipAxis, {
    {FlipAxis::Vertical, "vertical"},
    {FlipAxis::Horizontal, "horizontal"},
    {FlipAxis::Both, "both"}
})
// clang-format on
}  // namespace volcart

LoadVolumePkgNode::LoadVolumePkgNode() : path{&path_}, volpkg{&vpkg_}
{
    registerInputPort("path", path);
    registerOutputPort("volpkg", volpkg);
    compute = [=]() { vpkg_ = VolumePkg::New(path_); };
}

auto LoadVolumePkgNode::serialize_(
    bool /*useCache*/, const fs::path& /*cacheDir*/) -> smgl::Metadata
{
    return {{"path", path_.string()}};
}

void LoadVolumePkgNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    path_ = meta["path"].get<std::string>();
    vpkg_ = VolumePkg::New(path_);
}

VolumePkgPropertiesNode::VolumePkgPropertiesNode()
    : volpkg{&vpkg_}
    , name{[=]() { return vpkg_->name(); }}
    , version{[=]() { return vpkg_->version(); }}
    , materialThickness{[=]() { return vpkg_->materialThickness(); }}
{
    registerInputPort("volpkg", volpkg);
    registerOutputPort("name", name);
    registerOutputPort("version", version);
    registerOutputPort("materialThickness", materialThickness);
}

VolumeSelectorNode::VolumeSelectorNode()
    : volpkg{&vpkg_}, id{&id_}, volume{&vol_}
{
    registerInputPort("volpkg", volpkg);
    registerInputPort("id", id);
    registerOutputPort("volume", volume);
    compute = [=]() {
        if (id_.empty()) {
            vol_ = vpkg_->volume();
            id_ = vol_->id();
        } else {
            vol_ = vpkg_->volume(id_);
        }
    };
}
auto VolumeSelectorNode::serialize_(
    bool /*useCache*/, const fs::path& /*cacheDir*/) -> smgl::Metadata
{
    return {{"id", id_}};
}

void VolumeSelectorNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    id_ = meta["id"].get<std::string>();
}

VolumePropertiesNode::VolumePropertiesNode()
    : volumeIn{&volume_}
    , cacheMemory{&cacheMem_}
    , bounds{[=]() { return volume_->bounds(); }}
    , voxelSize{[=]() { return volume_->voxelSize(); }}
    , volumeOut{&volume_}
{
    registerInputPort("volumeIn", volumeIn);
    registerInputPort("cacheMemory", cacheMemory);
    registerOutputPort("bounds", bounds);
    registerOutputPort("voxelSize", voxelSize);
    registerOutputPort("volumeOut", volumeOut);

    compute = [=]() {
        if (volume_) {
            volume_->setCacheMemoryInBytes(cacheMem_);
        }
    };
}

auto VolumePropertiesNode::serialize_(
    bool /*useCache*/, const filesystem::path& /*cacheDir*/) -> smgl::Metadata
{
    return {{"cacheMemory", cacheMem_}};
}

void VolumePropertiesNode::deserialize_(
    const smgl::Metadata& meta, const filesystem::path& /*cacheDir*/)
{
    cacheMem_ = meta["cacheMemory"].get<size_t>();
}

SegmentationSelectorNode::SegmentationSelectorNode()
    : volpkg{&vpkg_}, id{&id_}, segmentation{&seg_}
{
    registerInputPort("volpkg", volpkg);
    registerInputPort("id", id);
    registerOutputPort("segmentation", segmentation);

    compute = [=]() { seg_ = vpkg_->segmentation(id_); };
}

auto SegmentationSelectorNode::serialize_(
    bool /*useCache*/, const fs::path& /*cacheDir*/) -> smgl::Metadata
{
    return {{"id", id_}};
}

void SegmentationSelectorNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    id_ = meta["id"].get<std::string>();
}

SegmentationPropertiesNode::SegmentationPropertiesNode()
    : segmentation{&seg_}, pointSet{[=]() { return seg_->getPointSet(); }}
{
    registerInputPort("segmentation", segmentation);
    registerOutputPort("pointSet", pointSet);
}

MeshPropertiesNode::MeshPropertiesNode()
    : mesh{&mesh_}
    , numVertices{[=]() { return mesh_->GetNumberOfPoints(); }}
    , numFaces{[=]() { return mesh_->GetNumberOfCells(); }}
{
    registerInputPort("mesh", mesh);
    registerOutputPort("numVertices", numVertices);
    registerOutputPort("numFaces", numFaces);
}

LoadMeshNode::LoadMeshNode()
    : smgl::Node{true}
    , path{&path_}
    , loaded_{}
    , cacheArgs{&cacheArgs_}
    , mesh{&loaded_.mesh}
    , uvMap{&loaded_.uv}
    , texture{&loaded_.texture}
{
    registerInputPort("path", path);
    registerInputPort("cacheArgs", cacheArgs);
    registerOutputPort("mesh", mesh);
    registerOutputPort("uvMap", uvMap);
    registerOutputPort("texture", texture);
    compute = [=]() { loaded_ = ReadMesh(path_); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto LoadMeshNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_ and loaded_.mesh) {
        auto file = path_.filename().replace_extension(".obj");
        WriteMesh(cacheDir / file, loaded_.mesh, loaded_.uv, loaded_.texture);
        meta["cachedFile"] = file.string();
    }
    return meta;
}

void LoadMeshNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /** cacheDir */)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

WriteMeshNode::WriteMeshNode()
    : smgl::Node{true}
    , path{&path_}
    , mesh{&mesh_}
    , uvMap{&uv_}
    , texture{&texture_}
    , cacheArgs{&cacheArgs_}
{
    registerInputPort("path", path);
    registerInputPort("mesh", mesh);
    registerInputPort("uvMap", uvMap);
    registerInputPort("texture", texture);
    registerInputPort("cacheArgs", cacheArgs);
    compute = [=]() { WriteMesh(path_, mesh_, uv_, texture_); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto WriteMeshNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};

    if (useCache and cacheArgs_) {
        auto file = path_.filename().replace_extension(".obj");
        WriteMesh(cacheDir / file, mesh_, uv_, texture_);
        meta["cachedFile"] = file.string();
    }

    return meta;
}

void WriteMeshNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

RotateUVMapNode::RotateUVMapNode()
    : Node{true}, uvMapIn{&uvMapIn_}, theta{&theta_}, uvMapOut{&uvMapOut_}
{
    registerInputPort("uvMapIn", uvMapIn);
    registerInputPort("theta", theta);
    registerOutputPort("uvMapOut", uvMapOut);

    compute = [=]() {
        static constexpr double PI_CONST{3.1415926535897932385L};
        static constexpr double DEG_TO_RAD{PI_CONST / 180.0};
        uvMapOut_ = UVMap::New(*uvMapIn_);
        if (not AlmostEqual(theta_, 0.0)) {
            auto radians = theta_ * DEG_TO_RAD;
            UVMap::Rotate(*uvMapOut_, radians);
        }
    };
}

auto RotateUVMapNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta;
    meta["theta"] = theta_;
    if (useCache and uvMapOut_ and not uvMapOut_->empty()) {
        io::WriteUVMap(cacheDir / "uvMap_rot.uvm", *uvMapOut_);
        meta["uvMap"] = "uvMap_rot.uvm";
    }
    return meta;
}

void RotateUVMapNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& cacheDir)
{
    theta_ = meta["theta"].get<double>();
    if (meta.contains("uvMap")) {
        auto uvMapFile = meta["uvMap"].get<std::string>();
        uvMapOut_ = UVMap::New(io::ReadUVMap(cacheDir / uvMapFile));
    }
}

FlipUVMapNode::FlipUVMapNode()
    : Node{true}, uvMapIn{&uvMapIn_}, flipAxis{&axis_}, uvMapOut{&uvMapOut_}
{
    registerInputPort("uvMapIn", uvMapIn);
    registerInputPort("flipAxis", flipAxis);
    registerOutputPort("uvMapOut", uvMapOut);

    compute = [=]() {
        uvMapOut_ = UVMap::New(*uvMapIn_);
        UVMap::Flip(*uvMapOut_, axis_);
    };
}

auto FlipUVMapNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta;
    meta["flipAxis"] = axis_;
    if (useCache and uvMapOut_ and not uvMapOut_->empty()) {
        io::WriteUVMap(cacheDir / "uvMap_flip.uvm", *uvMapOut_);
        meta["uvMap"] = "uvMap_flip.uvm";
    }
    return meta;
}

void FlipUVMapNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& cacheDir)
{
    axis_ = meta["flipAxis"].get<FlipAxis>();
    if (meta.contains("uvMap")) {
        auto uvMapFile = meta["uvMap"].get<std::string>();
        uvMapOut_ = UVMap::New(io::ReadUVMap(cacheDir / uvMapFile));
    }
}

PlotUVMapNode::PlotUVMapNode()
    : Node{true}, uvMap{&uvMap_}, uvMesh{&uvMesh_}, plot{&plot_}
{
    registerInputPort("uvMap", uvMap);
    registerInputPort("uvMesh", uvMesh);
    registerOutputPort("plot", plot);

    compute = [=]() {
        if (uvMap_ and uvMesh_ and not uvMap_->empty()) {
            plot_ = UVMap::Plot(*uvMap_, uvMesh_);
        }
    };
}

auto PlotUVMapNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta;
    if (useCache and not plot_.empty()) {
        WriteImage(cacheDir / "uvPlot.tif", plot_);
        meta["plot"] = "uvPlot.tif";
    }
    return meta;
}

void PlotUVMapNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& cacheDir)
{
    if (meta.contains("plot")) {
        auto file = meta["plot"].get<std::string>();
        plot_ = ReadImage(cacheDir / file);
    }
}

LoadImageNode::LoadImageNode()
    : smgl::Node{true}, path{&path_}, cacheArgs{&cacheArgs_}, image{&image_}
{
    registerInputPort("path", path);
    registerInputPort("cacheArgs", cacheArgs);
    registerOutputPort("image", image);
    compute = [=]() { image_ = ReadImage(path_); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto LoadImageNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_ and not image_.empty()) {
        auto file = path_.filename().replace_extension(".tif");
        WriteImage(cacheDir / file, image_);
        meta["cachedFile"] = file.string();
    }
    return meta;
}

void LoadImageNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /** cacheDir */)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

WriteImageNode::WriteImageNode()
    : smgl::Node{true}, path{&path_}, image{&image_}, cacheArgs{&cacheArgs_}
{
    registerInputPort("path", path);
    registerInputPort("image", image);
    registerInputPort("cacheArgs", cacheArgs);
    compute = [=]() { WriteImage(path_, image_); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto WriteImageNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_ and not image_.empty()) {
        auto file = path_.filename().replace_extension(".tif");
        WriteImage(cacheDir / file, image_);
        meta["cachedFile"] = file.string();
    }

    return meta;
}

void WriteImageNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

LoadPPMNode::LoadPPMNode()
    : smgl::Node{true}, path{&path_}, cacheArgs{&cacheArgs_}, ppm{&ppm_}
{
    registerInputPort("path", path);
    registerInputPort("cacheArgs", cacheArgs);
    registerOutputPort("ppm", ppm);
    compute = [=]() { ppm_ = PerPixelMap::New(PerPixelMap::ReadPPM(path_)); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto LoadPPMNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_) {
        auto file = path_.filename().replace_extension(".ppm");
        PerPixelMap::WritePPM(cacheDir / file, *ppm_);
        meta["cachedFile"] = file.string();
    }
    return meta;
}

void LoadPPMNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /** cacheDir */)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

WritePPMNode::WritePPMNode()
    : smgl::Node{true}, path{&path_}, ppm{&ppm_}, cacheArgs{&cacheArgs_}
{
    registerInputPort("path", path);
    registerInputPort("ppm", ppm);
    registerInputPort("cacheArgs", cacheArgs);
    compute = [=]() { PerPixelMap::WritePPM(path_, *ppm_); };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto WritePPMNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_) {
        auto file = path_.filename().replace_extension(".ppm");
        PerPixelMap::WritePPM(cacheDir / file, *ppm_);
        meta["cachedFile"] = file.string();
    }

    return meta;
}

void WritePPMNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& /*cacheDir*/)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();
}

PPMPropertiesNode::PPMPropertiesNode()
    : ppm{&ppm_}
    , mask{[=]() { return ppm_->mask(); }}
    , cellMap{[=]() { return ppm_->cellMap(); }}
{
    registerInputPort("ppm", ppm);
    registerOutputPort("mask", mask);
    registerOutputPort("cellMap", cellMap);
}

LoadVolumetricMaskNode::LoadVolumetricMaskNode()
    : smgl::Node{true}
    , path{&path_}
    , cacheArgs{&cacheArgs_}
    , volumetricMask{&mask_}
{
    registerInputPort("path", path);
    registerInputPort("cacheArgs", cacheArgs);
    registerOutputPort("volumetricMask", volumetricMask);
    compute = [=]() {
        using psio = PointSetIO<cv::Vec3i>;
        mask_ = VolumetricMask::New(psio::ReadPointSet(path_));
    };
    usesCacheDir = [this]() { return cacheArgs_; };
}

auto LoadVolumetricMaskNode::serialize_(bool useCache, const fs::path& cacheDir)
    -> smgl::Metadata
{
    smgl::Metadata meta{{"path", path_.string()}, {"cacheArgs", cacheArgs_}};
    if (useCache and cacheArgs_ and mask_) {
        using psio = PointSetIO<cv::Vec3i>;
        PointSet<cv::Vec3i> ps;
        ps.append(*mask_);

        auto file = path_.filename().replace_extension(".vcps");
        psio::WritePointSet(cacheDir / file, ps);
        meta["cachedFile"] = file.string();
    }
    return meta;
}

void LoadVolumetricMaskNode::deserialize_(
    const smgl::Metadata& meta, const fs::path& cacheDir)
{
    path_ = meta["path"].get<std::string>();
    cacheArgs_ = meta["cacheArgs"].get<bool>();

    if (meta.contains("cachedFile")) {
        using psio = PointSetIO<cv::Vec3i>;
        auto file = meta["cachedFile"].get<std::string>();
        mask_ = VolumetricMask::New(psio::ReadPointSet(cacheDir / file));
    }
}