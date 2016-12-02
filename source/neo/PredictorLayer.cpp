// ----------------------------------------------------------------------------
//  OgmaNeo
//  Copyright(c) 2016 Ogma Intelligent Systems Corp. All rights reserved.
//
//  This copy of OgmaNeo is licensed to you under the terms described
//  in the OGMANEO_LICENSE.md file included in this distribution.
// ----------------------------------------------------------------------------

#include "PredictorLayer.h"
#include "SparseFeaturesChunk.h"
#include "SparseFeaturesDelay.h"
#include "SparseFeaturesSTDP.h"

#include "PlotDebug.h"

using namespace ogmaneo;

void PredictorLayer::createRandom(ComputeSystem &cs, ComputeProgram &plProgram,
    cl_int2 hiddenSize, const std::vector<VisibleLayerDesc> &visibleLayerDescs,
    const std::shared_ptr<SparseFeatures> &inhibitSparseFeatures,
    cl_float2 initWeightRange, std::mt19937 &rng)
{
    cl_float4 zeroColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    _visibleLayerDescs = visibleLayerDescs;

    _hiddenSize = hiddenSize;

    _inhibitSparseFeatures = inhibitSparseFeatures;

    cl::array<cl::size_type, 3> zeroOrigin = { 0, 0, 0 };
    cl::array<cl::size_type, 3> hiddenRegion = { static_cast<cl_uint>(_hiddenSize.x), static_cast<cl_uint>(_hiddenSize.y), 1 };

    _visibleLayers.resize(_visibleLayerDescs.size());

    cl::Kernel randomUniform2DKernel = cl::Kernel(plProgram.getProgram(), "randomUniform2D");
    cl::Kernel randomUniform3DKernel = cl::Kernel(plProgram.getProgram(), "randomUniform3D");

    // Create layers
    for (int vli = 0; vli < _visibleLayers.size(); vli++) {
        VisibleLayer &vl = _visibleLayers[vli];
        VisibleLayerDesc &vld = _visibleLayerDescs[vli];

        vl._hiddenToVisible = cl_float2{ static_cast<float>(vld._size.x) / static_cast<float>(_hiddenSize.x),
            static_cast<float>(vld._size.y) / static_cast<float>(_hiddenSize.y)
        };

        vl._visibleToHidden = cl_float2{ static_cast<float>(_hiddenSize.x) / static_cast<float>(vld._size.x),
            static_cast<float>(_hiddenSize.y) / static_cast<float>(vld._size.y)
        };

        vl._reverseRadii = cl_int2{ static_cast<cl_int>(std::ceil(vl._visibleToHidden.x * vld._radius) + 1),
            static_cast<cl_int>(std::ceil(vl._visibleToHidden.y * vld._radius) + 1)
        };

        {
            int weightDiam = vld._radius * 2 + 1;

            int numWeights = weightDiam * weightDiam;

            cl_int3 weightsSize = { _hiddenSize.x, _hiddenSize.y, numWeights };

            vl._weights = createDoubleBuffer3D(cs, weightsSize, CL_R, CL_FLOAT);

            randomUniform(vl._weights[_back], cs, randomUniform3DKernel, weightsSize, initWeightRange, rng);
        }

        vl._derivedInput = createDoubleBuffer2D(cs, vld._size, CL_R, CL_FLOAT);

        cs.getQueue().enqueueFillImage(vl._derivedInput[_back], zeroColor, zeroOrigin, { static_cast<cl::size_type>(vld._size.x), static_cast<cl::size_type>(vld._size.y), 1 });
    }

    // Hidden state data
    _hiddenStates = createDoubleBuffer2D(cs, _hiddenSize, CL_R, CL_FLOAT);

    _hiddenSummationTemp = createDoubleBuffer2D(cs, _hiddenSize, CL_R, CL_FLOAT);

    cs.getQueue().enqueueFillImage(_hiddenStates[_back], zeroColor, zeroOrigin, hiddenRegion);

    // Create kernels
    _deriveInputsKernel = cl::Kernel(plProgram.getProgram(), "plDeriveInputs");
    _stimulusKernel = cl::Kernel(plProgram.getProgram(), "plStimulus");
    _learnPredWeightsKernel = cl::Kernel(plProgram.getProgram(), "plLearnPredWeights");
}

