#include <unordered_map>

#include <boost/program_options.hpp>
#include <smgl/smgl.hpp>

#include "vc/app_support/GetMemorySize.hpp"
#include "vc/core/Version.hpp"
#include "vc/core/filesystem.hpp"
#include "vc/core/io/FileExtensionFilter.hpp"
#include "vc/core/io/PointSetIO.hpp"
#include "vc/core/types/VolumePkg.hpp"
#include "vc/core/util/Iteration.hpp"
#include "vc/core/util/Logging.hpp"
#include "vc/core/util/MemorySizeStringParser.hpp"
#include "vc/core/util/String.hpp"
#include "vc/graph.hpp"

namespace vc = volcart;
namespace fs = volcart::filesystem;
namespace po = boost::program_options;

using namespace volcart;

// Volpkg version required by this app
static constexpr int VOLPKG_SUPPORTED_VERSION = 6;

// When to smooth the mesh
enum class SmoothOpt { Off = 0, Before, After, Both };

// Flattening algorithm opt
enum class FlatteningAlgorithm { ABF = 0, LSCM, Orthographic };

// Available texturing algorithms
enum class Method { Composite = 0, Intersection, Integral, Thickness };

static auto GetGeneralOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("General Options");
    opts.add_options()
        ("help,h", "Show this message")
        ("cache-memory-limit", po::value<std::string>(),
         "Maximum size of the slice cache in bytes. Accepts the suffixes: "
         "(K|M|G|T)(B). Default: 50% of the total system memory.")
        ("log-level", po::value<std::string>()->default_value("info"),
         "Options: off, critical, error, warn, info, debug");
    // clang-format on
    return opts;
}

static auto GetIOOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Input/Output Options");
    opts.add_options()
    ("volpkg,v", po::value<std::string>()->required(), "VolumePkg path")
    ("seg,s", po::value<std::string>(), "Segmentation ID")
    ("input-mesh", po::value<std::string>(), "Path to input OBJ or PLY")
    ("volume", po::value<std::string>(),
        "Volume to use for texturing. Default: Segmentation's associated "
        "volume or the first volume in the volume package.")
    ("output-file,o", po::value<std::string>(),
        "Output file path. If not specified, an OBJ file and texture image "
        "will be placed in the current working directory.")
    ("output-ppm", po::value<std::string>(),
        "Output file path for the generated PPM.")
    ("save-graph", po::value<bool>()->default_value(true),
        "Save the generated render graph into the volume package.");
    // clang-format on

    return opts;
}

static auto GetMeshingOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Meshing Options");
    opts.add_options()
        ("scale-mesh", po::value<double>(), "Scale the mesh by a linear scale "
            "factor")
        ("enable-mesh-resampling", "Enable ACVD mesh resampling. Automatically "
            "enabled if the input is a Segmentation")
        ("mesh-resample-factor", po::value<double>()->default_value(50),
            "Roughly, the number of vertices per square millimeter in the "
            "output mesh")
        ("mesh-resample-vcount", po::value<std::size_t>(), "The target number of vertices "
            "in the resampled mesh. If specified, overrides both the mesh-resample-factor "
            "and mesh-resample-keep-vcount options.")
        ("mesh-resample-keep-vcount", "If enabled, mesh resampling will "
            "attempt to maintain the number of vertices in the input mesh. "
            "Overrides the value set by --mesh-resample-factor.")
        ("mesh-resample-anisotropic", "Enable the anisotropic extension "
            "of the mesh resampler.")
        ("mesh-resample-gradation", po::value<double>()->default_value(0),
            "Set the resampling gradation constraint.")
        ("mesh-resample-quadrics-level", po::value<std::size_t>()->default_value(1),
            "Set the mesh resampling quadrics optimization level.")
        ("mesh-resample-smoothing", po::value<int>()->default_value(0),
            "Smoothing Options:\n"
            "  0 = Off\n"
            "  1 = Before mesh resampling\n"
            "  2 = After mesh resampling\n"
            "  3 = Both before and after mesh resampling")
        ("intermediate-mesh", po::value<std::string>(),"Output file path for the "
            "intermediate (i.e. scale + resampled) mesh. File is saved prior "
            "to flattening. Useful for testing meshing parameters.");
    // clang-format on

    return opts;
}

