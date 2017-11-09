//
//  GLBackend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLBackend_h
#define hifi_gpu_gl_GLBackend_h

#include <assert.h>
#include <functional>
#include <memory>
#include <bitset>
#include <queue>
#include <utility>
#include <list>
#include <array>

#include <QtCore/QLoggingCategory>

// #include <gl/Config.h>

#include <gpu/Forward.h>
#include <gpu/Context.h>
#include "d3d12shared.h"


// Different versions for the stereo drawcall
// Current preferred is  "instanced" which draw the shape twice but instanced and rely on clipping plane to draw left/right side only
//#define GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
//#define GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_TECHNIQUE_INSTANCED


// Let these be configured by the one define picked above
#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SIMPLE
#define GPU_STEREO_DRAWCALL_DOUBLED
#endif

#ifdef GPU_STEREO_TECHNIQUE_DOUBLED_SMARTER
#define GPU_STEREO_DRAWCALL_DOUBLED
#define GPU_STEREO_CAMERA_BUFFER
#endif

#ifdef GPU_STEREO_TECHNIQUE_INSTANCED
#define GPU_STEREO_DRAWCALL_INSTANCED
#define GPU_STEREO_CAMERA_BUFFER
#endif

namespace gpu { namespace d3d12 {

class D3D12Backend : public Backend, public std::enable_shared_from_this<D3D12Backend> {
    // Context Backend static interface required
    friend class gpu::Context;
    static void init();
    static BackendPointer createBackend();

public:
    explicit D3D12Backend(bool syncCache);
    D3D12Backend();

    // TODO: Implement this properly.
    const std::string DX12_VERSION = "DX12";
    const std::string& getVersion() const override { return DX12_VERSION; }


public:
    static bool makeProgram(Shader& shader, const Shader::BindingSet& slotBindings = Shader::BindingSet());

    virtual ~D3D12Backend();

    void setCameraCorrection(const Mat4& correction);
    void render(const Batch& batch) final override;

    // This call synchronize the Full Backend cache with the current GLState
    // THis is only intended to be used when mixing raw gl calls with the gpu api usage in order to sync
    // the gpu::Backend state with the true gl state which has probably been messed up by these ugly naked gl calls
    // Let's try to avoid to do that as much as possible!
    void syncCache() final override;

    // This is the ugly "download the pixels to sysmem for taking a snapshot"
    // Just avoid using it, it's ugly and will break performances
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer,
                                     const Vec4i& region, QImage& destImage) final override;


    // this is the maximum numeber of available input buffers
    size_t getNumInputBuffers() const { return _input._invalidBuffers.size(); }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_UNIFORM_BUFFERS = 12;
    size_t getMaxNumUniformBuffers() const { return MAX_NUM_UNIFORM_BUFFERS; }

    // this is the maximum per shader stage on the low end apple
    // TODO make it platform dependant at init time
    static const int MAX_NUM_RESOURCE_BUFFERS = 16;
    size_t getMaxNumResourceBuffers() const { return MAX_NUM_RESOURCE_BUFFERS; }
    static const int MAX_NUM_RESOURCE_TEXTURES = 16;
    size_t getMaxNumResourceTextures() const { return MAX_NUM_RESOURCE_TEXTURES; }