void PredictorLayer::activate(ComputeSystem &cs, const std::vector<cl::Image2D> &visibleStates, std::mt19937 &rng) {
    cl::array<cl::size_type, 3> zeroOrigin = { 0, 0, 0 };
    cl::array<cl::size_type, 3> hiddenRegion = { static_cast<cl_uint>(_hiddenSize.x), static_cast<cl_uint>(_hiddenSize.y), 1 };

    // Start by clearing stimulus summation buffer to biases
    cs.getQueue().enqueueFillImage(_hiddenSummationTemp[_back], cl_float4{ 0.0f, 0.0f, 0.0f, 0.0f }, zeroOrigin, hiddenRegion);

	//std::cout << "INFO: PredictorLayer:activate: " << visibleStates.size() << ", " << _visibleLayers.size() << std::endl;

    // Find up stimulus
    for (int vli = 0; vli < _visibleLayers.size(); vli++) {
        VisibleLayer &vl = _visibleLayers[vli];
        VisibleLayerDesc &vld = _visibleLayerDescs[vli];

		plots::plotImage(cs, visibleStates[vli], 6.0f, "PredictorLayer:activate:visibleStates" + std::to_string(vli));
		//plots::plotImage(cs, vl._derivedInput[_back], 6.0f, "PredictorLayer:activate:derivedInput-back" + std::to_string(vli));

		// Derive inputs
        {
            int argIndex = 0;

            _deriveInputsKernel.setArg(argIndex++, visibleStates[vli]);
            _deriveInputsKernel.setArg(argIndex++, vl._derivedInput[_back]);
            _deriveInputsKernel.setArg(argIndex++, vl._derivedInput[_front]);
 
            cs.getQueue().enqueueNDRangeKernel(_deriveInputsKernel, cl::NullRange, cl::NDRange(vld._size.x, vld._size.y));
        }

		//plots::plotImage(cs, vl._derivedInput[_front], 6.0f, "PredictorLayer:activate:derivedInput-front" + std::to_string(vli));

        {
            int argIndex = 0;

            _stimulusKernel.setArg(argIndex++, vl._derivedInput[_front]);
            _stimulusKernel.setArg(argIndex++, _hiddenSummationTemp[_back]);
            _stimulusKernel.setArg(argIndex++, _hiddenSummationTemp[_front]);
            _stimulusKernel.setArg(argIndex++, vl._weights[_back]);
            _stimulusKernel.setArg(argIndex++, vld._size);
            _stimulusKernel.setArg(argIndex++, vl._hiddenToVisible);
            _stimulusKernel.setArg(argIndex++, vld._radius);

            cs.getQueue().enqueueNDRangeKernel(_stimulusKernel, cl::NullRange, cl::NDRange(_hiddenSize.x, _hiddenSize.y));
        }

        // Swap buffers
        std::swap(_hiddenSummationTemp[_front], _hiddenSummationTemp[_back]);
    }

    if (_inhibitSparseFeatures != nullptr)
        _inhibitSparseFeatures->inhibit(cs, _hiddenSummationTemp[_back], _hiddenStates[_front], rng);
    else {
        // Copy to hidden states
        cs.getQueue().enqueueCopyImage(_hiddenSummationTemp[_back], _hiddenStates[_front], zeroOrigin, zeroOrigin, hiddenRegion);
    }
}

void PredictorLayer::stepEnd(ComputeSystem &cs) {
    cl::array<cl::size_type, 3> zeroOrigin = { 0, 0, 0 };
    cl::array<cl::size_type, 3> hiddenRegion = { static_cast<cl_uint>(_hiddenSize.x), static_cast<cl_uint>(_hiddenSize.y), 1 };

    std::swap(_hiddenStates[_front], _hiddenStates[_back]);

    // Swap buffers
    for (int vli = 0; vli < _visibleLayers.size(); vli++) {
        VisibleLayer &vl = _visibleLayers[vli];
        VisibleLayerDesc &vld = _visibleLayerDescs[vli];

        std::swap(vl._derivedInput[_front], vl._derivedInput[_back]);
    }
}