static auto GetUVOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Flattening & UV Options");
    opts.add_options()
        ("uv-algorithm", po::value<int>()->default_value(0),
            "Select the flattening algorithm:\n"
                "  0 = ABF\n"
                "  1 = LSCM\n"
                "  2 = Orthographic Projection")
        ("uv-reuse", "If input-mesh is specified, attempt to use its existing "
            "UV map instead of generating a new one.")
        ("uv-rotate", po::value<double>(), "Rotate the generated UV map by an "
            "angle in degrees (counterclockwise).")
        ("uv-flip", po::value<int>(),
            "Flip the UV map along an axis. If uv-rotate is specified, flip is "
            "performed after rotation.\n"
            "Axis along which to flip:\n"
                "  0 = Vertical\n"
                "  1 = Horizontal\n"
                "  2 = Both")
        ("uv-plot", po::value<std::string>(), "Plot the UV points and save "
            "it to the provided image path.")
        ("uv-plot-error", po::value<std::string>(), "Plot the UV L-stretch "
            "error metrics and save them to the provided image path. The "
            "provided filename will have \'_l2\' and \'lInf\' appended for "
            "each metric: e.g. providing \'foo.png\' to this argument will "
            "produce image files \'foo_l2.png\' and \'foo_lInf.png\'")
        ("uv-plot-error-legend", po::value<bool>()->default_value(true),
            "If enabled (default), add a legend to the UV error plot images");
    // clang-format on

    return opts;
}

static auto GetFilteringOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Generic Texture Filtering Options");
    opts.add_options()
        ("method,m", po::value<int>()->default_value(0),
             "Texturing method: \n"
                 "  0 = Composite\n"
                 "  1 = Intersection\n"
                 "  2 = Integral\n"
                 "  3 = Thickness")
        ("neighborhood-shape,n", po::value<int>()->default_value(0),
             "Neighborhood shape:\n"
                 "  0 = Linear\n"
                 "  1 = Cuboid")
        ("radius,r", po::value<std::vector<double>>()->multitoken(), "Search "
            "radius. Defaults to value calculated from estimated layer "
            "thickness.")
        ("interval,i", po::value<double>()->default_value(1.0),
            "Sampling interval")
        ("direction,d", po::value<int>()->default_value(0),
            "Sample Direction:\n"
                " -1 = Negative\n"
                "  0 = Omni\n"
                "  1 = Positive")
        ("shading", po::value<int>()->default_value(1),
            "Surface Normal Shading:\n"
                "  0 = Flat\n"
                "  1 = Smooth");
    // clang-format on

    return opts;
}

static auto GetCompositeOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Composite Texture Options");
    opts.add_options()
        ("filter,f", po::value<int>()->default_value(1),
            "Filter:\n"
                "  0 = Minimum\n"
                "  1 = Maximum\n"
                "  2 = Median\n"
                "  3 = Mean\n"
                "  4 = Median w/ Averaging");
    // clang-format on

    return opts;
}

static auto GetIntegralOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Integral Texture Options");
    opts.add_options()
        ("weight-type,w", po::value<int>()->default_value(0),
            "Weight Type:\n"
                "  0 = None\n"
                "  1 = Linear\n"
                "  2 = Exponential Difference")
        ("linear-weight-direction", po::value<int>()->default_value(0),
            "Linear Weight Direction:\n"
                "  0 = Favor the + normal direction\n"
                "  1 = Favor the - normal direction")
        ("expodiff-exponent", po::value<int>()->default_value(2), "Exponent "
            "applied to the absolute difference values.")
        ("expodiff-base-method", po::value<int>()->default_value(0),
            "Exponential Difference Base Calculation Method:\n"
                "  0 = Mean\n"
                "  1 = Mode\n"
                "  2 = Manually specified")
        ("expodiff-base", po::value<double>()->default_value(0.0), "If the "
            "base calculation method is set to Manual, the value from which "
            "voxel values are differenced.")
        ("clamp-to-max", po::value<uint16_t>(), "Clamp values to the specified "
            "maximum.");
    // clang-format on

    return opts;
}

