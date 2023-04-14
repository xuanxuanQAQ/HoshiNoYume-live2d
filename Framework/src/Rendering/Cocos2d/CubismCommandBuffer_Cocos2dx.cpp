﻿/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismCommandBuffer_Cocos2dx.hpp"
#include "CubismFramework.hpp"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {
CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand::DrawCommand()
{
    _command.init(0.0);
}

CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand::~DrawCommand()
{
    if (_command.getPipelineDescriptor().programState != NULL)
    {
        _command.getPipelineDescriptor().programState->release();
    }
}

cocos2d::backend::BlendDescriptor* CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand::GetBlendDescriptor()
{
    return &_command.getPipelineDescriptor().blendDescriptor;
}

cocos2d::PipelineDescriptor* CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand::GetPipelineDescriptor()
{
    return &_command.getPipelineDescriptor();
}

cocos2d::CustomCommand* CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand::GetCommand()
{
    return &_command;
}

CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommandBuffer()
    : _vbStride(0)
    , _vbCount(0)
    , _ibCount(0)
{
}

CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::~DrawCommandBuffer()
{
    if (_drawBuffer != NULL)
    {
        CSM_FREE(_drawBuffer);
    }
}

void CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::CreateVertexBuffer(csmSizeInt stride, csmSizeInt count)
{
    _vbStride = stride;
    _vbCount = count;
    _drawBuffer = static_cast<csmUint8*>(CSM_MALLOC(stride * count));
    _drawCommandDraw.GetCommand()->createVertexBuffer(stride, count, cocos2d::CustomCommand::BufferUsage::DYNAMIC);
}

void CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::CreateIndexBuffer(csmSizeInt count)
{
    _ibCount = count;
    _drawCommandDraw.GetCommand()->createIndexBuffer(cocos2d::backend::IndexFormat::U_SHORT, count, cocos2d::CustomCommand::BufferUsage::DYNAMIC);
}

void CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::UpdateVertexBuffer(void* data, void* uvData, csmSizeInt count)
{
    csmSizeInt length = count * _vbStride;
    csmFloat32* dest = reinterpret_cast<csmFloat32*>(_drawBuffer);
    csmFloat32* sourceVertices = reinterpret_cast<csmFloat32*>(data);
    csmFloat32* sourceUvs = reinterpret_cast<csmFloat32*>(uvData);

    for (csmUint32 i = 0, j = 0; i < count; ++i)
    {
        *dest = *sourceVertices;
        dest++;
        sourceVertices++;
        *dest = *sourceVertices;
        dest++;
        sourceVertices++;

        *dest = *sourceUvs;
        dest++;
        sourceUvs++;
        *dest = *sourceUvs;
        dest++;
        sourceUvs++;
    }
}

void CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::UpdateIndexBuffer(void* data, csmSizeInt count)
{
    csmSizeInt length = count * sizeof(csmInt16);

    _drawCommandDraw.GetCommand()->updateIndexBuffer(data, length);
}

void CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::CommitVertexBuffer() {

    csmSizeInt count = _vbCount * _vbStride;

    _drawCommandDraw.GetCommand()->updateVertexBuffer(_drawBuffer, count);
}

CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::DrawCommand* CubismCommandBuffer_Cocos2dx::DrawCommandBuffer::GetCommandDraw()
{
    return &_drawCommandDraw;
}

CubismCommandBuffer_Cocos2dx::CubismCommandBuffer_Cocos2dx()
    :_currentColorBuffer(NULL)
{
    _groupCommand.init(0.0);
}

CubismCommandBuffer_Cocos2dx::~CubismCommandBuffer_Cocos2dx()
{
}

void CubismCommandBuffer_Cocos2dx::PushCommandGroup()
{
    GetCocos2dRenderer()->addCommand(&_groupCommand);
    GetCocos2dRenderer()->pushGroup(_groupCommand.getRenderQueueID());
}

void CubismCommandBuffer_Cocos2dx::PopCommandGroup()
{
    GetCocos2dRenderer()->popGroup();
}