void PredictorLayer::learn(ComputeSystem &cs, const cl::Image2D &targets) {
    // Learn weights
    for (int vli = 0; vli < _visibleLayers.size(); vli++) {
        VisibleLayer &vl = _visibleLayers[vli];
        VisibleLayerDesc &vld = _visibleLayerDescs[vli];

		//plots::plotImage(cs, vl._derivedInput[_back], 3, "PredictorLayer:learn:derivedInputs" + std::to_string(vli));

        int argIndex = 0;

        _learnPredWeightsKernel.setArg(argIndex++, vl._derivedInput[_back]);
        _learnPredWeightsKernel.setArg(argIndex++, targets);
        _learnPredWeightsKernel.setArg(argIndex++, _hiddenStates[_back]);
        _learnPredWeightsKernel.setArg(argIndex++, vl._weights[_back]);
        _learnPredWeightsKernel.setArg(argIndex++, vl._weights[_front]);
        _learnPredWeightsKernel.setArg(argIndex++, vld._size);
        _learnPredWeightsKernel.setArg(argIndex++, vl._hiddenToVisible);
        _learnPredWeightsKernel.setArg(argIndex++, vld._radius);
        _learnPredWeightsKernel.setArg(argIndex++, vld._alpha);

        cs.getQueue().enqueueNDRangeKernel(_learnPredWeightsKernel, cl::NullRange, cl::NDRange(_hiddenSize.x, _hiddenSize.y));

        std::swap(vl._weights[_front], vl._weights[_back]);
    }
}

void PredictorLayer::clearMemory(ComputeSystem &cs) {
    cl_float4 zeroColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    cl::array<cl::size_type, 3> zeroOrigin = { 0, 0, 0 };
    cl::array<cl::size_type, 3> hiddenRegion = { static_cast<cl_uint>(_hiddenSize.x), static_cast<cl_uint>(_hiddenSize.y), 1 };

    // Clear buffers
    cs.getQueue().enqueueFillImage(_hiddenStates[_back], zeroColor, zeroOrigin, hiddenRegion);
}


void PredictorLayer::VisibleLayerDesc::load(const schemas::VisiblePredictorLayerDesc* fbVisiblePredictorLayerDesc, ComputeSystem &cs) {
    _size = cl_int2{ fbVisiblePredictorLayerDesc->_size().x(), fbVisiblePredictorLayerDesc->_size().y() };
    _radius = fbVisiblePredictorLayerDesc->_radius();
    _alpha = fbVisiblePredictorLayerDesc->_alpha();
}

schemas::VisiblePredictorLayerDesc PredictorLayer::VisibleLayerDesc::save(flatbuffers::FlatBufferBuilder &builder, ComputeSystem &cs) {
    schemas::int2 size(_size.x, _size.y);
    return schemas::VisiblePredictorLayerDesc(size, _radius, _alpha);
}

void PredictorLayer::VisibleLayer::load(const schemas::VisiblePredictorLayer* fbVisiblePredictorLayer, ComputeSystem &cs) {
    _hiddenToVisible = cl_float2{ fbVisiblePredictorLayer->_hiddenToVisible()->x(), fbVisiblePredictorLayer->_hiddenToVisible()->y() };
    _visibleToHidden = cl_float2{ fbVisiblePredictorLayer->_visibleToHidden()->x(), fbVisiblePredictorLayer->_visibleToHidden()->y() };
    _reverseRadii = cl_int2{ fbVisiblePredictorLayer->_reverseRadii()->x(), fbVisiblePredictorLayer->_reverseRadii()->y() };
    ogmaneo::load(_derivedInput, fbVisiblePredictorLayer->_derivedInput(), cs);
    ogmaneo::load(_weights, fbVisiblePredictorLayer->_weights(), cs);
}

flatbuffers::Offset<schemas::VisiblePredictorLayer> PredictorLayer::VisibleLayer::save(flatbuffers::FlatBufferBuilder &builder, ComputeSystem &cs) {
    schemas::float2 hiddenToVisible(_hiddenToVisible.x, _hiddenToVisible.y);
    schemas::float2 visibleToHidden(_visibleToHidden.x, _visibleToHidden.y);
    schemas::int2 reverseRadii(_reverseRadii.x, _reverseRadii.y);

    return schemas::CreateVisiblePredictorLayer(builder,
        ogmaneo::save(_derivedInput, builder, cs),
        ogmaneo::save(_weights, builder, cs),
        &hiddenToVisible, &visibleToHidden, &reverseRadii);
}