static auto GetThicknessOpts() -> po::options_description
{
    // clang-format off
    po::options_description opts("Thickness Texture Options");
    opts.add_options()
        ("volume-mask", po::value<std::string>(),
            "Path to volumetric mask point set")
        ("normalize-output", po::value<bool>()->default_value(true),
            "Normalize the output image between [0, 1]");
    // clang-format on

    return opts;
}

auto main(int argc, char* argv[]) -> int
{
    ///// Parse the command line options /////
    po::options_description all("Usage");
    all.add(GetGeneralOpts())
        .add(GetIOOpts())
        .add(GetMeshingOpts())
        .add(GetUVOpts())
        .add(GetFilteringOpts())
        .add(GetCompositeOpts())
        .add(GetIntegralOpts())
        .add(GetThicknessOpts());

    // Parse the cmd line
    po::variables_map parsed;
    try {
        po::store(
            po::command_line_parser(argc, argv).options(all).run(), parsed);
    } catch (const po::error& e) {
        Logger()->error(e.what());
        return EXIT_FAILURE;
    }

    // Show the help message
    if (parsed.count("help") > 0 || argc < 5) {
        std::cout << all << std::endl;
        return EXIT_SUCCESS;
    }

    // Warn of missing options
    try {
        po::notify(parsed);
    } catch (po::error& e) {
        Logger()->error(e.what());
        return EXIT_FAILURE;
    }

    // Set logging level
    auto logLevel = parsed["log-level"].as<std::string>();
    to_lower(logLevel);
    logging::SetLogLevel(logLevel);

    // Register VC graph nodes
    vc::RegisterNodes();

    ///// Load the volume package /////
    fs::path volpkgPath = parsed["volpkg"].as<std::string>();
    Logger()->info(
        "Loading VolumePkg: {}",
        fs::weakly_canonical(volpkgPath).filename().string());
    VolumePkg::Pointer vpkg;
    try {
        vpkg = VolumePkg::New(volpkgPath);
    } catch (const std::exception& e) {
        Logger()->critical(e.what());
        return EXIT_FAILURE;
    }

    if (vpkg->version() != VOLPKG_SUPPORTED_VERSION) {
        Logger()->error(
            "Volume Package is version {} but this program requires version {}",
            vpkg->version(), VOLPKG_SUPPORTED_VERSION);
        return EXIT_FAILURE;
    }

    //// Create the graph pipeline ////
    std::shared_ptr<smgl::Graph> graph;
    if (parsed["save-graph"].as<bool>()) {
        auto render = vpkg->newRender();
        graph = render->graph();
        vc::Logger()->info(
            "Created new Render graph in VolPkg: {}", render->id());
    } else {
        graph = std::make_shared<smgl::Graph>();
    }

    // Add the project metadata
    // clang-format off
    smgl::Metadata projectInfo{{
        vc::ProjectInfo::Name(), {
            {"version", ProjectInfo::VersionString()},
            {"git-url", ProjectInfo::RepositoryURL()},
            {"git-hash", ProjectInfo::RepositoryHash()},
            {"volpkg", {
                {"path", fs::weakly_canonical(volpkgPath).filename().string()},
                {"name", vpkg->name()}
            }}
        }}
    };
    // clang-format on
    graph->setProjectMetadata(projectInfo);
    // Setup a map to keep a reference to important output ports
    std::unordered_map<std::string, smgl::Output*> results;

    //// Load the segmentation/mesh ////
    bool loadSeg = parsed.count("seg") > 0;
    bool loadMesh = parsed.count("input-mesh") > 0;
    if (loadSeg && loadMesh) {
        Logger()->error(
            "Specified mutually exclusive flags: --seg and --input-mesh");
        return EXIT_FAILURE;
    } else if (not loadSeg and not loadMesh) {
        Logger()->error("Missing required flag: --seg or --input-mesh");
        return EXIT_FAILURE;
    }

    std::string outStem;
    if (loadSeg) {
        auto id = parsed["seg"].as<std::string>();
        outStem = id;

        auto seg = graph->insertNode<SegmentationSelectorNode>();
        seg->volpkg = vpkg;
        seg->id = id;

        auto getPts = graph->insertNode<SegmentationPropertiesNode>();
        getPts->segmentation = seg->segmentation;

        auto mesher = graph->insertNode<MeshingNode>();
        mesher->points = getPts->pointSet;
        results["mesh"] = &mesher->mesh;
    } else {
        fs::path inputPath = parsed["input-mesh"].as<std::string>();
        outStem = inputPath.stem().string();

        auto reader = graph->insertNode<LoadMeshNode>();
        reader->path = inputPath;
        reader->cacheArgs = true;
        results["mesh"] = &reader->mesh;
        if (parsed.count("uv-reuse") > 0) {
            results["uvMap"] = &reader->uvMap;
        }
    }

    //// Setup the output file path ////
    fs::path outputPath;
    if (parsed.count("output-file") > 0) {
        outputPath = parsed["output-file"].as<std::string>();
    } else {
        outputPath = outStem + "_render.obj";
    }

    //// Load the Volume ////
    if (not vpkg->hasVolumes()) {
        Logger()->error("Volume package does not contain any volumes");
        return EXIT_FAILURE;
    }
    auto volumeSelector = graph->insertNode<VolumeSelectorNode>();
    volumeSelector->volpkg = vpkg;
    Volume::Identifier volId;
    if (parsed.count("volume") > 0) {
        volId = parsed["volume"].as<std::string>();
    } else if (loadSeg) {
        auto segId = parsed["seg"].as<std::string>();
        auto seg = vpkg->segmentation(segId);
        if (seg->hasVolumeID()) {
            volId = seg->getVolumeID();
            Logger()->debug(
                "Selecting segmentation associated volume: {}", volId);
        }
    }
    if (not volId.empty() and not vpkg->hasVolume(volId)) {
        Logger()->error(
            "Volume package does not contain volume with ID: {}", volId);
        return EXIT_FAILURE;
    }
    volumeSelector->id = volId;

    // Set the cache size
    std::size_t cacheBytes{2'000'000'000};
    if (parsed.count("cache-memory-limit") > 0) {
        auto cacheSizeOpt = parsed["cache-memory-limit"].as<std::string>();
        cacheBytes = MemorySizeStringParser(cacheSizeOpt);
    } else {
        cacheBytes = SystemMemorySize() / 2;
    }
    auto volumeProps = graph->insertNode<VolumePropertiesNode>();
    volumeProps->volumeIn = volumeSelector->volume;
    volumeProps->cacheMemory = cacheBytes;
    results["volume"] = &volumeProps->volumeOut;

    //// Scale the mesh /////
    if (parsed.count("scale-mesh") > 0) {
        auto scaleMesh = graph->insertNode<ScaleMeshNode>();
        scaleMesh->input = *results["mesh"];
        scaleMesh->scaleFactor = parsed["scale-mesh"].as<double>();
        results["mesh"] = &scaleMesh->output;
    }

    ///// Resample and smooth the mesh /////
    auto needResample = loadSeg || parsed.count("enable-mesh-resampling") > 0;
    if (needResample) {
        // Pre-smooth
        auto smoothType =
            static_cast<SmoothOpt>(parsed["mesh-resample-smoothing"].as<int>());
        if (smoothType == SmoothOpt::Both || smoothType == SmoothOpt::Before) {
            auto smooth = graph->insertNode<LaplacianSmoothMeshNode>();
            smooth->input = *results["mesh"];
            results["mesh"] = &smooth->output;
        }

        // Setup resampling
        auto resample = graph->insertNode<ResampleMeshNode>();
        resample->input = *results["mesh"];
        if (parsed.count("mesh-resample-anisotropic") > 0) {
            resample->mode = ResampleMeshNode::Mode::Anisotropic;
        } else {
            resample->mode = ResampleMeshNode::Mode::Isotropic;
        }
        resample->gradation = parsed["mesh-resample-gradation"].as<double>();
        resample->quadricsOptimizationLevel =
            parsed["mesh-resample-quadrics-level"].as<std::size_t>();

        if (parsed.count("mesh-resample-vcount") > 0) {
            resample->numVertices =
                parsed["mesh-resample-vcount"].as<std::size_t>();
        }

        else if (parsed.count("mesh-resample-keep-vcount") > 0) {
            auto meshProps = graph->insertNode<MeshPropertiesNode>();
            meshProps->mesh = *results["mesh"];
            resample->numVertices = meshProps->numVertices;
        }

        else {
            auto calcVerts = graph->insertNode<CalculateNumVertsNode>();
            calcVerts->mesh = *results["mesh"];
            calcVerts->voxelSize = volumeProps->voxelSize;
            calcVerts->density = parsed["mesh-resample-factor"].as<double>();
            resample->numVertices = calcVerts->numVerts;
        }
        results["mesh"] = &resample->output;

        // Post-smooth
        if (smoothType == SmoothOpt::Both || smoothType == SmoothOpt::After) {
            auto smooth = graph->insertNode<LaplacianSmoothMeshNode>();
            smooth->input = *results["mesh"];
            results["mesh"] = &smooth->output;
        }

        // Save the intermediate mesh
        if (parsed.count("intermediate-mesh") > 0) {
            fs::path meshPath = parsed["intermediate-mesh"].as<std::string>();
            auto writer = graph->insertNode<WriteMeshNode>();
            writer->path = meshPath;
            writer->mesh = *results["mesh"];
        }
    }

    ///// Flattening /////
    if (needResample and results.count("uvMap") > 0) {
        Logger()->warn(
            "Provided '--uv-reuse' option, but input mesh has been "
            "resampled. Ignoring loaded UV map.");
        results.erase("uvMap");
    }

    // Compute a UV map if we're not using a loaded one
    if (results.count("uvMap") == 0) {
        auto method =
            static_cast<FlatteningAlgorithm>(parsed["uv-algorithm"].as<int>());
        if (method == FlatteningAlgorithm::ABF ||
            method == FlatteningAlgorithm::LSCM) {
            auto flatten = graph->insertNode<ABFNode>();
            flatten->input = *results["mesh"];
            flatten->useABF = (method == FlatteningAlgorithm::ABF);
            results["uvMap"] = &flatten->uvMap;
            results["uvMesh"] = &flatten->output;

            auto calcError = graph->insertNode<FlatteningErrorNode>();
            calcError->mesh3D = *results["mesh"];
            calcError->mesh2D = flatten->output;
            results["flatteningError"] = &calcError->error;
        }

        // Orthographic
        else if (method == FlatteningAlgorithm::Orthographic) {
            auto flatten = graph->insertNode<OrthographicFlatteningNode>();
            flatten->input = *results["mesh"];
            results["uvMap"] = &flatten->uvMap;
            results["uvMesh"] = &flatten->output;
        }

        // Error
        else {
            Logger()->error(
                "Provided unrecognized flattening option: {}",
                parsed["uv-algorithm"].as<int>());
            return EXIT_FAILURE;
        }
    }

    // Rotate
    if (parsed.count("uv-rotate") > 0) {
        auto rotate = graph->insertNode<RotateUVMapNode>();
        rotate->uvMapIn = *results["uvMap"];
        rotate->theta = parsed["uv-rotate"].as<double>();
        results["uvMap"] = &rotate->uvMapOut;

        // UV Mesh needs to be recalculated after transform
        results.erase("uvMesh");
    }

    // Flip
    if (parsed.count("uv-flip") > 0) {
        auto axis = static_cast<UVMap::FlipAxis>(parsed["uv-flip"].as<int>());
        auto flip = graph->insertNode<FlipUVMapNode>();
        flip->uvMapIn = *results["uvMap"];
        flip->flipAxis = axis;
        results["uvMap"] = &flip->uvMapOut;

        // UV Mesh needs to be recalculated after transform
        results.erase("uvMesh");
    }

    // UV plotting options
    auto plotUV = parsed.count("uv-plot") > 0;
    auto plotUVError = parsed.count("uv-plot-error") > 0;

    // Make a UV Mesh if we don't have one yet
    if ((plotUV or plotUVError) and results.count("uvMesh") == 0) {
        auto mesher = graph->insertNode<UVMapToMeshNode>();
        mesher->inputMesh = *results["mesh"];
        mesher->uvMap = *results["uvMap"];
        mesher->scaleToUVDimensions = true;
        results["uvMesh"] = &mesher->outputMesh;
    }

    // Plot the UV Map
    if (plotUV) {
        auto plot = graph->insertNode<PlotUVMapNode>();
        plot->uvMap = *results["uvMap"];
        plot->uvMesh = *results["uvMesh"];

        auto writer = graph->insertNode<WriteImageNode>();
        writer->path = parsed["uv-plot"].as<std::string>();
        writer->image = plot->plot;
    }

    // Generate the PPM
    using Shading = PPMGeneratorNode::Shading;
    auto ppmGen = graph->insertNode<PPMGeneratorNode>();
    ppmGen->mesh = *results["mesh"];
    ppmGen->uvMap = *results["uvMap"];
    ppmGen->shading = static_cast<Shading>(parsed["shading"].as<int>());

    // Save the PPM
    if (parsed.count("output-ppm") > 0) {
        auto writer = graph->insertNode<WritePPMNode>();
        writer->path = parsed["output-ppm"].as<std::string>();
        writer->ppm = ppmGen->ppm;
    }

    // Plot the UV error maps
    // Needs PPM to compute
    if (plotUVError) {
        if (results.count("flatteningError") == 0) {
            auto calcError = graph->insertNode<FlatteningErrorNode>();
            calcError->mesh3D = *results["mesh"];
            calcError->mesh2D = *results["uvMesh"];
            results["flatteningError"] = &calcError->error;
        }

        // PPM properties
        auto ppmProps = graph->insertNode<PPMPropertiesNode>();
        ppmProps->ppm = ppmGen->ppm;

        // Generate the error plots
        auto plotErr = graph->insertNode<PlotLStretchErrorNode>();
        plotErr->error = *results["flatteningError"];
        plotErr->cellMap = ppmProps->cellMap;
        plotErr->drawLegend = parsed["uv-plot-error-legend"].as<bool>();

        // Save the images
        fs::path baseName = parsed["uv-plot-error"].as<std::string>();
        auto l2File =
            baseName.stem().string() + "_l2" + baseName.extension().string();
        auto writerL2 = graph->insertNode<WriteImageNode>();
        writerL2->path = baseName.parent_path() / l2File;
        writerL2->image = plotErr->l2Plot;

        auto lInfFile =
            baseName.stem().string() + "_lInf" + baseName.extension().string();
        auto writerLInf = graph->insertNode<WriteImageNode>();
        writerLInf->path = baseName.parent_path() / lInfFile;
        writerLInf->image = plotErr->lInfPlot;
    }

    // Neighborhood generator
    Method method = static_cast<Method>(parsed["method"].as<int>());
    if (method != Method::Intersection and method != Method::Thickness) {
        using Shape = NeighborhoodGeneratorNode::Shape;
        auto neighborGen = graph->insertNode<NeighborhoodGeneratorNode>();
        neighborGen->shape =
            static_cast<Shape>(parsed["neighborhood-shape"].as<int>());
        neighborGen->interval = parsed["interval"].as<double>();
        neighborGen->direction =
            static_cast<Direction>(parsed["direction"].as<int>());

        // Calculate neighbordhood radius
        if (parsed.count("radius") > 0) {
            cv::Vec3d radius;
            auto parsedRadius = parsed["radius"].as<std::vector<double>>();
            radius[0] = parsedRadius[0];
            radius[1] = radius[2] = std::sqrt(std::abs(radius[0]));
            if (parsedRadius.size() >= 2) {
                radius[1] = parsedRadius[1];
            }
            if (parsedRadius.size() >= 3) {
                radius[2] = parsedRadius[2];
            }
            neighborGen->radius = radius;
        } else {
            auto radiusCalc =
                graph->insertNode<CalculateNeighborhoodRadiusNode>();
            radiusCalc->thickness = vpkg->materialThickness();
            radiusCalc->voxelSize = volumeProps->voxelSize;
            neighborGen->radius = radiusCalc->radius;
        }

        results["generator"] = &neighborGen->generator;
    }

    // Setup texturing method
    smgl::Node::Pointer texturing;
    if (method == Method::Intersection) {
        auto t = graph->insertNode<IntersectionTextureNode>();
        texturing = t;
    }

    else if (method == Method::Composite) {
        using Filter = CompositeTextureNode::Filter;
        auto filter = static_cast<Filter>(parsed["filter"].as<int>());
        auto t = graph->insertNode<CompositeTextureNode>();
        t->generator = *results["generator"];
        t->filter = filter;
        texturing = t;
    }

    else if (method == Method::Integral) {
        using WeightMethod = IntegralTextureNode::WeightMethod;
        using WeightDirection = IntegralTextureNode::WeightDirection;
        using ExpoDiffBaseMethod = IntegralTextureNode::ExpoDiffBaseMethod;

        auto wType = static_cast<WeightMethod>(parsed["weight-type"].as<int>());
        auto wDir = static_cast<WeightDirection>(
            parsed["linear-weight-direction"].as<int>());
        auto wExponent = parsed["expodiff-exponent"].as<int>();
        auto expoMethod = static_cast<ExpoDiffBaseMethod>(
            parsed["expodiff-base-method"].as<int>());
        auto expoDiffBase = parsed["expodiff-base"].as<double>();
        auto clampToMax = parsed.count("clamp-to-max") > 0;

        auto t = graph->insertNode<IntegralTextureNode>();
        t->generator = *results["generator"];
        t->weightMethod = wType;
        t->linearWeightDirection = wDir;
        t->exponentialDiffExponent = wExponent;
        t->exponentialDiffBaseMethod = expoMethod;
        t->exponentialDiffBaseValue = expoDiffBase;
        t->clampValuesToMax = clampToMax;
        if (clampToMax) {
            t->clampMax = parsed["clamp-to-max"].as<uint16_t>();
        }
        texturing = t;
    }

    else if (method == Method::Thickness) {
        // Load mask
        if (parsed.count("volume-mask") == 0) {
            vc::Logger()->error(
                "Selected Thickness texturing, but did not provide volume mask "
                "path.");
            std::exit(EXIT_FAILURE);
        }
        auto reader = graph->insertNode<LoadVolumetricMaskNode>();
        reader->cacheArgs = true;
        reader->path = parsed["volume-mask"].as<std::string>();

        auto t = graph->insertNode<ThicknessTextureNode>();
        t->volumetricMask = reader->volumetricMask;
        t->normalizeOutput = parsed["normalize-output"].as<bool>();
        t->samplingInterval = parsed["interval"].as<double>();
        texturing = t;
    }

    // Set/get generic parameters
    texturing->getInputPort("ppm") = ppmGen->ppm;
    texturing->getInputPort("volume") = *results["volume"];
    results["texture"] = &texturing->getOutputPort("texture");

    // Save final outputs
    if (vc::IsFileType(outputPath, {"png", "jpg", "jpeg", "tiff", "tif"})) {
        auto writer = graph->insertNode<WriteImageNode>();
        writer->path = outputPath;
        writer->image = *results["texture"];
    } else if (vc::IsFileType(outputPath, {"obj", "ply"})) {
        auto writer = graph->insertNode<WriteMeshNode>();
        writer->path = outputPath;
        writer->mesh = *results["mesh"];
        writer->uvMap = *results["uvMap"];
        writer->texture = *results["texture"];
    } else {
        vc::Logger()->error(
            "Unrecognized output format: {}", outputPath.extension().string());
    }

    // Update the graph
    try {
        graph->update();
    } catch (const std::exception& e) {
        Logger()->error(e.what());
        return EXIT_FAILURE;
    }
}
