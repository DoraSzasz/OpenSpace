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

#ifndef __OPENSPACE_MODULE_BASE___RENDERABLESOLARVIDEO___H__
#define __OPENSPACE_MODULE_BASE___RENDERABLESOLARVIDEO___H__

// TODO(mnoven): A-Z
#include <modules/base/rendering/renderableplane.h>
#include <openspace/properties/scalar/doubleproperty.h>
#include <openspace/properties/scalar/intproperty.h>
#include <openspace/properties/scalar/uintproperty.h>
#include <openspace/properties/optionproperty.h>
#include <openspace/properties/scalar/boolproperty.h>
#include <openspace/properties/stringproperty.h>
#include <openspace/engine/downloadmanager.h> // Make pointer & forward declare?
#include <modules/solarbrowsing/util/spacecraftimagerymanager.h>
#include <openspace/rendering/transferfunction.h>
#include <memory>
#include <modules/solarbrowsing/util/simplej2kcodec.h>
#include <unordered_set>

#include <modules/solarbrowsing/rendering/spacecraftcameraplane.h>

#include <modules/solarbrowsing/util/streambuffer.h>
#include <modules/solarbrowsing/util/pixelbufferobject.h>
#include <modules/solarbrowsing/util/decodejob.h>
#include <modules/solarbrowsing/util/videoplayer.h>

namespace ghoul { namespace opengl { class Texture; }}

namespace openspace {

class H265Decoder;

class RenderableSolarVideo : public Renderable {

public:
    RenderableSolarVideo(const ghoul::Dictionary& dictionary);

    void render(const RenderData& data);
    void update(const UpdateData& data);
    void loadTexture();
    void performImageTimestep(const double& osTime);
    void updateTexture();

    TransferFunction* getTransferFunction() { return _lut; }
    const std::unique_ptr<ghoul::opengl::Texture>& getImageryTexture() { return _texture; }
    const SpacecraftCameraPlane& cameraPlane() {return *_spacecraftCameraPlane; };

    properties::FloatProperty _sharpenValue;
    properties::FloatProperty _contrastValue;
    properties::FloatProperty _gammaValue;

    float _currentScale;
    glm::vec2 _currentCenterPixel;
    bool _isCoronaGraph;

private:
    properties::BoolProperty _asyncUploadPBO;
    properties::OptionProperty _activeInstruments;
    properties::IntProperty _bufferSize;
    properties::FloatProperty _planeOpacity;
    properties::BoolProperty _disableBorder;
    properties::BoolProperty _disableFrustum;
    properties::BoolProperty _displayTimers;
    properties::BoolProperty _lazyBuffering;
    properties::IntProperty _minRealTimeUpdateInterval;
    properties::DoubleProperty _moveFactor;
    properties::IntProperty _resolutionLevel;
    //properties::StringProperty _target;
    properties::BoolProperty _useBuffering;
    properties::BoolProperty _usePBO;
    properties::BoolProperty _verboseMode;
    properties::DoubleProperty _planeSize;

    std::chrono::milliseconds _realTime;
    std::chrono::milliseconds _lastUpdateRealTime;
    std::unique_ptr<ghoul::opengl::Texture> _texture;
    TransferFunction* _lut;
    std::unique_ptr<PixelBufferObject> _pbo;

    // TODO: Remove these?
    bool _updatingCurrentActiveChannel = false;
    bool _initializePBO;
    bool _pboIsDirty = false;
    bool _timeToUpdateTexture = false;

    double _deltaTimeLast = 0.0;
    double _realTimeDiff;
    double _openspaceDiff;
    double _openspaceTimeLast = 0.0;
    bool _videoIsStopped = false;

    std::unique_ptr<VideoPlayer> _videoPlayer;

    std::unordered_map<std::string, TimedependentStateSequence<ImageMetadata>> _imageMetadataMap;

    int datacount  = 0;

    std::string _name;
    std::unordered_map<std::string, std::shared_ptr<TransferFunction>> _tfMap;
    std::string _currentActiveInstrument;
    VideoMetadata _cVideoMetadata;

    std::unique_ptr<SpacecraftCameraPlane> _spacecraftCameraPlane;
    std::unordered_map<std::string, VideoMetadata> _videoMetadataMap;

    DecodeData getDecodeDataFromOsTime(const int& osTime);
    void uploadImageDataToPBO();
    void updateTextureGPU(bool asyncUpload = true, bool resChanged = false);

    void decode(unsigned char* buffer, const std::string& fileame);
    bool initialize() override;
    bool deinitialize() override;
    bool isReady() const override;
    bool checkBoundaries(const RenderData& data);
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_BASE___RENDERABLESOLARVIDEO___H__