void PredictorLayer::load(const schemas::PredictorLayer* fbPredictorLayer, ComputeSystem &cs) {
    if (!_visibleLayers.empty()) {
        assert(_hiddenSize.x == fbPredictorLayer->_hiddenSize()->x());
        assert(_hiddenSize.y == fbPredictorLayer->_hiddenSize()->y());
        assert(_visibleLayerDescs.size() == fbPredictorLayer->_visibleLayerDescs()->Length());
        assert(_visibleLayers.size() == fbPredictorLayer->_visibleLayers()->Length());
    }
    else {
        _hiddenSize.x = fbPredictorLayer->_hiddenSize()->x();
        _hiddenSize.y = fbPredictorLayer->_hiddenSize()->y();
        _visibleLayerDescs.reserve(fbPredictorLayer->_visibleLayerDescs()->Length());
        _visibleLayers.reserve(fbPredictorLayer->_visibleLayers()->Length());
    }
    _hiddenSize = cl_int2{ fbPredictorLayer->_hiddenSize()->x(), fbPredictorLayer->_hiddenSize()->y() };

    ogmaneo::load(_hiddenSummationTemp, fbPredictorLayer->_hiddenSummationTemp(), cs);
    ogmaneo::load(_hiddenStates, fbPredictorLayer->_hiddenStates(), cs);

    if (fbPredictorLayer->_inhibitSparseFeatures()) {
        if (_inhibitSparseFeatures == nullptr) {
            switch (fbPredictorLayer->_inhibitSparseFeatures()->_sf_type())
            {
            case schemas::SparseFeaturesType::SparseFeaturesType_SparseFeaturesChunk:
                _inhibitSparseFeatures = std::make_shared<SparseFeaturesChunk>();
                break;
            case schemas::SparseFeaturesType::SparseFeaturesType_SparseFeaturesDelay:
                _inhibitSparseFeatures = std::make_shared<SparseFeaturesDelay>();
                break;
            case schemas::SparseFeaturesType::SparseFeaturesType_SparseFeaturesSTDP:
                _inhibitSparseFeatures = std::make_shared<SparseFeaturesSTDP>();
                break;
            default:
                // Unknown SparseFeatures type
                assert(0);
                break;
            }
        }
        _inhibitSparseFeatures->load(fbPredictorLayer->_inhibitSparseFeatures(), cs);
    }
    else {
        _inhibitSparseFeatures = nullptr;
    }

    for (flatbuffers::uoffset_t i = 0; i < fbPredictorLayer->_visibleLayerDescs()->Length(); i++) {
        _visibleLayerDescs[i].load(fbPredictorLayer->_visibleLayerDescs()->Get(i), cs);
    }

    for (flatbuffers::uoffset_t i = 0; i < fbPredictorLayer->_visibleLayers()->Length(); i++) {
        _visibleLayers[i].load(fbPredictorLayer->_visibleLayers()->Get(i), cs);
    }
}

flatbuffers::Offset<schemas::PredictorLayer> PredictorLayer::save(flatbuffers::FlatBufferBuilder &builder, ComputeSystem &cs) {
    schemas::int2 hiddenSize(_hiddenSize.x, _hiddenSize.y);

    std::vector<schemas::VisiblePredictorLayerDesc> visibleLayerDescs;
    for (VisibleLayerDesc layerDesc : _visibleLayerDescs)
        visibleLayerDescs.push_back(layerDesc.save(builder, cs));

    std::vector<flatbuffers::Offset<schemas::VisiblePredictorLayer>> visibleLayers;
    for (VisibleLayer layer : _visibleLayers)
        visibleLayers.push_back(layer.save(builder, cs));

    return schemas::CreatePredictorLayer(builder,
        &hiddenSize,
        ogmaneo::save(_hiddenSummationTemp, builder, cs),
        ogmaneo::save(_hiddenStates, builder, cs),
        ((_inhibitSparseFeatures != nullptr) ? _inhibitSparseFeatures->save(builder, cs) : 0),
        builder.CreateVectorOfStructs(visibleLayerDescs),
        builder.CreateVector(visibleLayers));
}