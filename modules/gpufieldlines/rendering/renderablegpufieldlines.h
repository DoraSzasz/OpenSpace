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

#ifndef __OPENSPACE_MODULE_GPUFIELDLINES___RENDERABLEGPUFIELDLINES___H__
#define __OPENSPACE_MODULE_GPUFIELDLINES___RENDERABLEGPUFIELDLINES___H__

#include <openspace/properties/scalarproperty.h>
#include <openspace/properties/vector/vec2property.h>
#include <openspace/properties/vector/vec3property.h>
#include <openspace/properties/vector/vec4property.h>

#include <openspace/rendering/renderable.h>

#include <modules/gpufieldlines/util/gpufieldlinesstate.h>

#include <modules/volume/rawvolume.h>


namespace ghoul {
    namespace opengl {
        class Texture;
        class TextureUnit;
    }
}

namespace openspace {

class RenderableGpuFieldlines : public Renderable {
public:
    RenderableGpuFieldlines(const ghoul::Dictionary& dictionary);
    // ~RenderableGpuFieldlines();

    bool initialize() override;
    bool deinitialize() override;

    bool isReady() const override;

    void render(const RenderData& data) override;
    void update(const UpdateData& data) override;

    bool isWithinInterval();
    void updateActiveStateIndex();
private:
    // PROPERTIES
    properties::BoolProperty _showGrid;
    properties::BoolProperty _isMorphing;
    properties::BoolProperty _showSeedPoints;

    properties::FloatProperty _stepSize;
    properties::FloatProperty _seedPointSize;
    properties::FloatProperty _clippingRadius;

    properties::IntProperty _maximumVertices;
    properties::IntProperty _stepMultiplier;

    properties::OptionProperty _integrationMethod;

    properties::Vec2Property _domainX;
    properties::Vec2Property _domainY;
    properties::Vec2Property _domainZ;

    properties::Vec4Property _uniformFieldlineColor;
    properties::Vec4Property _uniformSeedPointColor;

    // LUA INFO
    ghoul::Dictionary _vectorVolumeInfo;
    ghoul::Dictionary _fieldlineInfo;
    ghoul::Dictionary _seedPointsInfo;

    std::vector<glm::vec3> _seedPoints;
    std::vector<GpuFieldlinesState> _states;
    std::unique_ptr<ghoul::opengl::ProgramObject> _program;
    std::unique_ptr<ghoul::opengl::ProgramObject> _gridProgram;
    std::unique_ptr<ghoul::opengl::ProgramObject> _seedPointProgram;

    bool _shouldRender; // only temporary
    bool _needsUpdate;
    bool _isSpherical;

    GLuint _vertexArrayObject;
    GLuint _vertexPositionBuffer;
    GLuint _vertexColorBuffer;

    GLuint _gridVAO;
    GLuint _gridVBO;

    std::unique_ptr<RawVolume<float>> _rawVolume;
    std::unique_ptr<RawVolume<glm::vec3>> _normalizedVolume;

    std::vector<std::shared_ptr<ghoul::opengl::Texture>> _volumeTexture;

    std::unique_ptr<ghoul::opengl::TextureUnit> _textureUnit;
    std::unique_ptr<ghoul::opengl::TextureUnit> _textureUnit2;

    glm::vec3 _domainMins;
    glm::vec3 _domainMaxs;
    glm::uvec3 _dimensions;

    std::vector<glm::vec3> _gridVertices;
    std::vector<GLint> _gridStartPos;
    std::vector<GLsizei> _gridLineCount;

    int _activeStateIndex;
    int _numberOfStates;
    double _seqStartTime; // redundant, but hey.. nice n clear
    double _seqEndTime;
    double _currentTime;
    float _stateProgress;
    std::string _modelName;

    std::vector<double> _startTimes;
    void generateUniformSphericalGrid();
    void generateUniformCartesian3DGrid();
};

} // namespace openspace

#endif // __OPENSPACE_MODULE_GPUFIELDLINES___RENDERABLEGPUFIELDLINES___H__