    // Draw Stage
    virtual void do_draw(const Batch& batch, size_t paramOffset);
    virtual void do_drawIndexed(const Batch& batch, size_t paramOffset);
    virtual void do_drawInstanced(const Batch& batch, size_t paramOffset);
    virtual void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset);
    virtual void do_multiDrawIndirect(const Batch& batch, size_t paramOffset);
    virtual void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset);

    // Input Stage
    virtual void do_setInputFormat(const Batch& batch, size_t paramOffset) final;
    virtual void do_setInputBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndexBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setIndirectBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_generateTextureMips(const Batch& batch, size_t paramOffset) final;

    // Transform Stage
    virtual void do_setModelTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setProjectionTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setViewportTransform(const Batch& batch, size_t paramOffset) final;
    virtual void do_setDepthRangeTransform(const Batch& batch, size_t paramOffset) final;

    // Uniform Stage
    virtual void do_setUniformBuffer(const Batch& batch, size_t paramOffset) final;

    // Resource Stage
    virtual void do_setResourceBuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_setResourceTexture(const Batch& batch, size_t paramOffset) final;

    // Pipeline Stage
    virtual void do_setPipeline(const Batch& batch, size_t paramOffset) final;

    // Output stage 2
    virtual void do_setFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_clearFramebuffer(const Batch& batch, size_t paramOffset) final;
    virtual void do_blit(const Batch& batch, size_t paramOffset);

    // Query section
    virtual void do_beginQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_endQuery(const Batch& batch, size_t paramOffset) final;
    virtual void do_getQuery(const Batch& batch, size_t paramOffset) final;

    // Reset stages
    virtual void do_resetStages(const Batch& batch, size_t paramOffset) final;

    
    virtual void do_disableContextViewCorrection(const Batch& batch, size_t paramOffset) final;
    virtual void do_restoreContextViewCorrection(const Batch& batch, size_t paramOffset) final;

    virtual void do_disableContextStereo(const Batch& batch, size_t paramOffset) final;
    virtual void do_restoreContextStereo(const Batch& batch, size_t paramOffset) final;

    virtual void do_runLambda(const Batch& batch, size_t paramOffset) final;

    virtual void do_startNamedCall(const Batch& batch, size_t paramOffset) final;
    virtual void do_stopNamedCall(const Batch& batch, size_t paramOffset) final;

    static const int MAX_NUM_ATTRIBUTES = Stream::NUM_INPUT_SLOTS;
    // The drawcall Info attribute  channel is reserved and is the upper bound for the number of available Input buffers
    static const int MAX_NUM_INPUT_BUFFERS = Stream::DRAW_CALL_INFO;

    virtual void do_pushProfileRange(const Batch& batch, size_t paramOffset) final;
    virtual void do_popProfileRange(const Batch& batch, size_t paramOffset) final;

    // TODO: As long as we have gl calls explicitly issued from interface
    // code, we need to be able to record and batch these calls. THe long 
    // term strategy is to get rid of any GL calls in favor of the HIFI GPU API
    virtual void do_glUniform1i(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform1f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform2f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4f(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniform4iv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix3fv(const Batch& batch, size_t paramOffset) final;
    virtual void do_glUniformMatrix4fv(const Batch& batch, size_t paramOffset) final;

    virtual void do_glColor4f(const Batch& batch, size_t paramOffset) final;

    // The State setters called by the GLState::Commands when a new state is assigned
    virtual void do_setStateFillMode(int32 mode) final;
    virtual void do_setStateCullMode(int32 mode) final;
    virtual void do_setStateFrontFaceClockwise(bool isClockwise) final;
    virtual void do_setStateDepthClampEnable(bool enable) final;
    virtual void do_setStateScissorEnable(bool enable) final;
    virtual void do_setStateMultisampleEnable(bool enable) final;
    virtual void do_setStateAntialiasedLineEnable(bool enable) final;
    virtual void do_setStateDepthBias(Vec2 bias) final;
    virtual void do_setStateDepthTest(State::DepthTest test) final;
    virtual void do_setStateStencil(State::StencilActivation activation, State::StencilTest frontTest, State::StencilTest backTest) final;
    virtual void do_setStateAlphaToCoverageEnable(bool enable) final;
    virtual void do_setStateSampleMask(uint32 mask) final;
    virtual void do_setStateBlend(State::BlendFunction blendFunction) final;
    virtual void do_setStateColorWriteMask(uint32 mask) final;
    virtual void do_setStateBlendFactor(const Batch& batch, size_t paramOffset) final;
    virtual void do_setStateScissorRect(const Batch& batch, size_t paramOffset) final;

    // TODO: Incompatible with D3D12. Either do an ID to handle mapping, or change the interface.
    virtual unsigned int getFramebufferID(const FramebufferPointer& framebuffer)
    {
        return 0;
    }

    virtual unsigned int getTextureID(const TexturePointer& texture) final
    {
        return 0;
    }

    virtual unsigned int getBufferID(const Buffer& buffer)
    {
        return 0;
    }

    virtual unsigned int getQueryID(const QueryPointer& query)
    {
        return 0;
    }

    virtual d3d12Framebuffer* syncGPUObject(const Framebuffer& framebuffer)
    {
        return nullptr;
    }

    virtual d3d12Buffer* syncGPUObject(const Buffer& buffer)
    {
        return nullptr;
    }

    virtual D3D12Texture* syncGPUObject(const TexturePointer& texture)
    {
        return nullptr;
    }

    virtual GLQuery* syncGPUObject(const Query& query)
    {
        return nullptr;
    }

    //virtual bool isTextureReady(const TexturePointer& texture);

    // TODO: use d3d handles, or do int mapping.
    virtual void releaseBuffer(unsigned int id, Size size) const;
    virtual void releaseExternalTexture(unsigned int id, const Texture::ExternalRecycler& recycler) const;
    virtual void releaseTexture(unsigned int id, Size size) const;
    virtual void releaseFramebuffer(unsigned int id) const;
    virtual void releaseShader(unsigned int id) const;
    virtual void releaseProgram(unsigned int id) const;
    virtual void releaseQuery(unsigned int id) const;
    virtual void queueLambda(const std::function<void()> lambda) const;

    bool isTextureManagementSparseEnabled() const override { return (_textureManagement._sparseCapable && Texture::getEnableSparseTextures()); }

protected:

    void recycle() const override;

    static const size_t INVALID_OFFSET = (size_t)-1;
    bool _inRenderTransferPass { false };
    int32_t _uboAlignment { 0 };
    int _currentDraw { -1 };

    std::list<std::string> profileRanges;
    mutable Mutex _trashMutex;
    mutable std::list<std::pair<unsigned int, Size>> _buffersTrash;
    mutable std::list<std::pair<unsigned int, Size>> _texturesTrash;
    mutable std::list<std::pair<unsigned int, Texture::ExternalRecycler>> _externalTexturesTrash;
    mutable std::list<unsigned int> _framebuffersTrash;
    mutable std::list<unsigned int> _shadersTrash;
    mutable std::list<unsigned int> _programsTrash;
    mutable std::list<unsigned int> _queriesTrash;
    mutable std::list<std::function<void()>> _lambdaQueue;

    void renderPassTransfer(const Batch& batch);
    void renderPassDraw(const Batch& batch);

#ifdef GPU_STEREO_DRAWCALL_DOUBLED
    void setupStereoSide(int side);
#endif

    virtual void initInput() final;
    virtual void killInput() final;
    virtual void syncInputStateCache() final;
    virtual void resetInputStage();
    
    virtual void updateInput()
    {
    }

    struct InputStageState {
        bool _invalidFormat { true };
        Stream::FormatPointer _format;
        std::string _formatKey;

        typedef std::bitset<MAX_NUM_ATTRIBUTES> ActivationCache;
        ActivationCache _attributeActivation { 0 };

        typedef std::bitset<MAX_NUM_INPUT_BUFFERS> BuffersState;

        BuffersState _invalidBuffers{ 0 };
        BuffersState _attribBindingBuffers{ 0 };

        Buffers _buffers;
        Offsets _bufferOffsets;
        Offsets _bufferStrides;
        std::vector<unsigned int> _bufferVBOs;

        glm::vec4 _colorAttribute{ 0.0f };

        BufferPointer _indexBuffer;
        Offset _indexBufferOffset { 0 };
        Type _indexBufferType { UINT32 };
        
        BufferPointer _indirectBuffer;
        Offset _indirectBufferOffset{ 0 };
        Offset _indirectBufferStride{ 0 };

        unsigned int _defaultVAO { 0 };

        InputStageState() :
            _invalidFormat(true),
            _format(0),
            _formatKey(),
            _attributeActivation(0),
            _buffers(_invalidBuffers.size(), BufferPointer(0)),
            _bufferOffsets(_invalidBuffers.size(), 0),
            _bufferStrides(_invalidBuffers.size(), 0),
            _bufferVBOs(_invalidBuffers.size(), 0) {}
    } _input;

    virtual void initTransform()
    {
    }

    void killTransform();
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncTransformStateCache();
   
    virtual void updateTransform(const Batch& batch)
    {
    }

    virtual void resetTransformStage();

    // Allows for correction of the camera pose to account for changes
    // between the time when a was recorded and the time(s) when it is 
    // executed
    struct CameraCorrection {
        Mat4 correction;
        Mat4 correctionInverse;
    };

    struct TransformStageState {
#ifdef GPU_STEREO_CAMERA_BUFFER
        struct Cameras {
            TransformCamera _cams[2];

            Cameras() {};
            Cameras(const TransformCamera& cam) { memcpy(_cams, &cam, sizeof(TransformCamera)); };
            Cameras(const TransformCamera& camL, const TransformCamera& camR) { memcpy(_cams, &camL, sizeof(TransformCamera)); memcpy(_cams + 1, &camR, sizeof(TransformCamera)); };
        };

        using CameraBufferElement = Cameras;
#else
        using CameraBufferElement = TransformCamera;
#endif
        using TransformCameras = std::vector<CameraBufferElement>;

        TransformCamera _camera;
        TransformCameras _cameras;

        mutable std::map<std::string, void*> _drawCallInfoOffsets;

        unsigned int _objectBuffer { 0 };
        unsigned int _cameraBuffer { 0 };
        unsigned int _drawCallInfoBuffer { 0 };
        unsigned int _objectBufferTexture { 0 };
        size_t _cameraUboSize { 0 };
        bool _viewIsCamera{ false };
        bool _skybox { false };
        Transform _view;
        CameraCorrection _correction;
        bool _viewCorrectionEnabled{ true };


        Mat4 _projection;
        Vec4i _viewport { 0, 0, 1, 1 };
        Vec2 _depthRange { 0.0f, 1.0f };
        bool _invalidView { false };
        bool _invalidProj { false };
        bool _invalidViewport { false };

        bool _enabledDrawcallInfoBuffer{ false };

        using Pair = std::pair<size_t, size_t>;
        using List = std::list<Pair>;
        List _cameraOffsets;
        mutable List::const_iterator _camerasItr;
        mutable size_t _currentCameraOffset{ INVALID_OFFSET };

        void preUpdate(size_t commandIndex, const StereoState& stereo);
        void update(size_t commandIndex, const StereoState& stereo) const;
        void bindCurrentCamera(int stereoSide) const;
    } _transform;

    virtual void transferTransformState(const Batch& batch) const
    {
    }

    struct UniformStageState {
        std::array<BufferPointer, MAX_NUM_UNIFORM_BUFFERS> _buffers;
        //Buffers _buffers {  };
    } _uniform;

    void releaseUniformBuffer(uint32_t slot);
    void resetUniformStage();

    // update resource cache and do the gl bind/unbind call with the current gpu::Buffer cached at slot s
    // This is using different gl object  depending on the gl version
    virtual bool bindResourceBuffer(uint32_t slot, BufferPointer& buffer)
    {
        return false;
    }

    virtual void releaseResourceBuffer(uint32_t slot)
    {
    }

    // update resource cache and do the gl unbind call with the current gpu::Texture cached at slot s
    void releaseResourceTexture(uint32_t slot);

    void resetResourceStage();

    struct ResourceStageState {
        std::array<BufferPointer, MAX_NUM_RESOURCE_BUFFERS> _buffers;
        std::array<TexturePointer, MAX_NUM_RESOURCE_TEXTURES> _textures;
        //Textures _textures { { MAX_NUM_RESOURCE_TEXTURES } };
        int findEmptyTextureSlot() const;
    } _resource;

    size_t _commandIndex{ 0 };

    // Standard update pipeline check that the current Program and current State or good to go for a
    void updatePipeline();
    // Force to reset all the state fields indicated by the 'toBeReset" signature
    void resetPipelineState(State::Signature toBeReset);
    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncPipelineStateCache();
    void resetPipelineStage();

    struct PipelineStageState {
        PipelinePointer _pipeline;

        unsigned int _program { 0 };
        int _cameraCorrectionLocation { -1 };
        D3DShader* _programShader { nullptr };
        bool _invalidProgram { false };

        BufferView _cameraCorrectionBuffer { gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr )) };
        BufferView _cameraCorrectionBufferIdentity { gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(CameraCorrection), nullptr )) };

        State::Data _stateCache{ State::DEFAULT };
        State::Signature _stateSignatureCache { 0 };

        GLState* _state { nullptr };
        bool _invalidState { false };

        PipelineStageState() {
            _cameraCorrectionBuffer.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBufferIdentity.edit<CameraCorrection>() = CameraCorrection();
            _cameraCorrectionBufferIdentity._buffer->flush();
        }
    } _pipeline;

    // Backend dependant compilation of the shader
    virtual D3DShader* compileBackendProgram(const Shader& program);
    virtual D3DShader* compileBackendShader(const Shader& shader);
    virtual std::string getBackendShaderHeader() const;
    virtual void makeProgramBindings(ShaderObject& shaderObject);
    class ElementResource {
    public:
        gpu::Element _element;
        uint16 _resource;
        ElementResource(Element&& elem, uint16 resource) : _element(elem), _resource(resource) {}
    };
    ElementResource getFormatFromGLUniform(DXGI_FORMAT type);
    static const int UNUSED_SLOT {-1};
    static bool isUnusedSlot(int binding) { return (binding == UNUSED_SLOT); }
    virtual int makeUniformSlots(unsigned int glprogram, const Shader::BindingSet& slotBindings,
        Shader::SlotSet& uniforms, Shader::SlotSet& textures, Shader::SlotSet& samplers);
    virtual int makeUniformBlockSlots(unsigned int glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& buffers);
    
    virtual int makeResourceBufferSlots(unsigned int glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& resourceBuffers)
    {
        return 0;
    }
    
    virtual int makeInputSlots(unsigned int glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& inputs);
    virtual int makeOutputSlots(unsigned int glprogram, const Shader::BindingSet& slotBindings, Shader::SlotSet& outputs);


    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void syncOutputStateCache();
    void resetOutputStage();
    
    struct OutputStageState {
        FramebufferPointer _framebuffer { nullptr };
        unsigned int _drawFBO { 0 };
    } _output;

    void resetQueryStage();
    struct QueryStageState {
        uint32_t _rangeQueryDepth { 0 };
    } _queryStage;

    void resetStages();

    struct TextureManagementStageState {
        bool _sparseCapable { false };
    } _textureManagement;
    virtual void initTextureManagementStage() {}

    typedef void (D3D12Backend::*CommandCall)(const Batch&, size_t);
    static CommandCall _commandCalls[Batch::NUM_COMMANDS];
    friend class GLState;
    friend class D3D12Texture;
    friend class D3DShader;
};

} }

#endif
