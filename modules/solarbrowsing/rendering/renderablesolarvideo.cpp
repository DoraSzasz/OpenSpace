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

#include <modules/solarbrowsing/rendering/renderablesolarvideo.h>

#include <ghoul/logging/logmanager.h>
#include <openspace/engine/openspaceengine.h>
#include <openspace/scene/scenegraphnode.h>
#include <openspace/rendering/renderengine.h>

// TODO(mnoven) CLEAN REDUNDANT STUFF
#include <ghoul/filesystem/filesystem>
#include <ghoul/io/texture/texturereader.h>
#include <ghoul/opengl/programobject.h>
#include <ghoul/opengl/texture.h>
#include <ghoul/opengl/textureunit.h>

#include <ghoul/filesystem/filesystem.h>
#include <modules/solarbrowsing/util/h265decoder.h>

#include <openspace/util/time.h>
#include <openspace/util/timemanager.h>
#include <openspace/scene/scene.h>

#include <modules/solarbrowsing/util/videoplayer.h>

#include <chrono>
#include <math.h>

using namespace ghoul::opengl;
using namespace std::chrono;

typedef std::chrono::high_resolution_clock Clock;

namespace {
    static const std::string _loggerCat = "RenderableSolarVideo";
    const double EPSILON = std::numeric_limits<double>::epsilon();
}

