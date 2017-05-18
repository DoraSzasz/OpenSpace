/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2017                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_MODULE_BASE___RENDERABLESPACECRAFTCAMERAPLANE___H__
#define __OPENSPACE_MODULE_BASE___RENDERABLESPACECRAFTCAMERAPLANE___H__

// TODO(mnoven): A-Z
#include <modules/base/rendering/renderableplane.h>
#include <openspace/properties/scalar/doubleproperty.h>
#include <openspace/properties/scalar/intproperty.h>
#include <openspace/properties/optionproperty.h>
#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/engine/downloadmanager.h> // Make pointer & forward declare?
#include <modules/solarbrowsing/util/spacecraftimagerymanager.h>
#include <openspace/rendering/transferfunction.h>
#include <memory>
#include <modules/solarbrowsing/util/simplej2kcodec.h>
#include <unordered_set>

#include <openspace/util/powerscaledsphere.h>
#include <modules/globebrowsing/other/concurrentjobmanager.h>

namespace ghoul { namespace opengl { class Texture; }}

namespace openspace {

struct BufferObject {
    unsigned char* data;
    std::string name;
    unsigned int size;
};

// TODO(mnoven) : Move to separate class
class DecodeJob : public globebrowsing::Job<BufferObject>{
public:
    DecodeJob(const int& imageSize, const std::string& path, const int& resolutionLevel, const bool& verboseMode)
        : _imageSize(imageSize)
        , _path(path)
        , _resolutionLevel(resolutionLevel)
        , _verboseMode(verboseMode)
    {
    }

    virtual void execute() final {
        BufferObject b = {new unsigned char[_imageSize * _imageSize], _path, _imageSize};
        SimpleJ2kCodec j2c(_verboseMode);
        j2c.DecodeIntoBuffer(b.name, b.data, _resolutionLevel);
        _bufferObject = std::make_shared<BufferObject>(b);
    }

    virtual std::shared_ptr<BufferObject> product() const final {
        return std::move(_bufferObject);
    }

protected:
    std::shared_ptr<BufferObject> _bufferObject;
    std::string _path;
    int _resolutionLevel;
    int _imageSize;
    bool _verboseMode;
};

class RenderableSpacecraftCameraPlane : public Renderable {

public:
    RenderableSpacecraftCameraPlane(const ghoul::Dictionary& dictionary);

    void render(const RenderData& data);
    void update(const UpdateData& data);
    void loadTexture();
    void performImageTimestep(const double& osTime);
    void updateTexture();

    // TODO(mnoven): Getters
    glm::dvec3 _planePosSpacecraftRefFrame;
    glm::dmat4 _sunToSpacecraftTransform;
    std::unique_ptr<ghoul::opengl::Texture> _texture;
    TransferFunction* _lut;

    float _magicPlaneFactor = 0;
    glm::dvec2 _magicPlaneOffset = glm::dvec2();


private:
    globebrowsing::ConcurrentJobManager<BufferObject> _concurrentJobManager;

    properties::BoolProperty _asyncUploadPBO;
    properties::OptionProperty _activeInstruments;
    properties::IntProperty _bufferSize;
    properties::BoolProperty _displayTimers;
    properties::BoolProperty _lazyBuffering;
    properties::IntProperty _minRealTimeUpdateInterval;
    properties::DoubleProperty _moveFactor;
    properties::IntProperty _resolutionLevel;
    properties::StringProperty _target;
    properties::BoolProperty _useBuffering;
    properties::BoolProperty _usePBO;
    properties::BoolProperty _verboseMode;
    properties::DoubleProperty _planeSize;
    properties::BoolProperty _usePerformanceResolution;

    std::chrono::milliseconds _realTime;
    std::chrono::milliseconds _lastUpdateRealTime;
    std::unique_ptr<PowerScaledSphere> _sphere;

    std::unique_ptr<ghoul::opengl::ProgramObject> _frustumShader;
    //std::unique_ptr<ghoul::opengl::ProgramObject> _sphereShader;
    std::unique_ptr<ghoul::opengl::ProgramObject> _planeShader;

    std::string _type;
    int _currentActiveImage;
    unsigned int _pboSize;
    GLuint _frustum;
    GLuint _frustumPositionBuffer;

    GLuint pboHandles[2];
    unsigned int _currentPBO;

    double _startTimeSequence;
    double _endTimeSequence;
    int _id;

    bool _updatingCurrentActiveChannel = false;
    bool _updatingCurrentLevelOfResolution = false;
    bool _updatingPerformanceResolution = false;

    std::unique_ptr<std::future<void>> _future;
    bool _initializePBO;
    bool _bufferingForwardInTime = true;
    bool _pboIsDirty = false;

    IMG_PRECISION* _pboBufferData;

    float _size;
    GLuint _quad;
    GLuint _vertexPositionBuffer;

    unsigned int _imageSize;
    unsigned int _imageSizeToRender;
    unsigned int _imageSizeLast;
    unsigned int _fullResolution;
    double _move = 0.0;
    double _deltaTimeLast = 0.0;
    double _realTimeDiff;

    bool _resolutionIsDirty = false;

    std::vector<std::unique_ptr<TransferFunction>> _transferFunctions;
    std::unordered_map<std::string, std::shared_ptr<TransferFunction>> _tfMap;

    std::string _currentActiveInstrument;
    std::vector<std::vector<ImageMetadata>> _imageMetadata;

    std::unordered_map<int, std::string> _optionToInstrument;
    std::unordered_map<std::string, std::vector<ImageMetadata>> _imageMetadataMap;
    std::unordered_set<std::string> _instrumentFilter;

    void uploadImageDataToPBO(const int& image);
    void updateTextureGPU(bool asyncUpload = true, bool resChanged = false);

    void createFrustum();
    void createPlane();
    void updatePlane();
    void fillBuffer(const double& dt);

    void decode(unsigned char* buffer, const std::string& fileame);
    void downloadTextureResource();
    bool initialize() override;
    bool deinitialize() override;
    bool isReady() const override;
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_BASE___RENDERABLESPACECRAFTCAMERAPLANE___H__
