#ifndef _VOLUMEPKG_H_
#define _VOLUMEPKG_H_

#include <cstdlib>
#include <iostream>
#include <boost/filesystem.hpp>

// These boost libraries cause problems with QT4 + Boost 1.57. This is a
// workaround.
// https://bugreports.qt.io/browse/QTBUG-22829
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#endif

#include <pcl/point_types.h>
#include "picojson.h"
#include "vc_defines.h"
#include "vc_datatypes.h"
#include "volumepkgcfg.h"
#include "volumepkg_version.h"
#include "orderedPCDMesher.h"
#include "io/objWriter.h"

class VolumePkg
{
public:
    // Constructors
    VolumePkg(const boost::filesystem::path& file_location,
              double version);  // New volpkg, V.[version]

    VolumePkg(
        const boost::filesystem::path& file_location);  // Existing VolPkgs

    // Write to disk for the first time
    int initialize();

    // Accessor for volume
    const volcart::Volume& volume() const { return vol_; }

    volcart::Volume& volume() { return vol_; }
    // Debug
    void printJSON() const { config.printObject(); };

    void printDirs() const
    {
        std::cout << "root: " << root_dir << " seg: " << segs_dir
                  << " slice: " << slice_dir << " norm: " << norm_dir
                  << std::endl;
    };

    // Metadata Retrieval
    std::string getPkgName() const;

    double getVersion() const;

    int getNumberOfSlices() const;

    int getSliceWidth() const;

    int getSliceHeight() const;

    double getVoxelSize() const;

    double getMaterialThickness() const;

    // Metadata Assignment
    bool readOnly() const { return _readOnly; };

    void readOnly(bool b) { _readOnly = b; };

    // set a metadata key to a value
    // Sorry for this templated mess. - SP 072015
    template <typename T>
    int setMetadata(const std::string& key, T value)
    {
        if (_readOnly)
            VC_ERR_READONLY();

        std::string keyType = findKeyType(key);
        if (keyType == "string") {
            try {
                std::string castValue = boost::lexical_cast<std::string>(value);
                config.setValue(key, castValue);
                return EXIT_SUCCESS;
            } catch (const boost::bad_lexical_cast&) {
                std::cerr
                    << "ERROR: Given value \"" << value
                    << "\" cannot be cast to type specified by dictionary ("
                    << keyType << ")" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (keyType == "int") {
            try {
                int castValue = boost::lexical_cast<int>(value);
                config.setValue(key, castValue);
                return EXIT_SUCCESS;
            } catch (const boost::bad_lexical_cast&) {
                std::cerr
                    << "ERROR: Given value \"" << value
                    << "\" cannot be cast to type specified by dictionary ("
                    << keyType << ")" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (keyType == "double") {
            try {
                double castValue = boost::lexical_cast<double>(value);
                config.setValue(key, castValue);
                return EXIT_SUCCESS;
            } catch (const boost::bad_lexical_cast&) {
                std::cerr
                    << "ERROR: Given value \"" << value
                    << "\" cannot be cast to type specified by dictionary ("
                    << keyType << ")" << std::endl;
                return EXIT_FAILURE;
            }
        } else if (keyType == "") {
            return EXIT_FAILURE;
        } else {
            std::cerr << "ERROR: Value \"" << value
                      << "\" not of type specified by dictionary (" << keyType
                      << ")" << std::endl;
            return EXIT_FAILURE;
        }
    };

    // Metadata Export
    void saveMetadata(const std::string& filePath);

    void saveMetadata();

    // Slice manipulation
    bool setSliceData(size_t index, const cv::Mat& slice);

    // Segmentation functions
    std::vector<std::string> getSegmentations() const;

    void setActiveSegmentation(const std::string&);

    std::string newSegmentation();

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr openCloud() const;

    std::string getMeshPath() const;

    cv::Mat getTextureData() const;

    int saveCloud(
        const pcl::PointCloud<pcl::PointXYZRGB>& segmentedCloud) const;

    int saveMesh(
        const pcl::PointCloud<pcl::PointXYZRGB>::Ptr& segmentedCloud) const;

    void saveMesh(const VC_MeshType::Pointer& mesh,
                  volcart::Texture& texture) const;

    void saveTextureData(const cv::Mat& texture,
                         const std::string& name = "textured");

    void saveTextureData(volcart::Texture texture, int index = 0)
    {
        saveTextureData(texture.getImage(index));
    }

private:
    bool _readOnly = true;

    VolumePkgCfg config;

    volcart::Volume vol_;

    // Directory tree
    int _makeDirTree();
    boost::filesystem::path root_dir;
    boost::filesystem::path segs_dir;
    boost::filesystem::path slice_dir;
    boost::filesystem::path norm_dir;

    int getNumberOfSliceCharacters();
    std::string activeSeg = "";
    std::vector<std::string> segmentations;

    std::string findKeyType(const std::string& key);
};

#endif  // _VOLUMEPKG_H_