void CubismCommandBuffer_Cocos2dx::SetOperationEnable(OperationType operationType, csmBool enabled)
{
    _operationStateArray[operationType].Enabled = enabled;


    AddCommand
    (
        [=]() -> void
    {
        switch (operationType)
        {
        case OperationType_ScissorTest:
            GetCocos2dRenderer()->setScissorTest(enabled);
            break;
        case OperationType_DepthTest:
            GetCocos2dRenderer()->setDepthTest(enabled);
            break;
        case OperationType_StencilTest:
            GetCocos2dRenderer()->setStencilTest(enabled);
            break;
        case OperationType_Culling:
            if (!enabled)
            {
                GetCocos2dRenderer()->setCullMode(cocos2d::CullMode::NONE);
            }
            else
            {
                switch (_operationStateArray[operationType].Arg0.i32)
                {
                case CullType_Front:
                    GetCocos2dRenderer()->setCullMode(cocos2d::CullMode::FRONT);
                    break;
                case CullType_Back:
                    GetCocos2dRenderer()->setCullMode(cocos2d::CullMode::BACK);
                    break;
                }
            }
            break;

        }
    }
    );
}

csmBool CubismCommandBuffer_Cocos2dx::GetOperationEnabled(OperationType operationType)
{
    return _operationStateArray[operationType].Enabled;
}

void CubismCommandBuffer_Cocos2dx::SetCullMode(CullType cullType)
{
    _operationStateArray[OperationType_Culling].Arg0.i32 = cullType;


    SetOperationEnable(OperationType_Culling, _operationStateArray[OperationType_Culling].Enabled);
}

void CubismCommandBuffer_Cocos2dx::SetWindingMode(WindingType windingType)
{
    _operationStateArray[OperationType_Winding].Arg0.i32 = windingType;


    AddCommand
    (
        [=] () -> void
        {
            switch (_operationStateArray[OperationType_Winding].Arg0.i32)
            {
            case WindingType_ClockWise:
                GetCocos2dRenderer()->setWinding(cocos2d::Winding::CLOCK_WISE);
                break;

            case  WindingType_CounterClockWise:
                GetCocos2dRenderer()->setWinding(cocos2d::Winding::COUNTER_CLOCK_WISE);
                break;
            }
        }
    );
}

void CubismCommandBuffer_Cocos2dx::Clear(csmFloat32 r, csmFloat32 g, csmFloat32 b, csmFloat32 a)
{
    // Add the callback command internally.
    GetCocos2dRenderer()->clear(cocos2d::ClearFlag::COLOR, cocos2d::Color4F(r, g, b, a), 0.0f, 0, 0.0f);
}

void CubismCommandBuffer_Cocos2dx::Viewport(csmFloat32 x, csmFloat32 y, csmFloat32 w, csmFloat32 h)
{
    AddCommand
    (
        [=] () -> void
        {
            GetCocos2dRenderer()->setViewPort(x, y, w, h);
        }
    );
}

void CubismCommandBuffer_Cocos2dx::SetColorBuffer(cocos2d::Texture2D* colorBuffer)
{
    _currentColorBuffer = colorBuffer;

    AddCommand
    (
        [=] () -> void
        {
            GetCocos2dRenderer()->setRenderTarget(cocos2d::RenderTargetFlag::COLOR, colorBuffer, NULL, NULL);
        }
    );
}

cocos2d::Texture2D* CubismCommandBuffer_Cocos2dx::GetColorBuffer()
{
    return _currentColorBuffer;
}

void CubismCommandBuffer_Cocos2dx::AddDrawCommand(DrawCommandBuffer::DrawCommand* drawCommand)
{
    GetCocos2dRenderer()->addCommand(drawCommand->GetCommand());
}

void CubismCommandBuffer_Cocos2dx::AddCommand(const std::function<void()>& fn)
{
    cocos2d::CallbackCommand* command = CSM_NEW cocos2d::CallbackCommand();

    command->init(0.0f);
    command->func = [=] () -> void
    {
        fn();
        delete command;
    };

    GetCocos2dRenderer()->addCommand(command);
}

}}}}