namespace openspace {

RenderableSolarVideo::RenderableSolarVideo(const ghoul::Dictionary& dictionary)
    : Renderable(dictionary)
    , _sharpenValue("sharpenValue", "Sharpen", 0.0, 0.0, 1.0)
    , _contrastValue("contrastValue", "Contrast", 0.0, -15.0, 15.0)
    , _planeOpacity("planeOpacity", "Plane Opacity", 1.0, 0.0, 1.0)
    , _disableFrustum("disableFrustum", "Disable Frustum", true)
    , _disableBorder("disableBorder", "Disable Border", true)
    , _gammaValue("gammaValue", "Gamma", 0.9, 0.1, 10.0)
    , _asyncUploadPBO("asyncUploadPBO", "Upload to PBO Async", true)
    , _activeInstruments("activeInstrument", "Active Instrument", properties::OptionProperty::DisplayType::Radio)
    , _bufferSize("bufferSize", "Buffer Size", 5, 1, 300)
    , _displayTimers("displayTimers", "Display Timers", false)
    , _lazyBuffering("lazyBuffering", "Lazy Buffering", true)
    , _minRealTimeUpdateInterval("minRealTimeUpdateInterval", "Min Update Interval", 85, 0, 300)
    , _moveFactor("movefactor", "Move Factor" , 1.0, 0.0, 1.0)
    , _resolutionLevel("resolutionlevel", "Level of detail", 3, 0, 5)
    , _useBuffering("useBuffering", "Use Buffering", true)
    , _usePBO("usePBO", "Use PBO", true)
    , _planeSize("planeSize", "Plane Size", 50.0, 0.0, 1.0)
    , _verboseMode("verboseMode", "Verbose Mode", false)
{

    if (!dictionary.getValue("Name", _name)) {
        throw ghoul::RuntimeError("Nodename has to be specified");
    }

    std::string rootPath;
    if (!dictionary.getValue("RootPath", rootPath)) {
        throw ghoul::RuntimeError("RootPath has to be specified");
    }

    SpacecraftImageryManager::ref().loadVideoMetadata(rootPath, _videoMetadataMap);
    // Add GUI names
    unsigned int i = 0;
    for (auto& el : _videoMetadataMap) {
        _activeInstruments.addOption(i++, el.first);
    }

    _currentActiveInstrument
          = _activeInstruments.getDescriptionByValue(_activeInstruments.value());
    _cVideoMetadata = _videoMetadataMap[_currentActiveInstrument];

    std::string tfRootPath;
    if (dictionary.getValue("TransferfunctionPath", tfRootPath)) {
        SpacecraftImageryManager::ref().loadTransferFunctions(tfRootPath, _tfMap);
    }

    int imageSize = _cVideoMetadata.resolution;
    _pbo = std::make_unique<PixelBufferObject>(imageSize * imageSize * sizeof(IMG_PRECISION));

    // TODO(mnoven): Faster to send GL_RGBA32F as internal format - GPU Pads anyways?
    _texture =  std::make_unique<Texture>(
                    nullptr,
                    glm::size3_t(imageSize, imageSize, 1),
                    ghoul::opengl::Texture::Red, // Format of the pixeldata
                    GL_R8, // INTERNAL format. More preferable to give explicit precision here, otherwise up to the driver to decide
                    GL_UNSIGNED_BYTE, // Type of data
                    Texture::FilterMode::Linear,
                    Texture::WrappingMode::ClampToEdge
                );

    _texture->setDataOwnership(ghoul::Boolean::No);
    _texture->uploadTexture();

    // Initialize time
    _realTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    _lastUpdateRealTime = _realTime;

    _activeInstruments.onChange([this]() {
        _pboIsDirty = false;
        _currentActiveInstrument
               = _activeInstruments.getDescriptionByValue(_activeInstruments.value());
        // Upload asap
        updateTextureGPU(/*asyncUpload=*/);
        // if (_useBuffering) {
        //     fillBuffer(OsEng.timeManager().time().deltaTime());
        // }
    });

    std::string metadataPath;
    if (!dictionary.getValue("MetadataPath", metadataPath)) {
        throw ghoul::RuntimeError("MetadataPath has to be specified");
    }

    SpacecraftImageryManager::ref().loadImageMetadata(metadataPath, _imageMetadataMap);

    _currentScale = _cVideoMetadata.scale;
    _currentCenterPixel = _cVideoMetadata.centerPixel;
    _isCoronaGraph = false;

    _videoPlayer = std::make_unique<VideoPlayer>();
    _videoPlayer->initialize(_cVideoMetadata);
    _videoPlayer->run();

    _openspaceTimeLast = OsEng.timeManager().time().j2000Seconds();
    // _resolutionLevel.onChange([this]() {
    //     _pboIsDirty = false;
    //     _imageSize = _fullResolution / (pow(2, _resolutionLevel));
    //     _pbo->setSize(_imageSize * _imageSize * sizeof(IMG_PRECISION));
    //     updateTextureGPU(/*asyncUpload=*/false, /*resChanged=*/true);
    //     if (_useBuffering) {
    //         fillBuffer(OsEng.timeManager().time().deltaTime());
    //     } /*else {
    //         uploadImageDataToPBO();
    //     }*/
    //    // _updatingCurrentLevelOfResolution = false;
    // });

    _moveFactor.onChange([this]() {
        _spacecraftCameraPlane->createPlaneAndFrustum(_moveFactor);
    });

    _deltaTimeLast = 1.0;

    // if (_useBuffering) {
    //     fillBuffer(OsEng.timeManager().time().deltaTime());
    // }

    // If no buffer is used this is needed
    _initializePBO = true;

    // Initialize PBO - not needed since buffer is filled async anyways
    // if (_usePBO) {
    //     LDEBUG("Initializing PBO with image " << _currentActiveImage);
    //     uploadImageDataToPBO(_currentActiveImage); // Begin fill PBO 1
    // }

    performImageTimestep(OsEng.timeManager().time().j2000Seconds());

    addProperty(_planeOpacity);
    addProperty(_disableBorder);
    addProperty(_disableFrustum);
    addProperty(_activeInstruments);
    addProperty(_sharpenValue);
    addProperty(_gammaValue);
    addProperty(_contrastValue);
    addProperty(_bufferSize);
    addProperty(_displayTimers);
    addProperty(_planeSize);
    addProperty(_resolutionLevel);
    addProperty(_lazyBuffering);
    addProperty(_minRealTimeUpdateInterval);
    //addProperty(_asyncUploadPBO);
    addProperty(_useBuffering);
    addProperty(_usePBO);
    addProperty(_moveFactor);
    addProperty(_verboseMode);
}

// DecodeData RenderableSolarVideo::getDecodeDataFromOsTime(const int& osTime) {
//     auto& stateSequence = _imageMetadataMap2[_currentActiveInstrument];
//     auto& state = stateSequence.getState(osTime);
//     const double& timeObserved = state.timeObserved();
//     std::shared_ptr<ImageMetadata> im = state.contents();
//     //const std::string stateFilename = state.contents()->filename;
//     //DecodeData decodeData {_imageSize * _imageSize, stateFilename, _resolutionLevel, _verboseMode, timeObserved};

//     DecodeData decodeData {std::move(im), static_cast<unsigned int>(_resolutionLevel), timeObserved, _verboseMode};
//     return std::move(decodeData);
// }

// void RenderableSolarVideo::fillBuffer(const double& dt) {
//     _streamBuffer.clear();
//     if (_verboseMode) {
//         LDEBUG("Refilling Buffer. dt: " << dt);
//     }
//     const double& osTime = OsEng.timeManager().time().j2000Seconds();
//     const double& startTime = getDecodeDataFromOsTime(osTime).timeObserved;

//     for (int i = 1; i < _bufferSize; i++) {
//         const double& time = startTime + (dt * i) * (static_cast<double>(_minRealTimeUpdateInterval) / 1000.0);
//         DecodeData decodeData = getDecodeDataFromOsTime(time);
//         auto job = std::make_shared<DecodeJob>(decodeData, decodeData.im->filename + std::to_string(decodeData.im->fullResolution));
//         _streamBuffer.enqueueJob(job);
//         if (_verboseMode) {
//             LDEBUG("Enqueueing " << decodeData.im->filename);
//         }
//     }
// }

bool RenderableSolarVideo::isReady() const {
    return _spacecraftCameraPlane->isReady() && _texture != nullptr;
}

bool RenderableSolarVideo::initialize() {
    _spacecraftCameraPlane = std::make_unique<SpacecraftCameraPlane>(
          /*_magicPlaneOffset, _magicPlaneFactor,*/ _moveFactor);
    return isReady();
}

bool RenderableSolarVideo::deinitialize() {
    return _spacecraftCameraPlane->destroy();
}

void RenderableSolarVideo::uploadImageDataToPBO() {
    _pbo->activate();
    IMG_PRECISION* _pboBufferData = _pbo->mapToClientMemory<IMG_PRECISION>(/*shouldOrphanData=*/true);

    if (OsEng.timeManager().time().deltaTime() > 0) {
        const unsigned char* someData = _videoPlayer->popFrame();
        if (someData) {
            //std::cerr << "Got some data " << datacount++ << std::endl;
            std::memcpy(_pboBufferData, someData, 4096 * 4096 * sizeof(unsigned char));
            _initializePBO = false;
            _pboIsDirty = true;
        } else {
            LDEBUG("Nothing to UPDATE! ");
        }
    }

    // if (!_useBuffering) {
    //     const std::string filename = getDecodeDataFromOsTime(OsEng.timeManager().time().j2000Seconds()).im->filename;
    //     decode(_pboBufferData, filename);
    //     _pboIsDirty = true;
    // } else {
    //     if (_videoDecoder && _videoDecoder->hasMoreFrames() && OsEng.timeManager().time().deltaTime() > 0) {
    //         //std::cerr << "Popping frame " << std::endl;
    //         const uint8_t* data = _videoDecoder->popFrame();
    //         if (data) {
    //             // for (int i = 0 ; i < 4096 * 4096; ++i) {
    //             //     std::cout << static_cast<int>(data[i]) << " ";
    //             // }
    //             auto t1 = Clock::now();
    //             std::copy(data, data + 4096 * 4096, _pboBufferData);
    //             auto t2 = Clock::now();


    //             LDEBUG("Copy time "
    //                    << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
    //                             .count()
    //                    << " ms" << std::endl);


    //             const double& osTime = OsEng.timeManager().time().j2000Seconds();
    //             auto& stateSequence = _imageMetadataMap2[_currentActiveInstrument];
    //             auto& state = stateSequence.getState(osTime);
    //             const double& timeObserved = state.timeObserved();
    //             std::shared_ptr<ImageMetadata> im = state.contents();

    //             _currentScale = im->isCoronaGraph;
    //             _currentScale = im->scale;
    //             _isCoronaGraph = im->isCoronaGraph;

    //             _initializePBO = false;
    //             _pboIsDirty = true;
    //         }

    //     } else {
    //         std::shared_ptr<SolarImageData> _solarImageData = _streamBuffer.popFinishedJob();
    //         if (_solarImageData) {

    //             _currentScale = _solarImageData->im->scale;
    //             _currentCenterPixel = _solarImageData->im->centerPixel;
    //             _isCoronaGraph = _solarImageData->im->isCoronaGraph;

    //             auto t1 = Clock::now();
    //             std::memcpy(_pboBufferData, _solarImageData->data, _imageSize * _imageSize * sizeof(unsigned char));
    //             auto t2 = Clock::now();

    //             if (_displayTimers) {
    //                 LDEBUG("Memcpy time "
    //                        << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
    //                                 .count()
    //                        << " ms" << std::endl);
    //             }

    //             const double& osTime = OsEng.timeManager().time().j2000Seconds();
    //             DecodeData decodeData = getDecodeDataFromOsTime(osTime + (_bufferSize + 1) * (OsEng.timeManager().time().deltaTime() * (static_cast<double>(_minRealTimeUpdateInterval )/1000.0)));
    //             auto job = std::make_shared<DecodeJob>(decodeData, decodeData.im->filename + std::to_string(decodeData.im->fullResolution));
    //             _streamBuffer.enqueueJob(job);
    //             _initializePBO = false;
    //             _pboIsDirty = true;

    //             if (_verboseMode)  {
    //                 LDEBUG("Popped image" << _solarImageData->im->filename);
    //                 LDEBUG("Adding work from PBO " << decodeData.im->filename);
    //             }
    //         } else {
    //             if (_verboseMode) {
    //                 LWARNING("Nothing to update, buffer is not ready");
    //             }
    //         }
    //     }
    // }
    _pbo->releaseMappedBuffer();
    _pbo->deactivate();
}

void RenderableSolarVideo::updateTextureGPU(bool asyncUpload, bool resChanged) {
   // if (_usePBO && asyncUpload) {
    _pbo->activate();
    _texture->bind();
    // Send async to GPU by coping from PBO to texture objects
    glTexSubImage2D(_texture->type(), 0, 0, 0, 4096, 4096,
                    GLint(_texture->format()), _texture->dataType(), nullptr);
    _pbo->deactivate();
    _pboIsDirty = false;
   // }
}

void RenderableSolarVideo::performImageTimestep(const double& osTime) {
    if (_pboIsDirty) {
        updateTextureGPU();
    }

    //bool stateChanged = _imageMetadataMap2[_currentActiveInstrument].hasStateChanged(osTime);
    double openspaceTime = OsEng.timeManager().time().j2000Seconds();
    double openspaceDiff = abs(openspaceTime - _openspaceTimeLast);
    // Check open space time since last time
    bool updateFrame = openspaceDiff > _videoMetadataMap[_currentActiveInstrument].frameUpdateInterval;
    //bool stateChanged = _imageMetadataMap[_currentActiveInstrument].hasStateChanged(osTime);
    // Time to pop video frame
    if (updateFrame || _initializePBO) {
        if (_usePBO /*&& !_initializePBO*/) {
            uploadImageDataToPBO();
        }
        _openspaceTimeLast = openspaceTime;
    }
}

bool RenderableSolarVideo::checkBoundaries(const RenderData& data) {
    const glm::dvec3& normal = _spacecraftCameraPlane->normal();
    const glm::dvec3& cameraPosition = data.camera.positionVec3();
    const glm::dvec3& planePosition = _spacecraftCameraPlane->worldPosition();

    const glm::dvec3 toCamera = glm::normalize(cameraPosition - planePosition);
    if (glm::dot(toCamera, normal) < 0) {
        return false;
    }
    return true;
}

void RenderableSolarVideo::update(const UpdateData& data) {
    _realTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    _realTimeDiff = _realTime.count() - _lastUpdateRealTime.count();

    // if (_useBuffering) {
        const double& dt = OsEng.timeManager().time().deltaTime();
        // Delta time changed, need to refill buffer
        if ((abs(_deltaTimeLast - dt)) > EPSILON) {
            _pboIsDirty = false;
            if (dt < 0) {
                _videoPlayer->stop();
                _videoIsStopped = true;
            } else if (_videoIsStopped) {
                _videoPlayer->start(OsEng.timeManager().time().j2000Seconds());
            }

        }
        _deltaTimeLast = dt;
    // }

    _timeToUpdateTexture = _realTimeDiff > _minRealTimeUpdateInterval;

    // Update lookup table, TODO: No need to do this every update
    _lut = _tfMap[_currentActiveInstrument].get();
    _spacecraftCameraPlane->update();
}

void RenderableSolarVideo::render(const RenderData& data) {
    if (!isReady()) {
        return;
    }

    const bool isWithinFrustum = checkBoundaries(data);
    // if (_isWithinFrustumLast != isWithinFrustum) {
    //     //_pboIsDirty = false;
    //     fillBuffer(OsEng.timeManager().time().j2000Seconds());
    // }
    // _isWithinFrustumLast = isWithinFrustum;

    // Update texture
    // The bool blockers might probably not be needed now
    if (_timeToUpdateTexture && !_updatingCurrentActiveChannel && (isWithinFrustum || _initializePBO)) {
        performImageTimestep(OsEng.timeManager().time().j2000Seconds());
        _lastUpdateRealTime = _realTime;
    }

    // TODO: We want to create sun imagery node from within the module
    const glm::dvec3& sunPositionWorld
          = OsEng.renderEngine().scene()->sceneGraphNode("Sun")->worldPosition();

    _spacecraftCameraPlane->render(data, *_texture, _lut, sunPositionWorld, _planeOpacity,
                                   _contrastValue, _gammaValue, _disableBorder,
                                   _disableFrustum, _currentCenterPixel, _currentScale);
}

} // namespace openspace
