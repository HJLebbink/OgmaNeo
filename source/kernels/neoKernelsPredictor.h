// ----------------------------------------------------------------------------
//  OgmaNeo
//  Copyright(c) 2016 Ogma Intelligent Systems Corp. All rights reserved.
//
//  This copy of OgmaNeo is licensed to you under the terms described
//  in the OGMANEO_LICENSE.md file included in this distribution.
// ----------------------------------------------------------------------------

#pragma once

#if !defined(_NEOKERNELSPREDICTOR_OCL_HEADER)
#define _NEOKERNELSPREDICTOR_OCL_HEADER

#include <string>

const std::string neoKernelsPredictor_ocl[] = {
"// ----------------------------------------------------------------------------\n",
"//  OgmaNeo\n",
"//  Copyright(c) 2016 Ogma Intelligent Systems Corp. All rights reserved.\n",
"//\n",
"//  This copy of OgmaNeo is licensed to you under the terms described\n",
"//  in the OGMANEO_LICENSE.md file included in this distribution.\n",
"// ----------------------------------------------------------------------------\n",
"\n",
"// ----------------------------------------- Samplers -----------------------------------------\n",
"\n",
"constant sampler_t defaultSampler = CLK_NORMALIZED_COORDS_FALSE |\n",
"    CLK_ADDRESS_CLAMP_TO_EDGE |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"constant sampler_t normalizedClampedNearestSampler = CLK_NORMALIZED_COORDS_TRUE |\n",
"    CLK_ADDRESS_CLAMP |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"constant sampler_t normalizedClampedToEdgeNearestSampler = CLK_NORMALIZED_COORDS_TRUE |\n",
"    CLK_ADDRESS_CLAMP_TO_EDGE |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"constant sampler_t unnormalizedClampedNearestSampler = CLK_NORMALIZED_COORDS_FALSE |\n",
"    CLK_ADDRESS_CLAMP |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"constant sampler_t defaultNormalizedSampler = CLK_NORMALIZED_COORDS_TRUE |\n",
"    CLK_ADDRESS_CLAMP_TO_EDGE |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"constant sampler_t defaultUnnormalizedSampler = CLK_NORMALIZED_COORDS_FALSE |\n",
"    CLK_ADDRESS_CLAMP_TO_EDGE |\n",
"    CLK_FILTER_NEAREST;\n",
"\n",
"// ----------------------------------------- Common -----------------------------------------\n",
"\n",
"float randFloat(uint2* state) {\n",
"    const float invMaxInt = 1.0f / 4294967296.0f;\n",
"    uint x = (*state).x * 17 + (*state).y * 13123;\n",
"    (*state).x = (x << 13) ^ x;\n",
"    (*state).y ^= (x << 7);\n",
"\n",
"    uint tmp = x * (x * x * 15731 + 74323) + 871483;\n",
"\n",
"    return convert_float(tmp) * invMaxInt;\n",
"}\n",
"\n",
"float randNormal(uint2* state) {\n",
"    float u1 = randFloat(state);\n",
"    float u2 = randFloat(state);\n",
"\n",
"    return sqrt(-2.0f * log(u1)) * cos(6.28318f * u2);\n",
"}\n",
"\n",
"float sigmoid(float x) {\n",
"    return 1.0f / (1.0f + exp(-x));\n",
"}\n",
"\n",
"float relu(float x, float leak) {\n",
"    x += 0.5f;\n",
"\n",
"    if (x > 1.0f)\n",
"        return 1.0f + (x - 1.0f) * leak;\n",
"\n",
"    return x > 0.0f ? x : x * leak;\n",
"}\n",
"\n",
"float relud(float x, float leak) {\n",
"    x += 0.5f;\n",
"\n",
"    return x > 0.0f && x < 1.0f ? 1.0f : leak;\n",
"}\n",
"\n",
"bool inBounds0(int2 position, int2 upperBound) {\n",
"    return position.x >= 0 && position.x < upperBound.x && position.y >= 0 && position.y < upperBound.y;\n",
"}\n",
"\n",
"bool inBounds(int2 position, int2 lowerBound, int2 upperBound) {\n",
"    return position.x >= lowerBound.x && position.x < upperBound.x && position.y >= lowerBound.y && position.y < upperBound.y;\n",
"}\n",
"\n",
"int2 project(int2 position, float2 toScalars) {\n",
"    return (int2)(position.x * toScalars.x + 0.5f, position.y * toScalars.y + 0.5f);\n",
"}\n",
"\n",
"// Initialize a random uniform 2D image (X field)\n",
"void kernel randomUniform2D(write_only image2d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 29 + 12, get_global_id(1) * 16 + 23) * 36;\n",
"\n",
"    int2 position = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float value = randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x;\n",
"\n",
"    write_imagef(values, position, (float4)(value, 0.0f, 0.0f, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 3D image (X field)\n",
"void kernel randomUniform3D(write_only image3d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 12 + 76 + get_global_id(2) * 3, get_global_id(1) * 21 + 42 + get_global_id(2) * 7) * 12;\n",
"\n",
"    int3 position = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));\n",
"\n",
"    float value = randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x;\n",
"\n",
"    write_imagef(values, (int4)(position, 0), (float4)(value, 0.0f, 0.0f, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 2D image (XY fields)\n",
"void kernel randomUniform2DXY(write_only image2d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 15 + 66, get_global_id(1) * 61 + 2) * 56;\n",
"\n",
"    int2 position = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float2 v = (float2)(randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x);\n",
"\n",
"    write_imagef(values, position, (float4)(v.x, v.y, 0.0f, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 2D image (XYZ fields)\n",
"void kernel randomUniform2DXYZ(write_only image2d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 15 + 66, get_global_id(1) * 61 + 2) * 56;\n",
"\n",
"    int2 position = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float3 v = (float3)(randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x);\n",
"\n",
"    write_imagef(values, position, (float4)(v.x, v.y, v.z, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 2D image (XZ fields)\n",
"void kernel randomUniform2DXZ(write_only image2d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 29 + 12, get_global_id(1) * 16 + 23) * 36;\n",
"\n",
"    int2 position = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float2 v = (float2)(randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x);\n",
"\n",
"    write_imagef(values, position, (float4)(v.x, 0.0f, v.y, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 3D image (XY fields)\n",
"void kernel randomUniform3DXY(write_only image3d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 12 + 76 + get_global_id(2) * 3, get_global_id(1) * 21 + 42 + get_global_id(2) * 7) * 12;\n",
"\n",
"    int3 position = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));\n",
"\n",
"    float2 v = (float2)(randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x);\n",
"\n",
"    write_imagef(values, (int4)(position, 0), (float4)(v.x, v.y, 0.0f, 0.0f));\n",
"}\n",
"\n",
"// Initialize a random uniform 3D image (XZ fields)\n",
"void kernel randomUniform3DXZ(write_only image3d_t values, uint2 seed, float2 minMax) {\n",
"    uint2 seedValue = seed + (uint2)(get_global_id(0) * 12 + 76 + get_global_id(2) * 3, get_global_id(1) * 21 + 42 + get_global_id(2) * 7) * 12;\n",
"\n",
"    int3 position = (int3)(get_global_id(0), get_global_id(1), get_global_id(2));\n",
"\n",
"    float2 v = (float2)(randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x, randFloat(&seedValue) * (minMax.y - minMax.x) + minMax.x);\n",
"\n",
"    write_imagef(values, (int4)(position, 0), (float4)(v.x, 0.0f, v.y, 0.0f));\n",
"}\n",
"// ----------------------------------------------------------------------------\n",
"//  OgmaNeo\n",
"//  Copyright(c) 2016 Ogma Intelligent Systems Corp. All rights reserved.\n",
"//\n",
"//  This copy of OgmaNeo is licensed to you under the terms described\n",
"//  in the OGMANEO_LICENSE.md file included in this distribution.\n",
"// ----------------------------------------------------------------------------\n",
"\n",
"// ------------------------------------------- Predictor Layer -------------------------------------------\n",
"\n",
"void kernel plDeriveInputs(read_only image2d_t inputs, read_only image2d_t outputsBack, write_only image2d_t outputsFront) {\n",
"    int2 position = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float input = read_imagef(inputs, defaultSampler, position).x;\n",
"\n",
"    write_imagef(outputsFront, position, (float4)(input, 0.0f, 0.0f, 0.0f));\n",
"}\n",
"\n",
"void kernel plStimulus(read_only image2d_t visibleStates,\n",
"    read_only image2d_t hiddenSummationTempBack, write_only image2d_t hiddenSummationTempFront,\n",
"    read_only image3d_t weights,\n",
"    int2 visibleSize, float2 hiddenToVisible, int radius)\n",
"{\n",
"    int2 hiddenPosition = (int2)(get_global_id(0), get_global_id(1));\n",
"    int2 visiblePositionCenter = project(hiddenPosition, hiddenToVisible);\n",
"\n",
"    float sum = read_imagef(hiddenSummationTempBack, defaultSampler, hiddenPosition).x;\n",
"\n",
"    float subSum = 0.0f;\n",
"\n",
"    int2 fieldLowerBound = visiblePositionCenter - (int2)(radius);\n",
"\n",
"    for (int dx = -radius; dx <= radius; dx++)\n",
"        for (int dy = -radius; dy <= radius; dy++) {\n",
"            int2 visiblePosition = visiblePositionCenter + (int2)(dx, dy);\n",
"\n",
"            if (inBounds0(visiblePosition, visibleSize)) {\n",
"                int2 offset = visiblePosition - fieldLowerBound;\n",
"\n",
"                int wi = offset.y + offset.x * (radius * 2 + 1);\n",
"\n",
"                float weight = read_imagef(weights, defaultSampler, (int4)(hiddenPosition.x, hiddenPosition.y, wi, 0)).x;\n",
"\n",
"                float visibleState = read_imagef(visibleStates, defaultSampler, visiblePosition).x;\n",
"\n",
"                subSum += visibleState * weight;\n",
"            }\n",
"        }\n",
"\n",
"    write_imagef(hiddenSummationTempFront, hiddenPosition, (float4)(sum + subSum, 0.0f, 0.0f, 0.0f));\n",
"}\n",
"\n",
"void kernel plLearnPredWeights(read_only image2d_t visibleStatesPrev,\n",
"    read_only image2d_t targets, read_only image2d_t hiddenStatesPrev,\n",
"    read_only image3d_t weightsBack, write_only image3d_t weightsFront,\n",
"    int2 visibleSize, float2 hiddenToVisible, int radius, float alpha)\n",
"{\n",
"    int2 hiddenPosition = (int2)(get_global_id(0), get_global_id(1));\n",
"    int2 visiblePositionCenter = project(hiddenPosition, hiddenToVisible);\n",
"\n",
"    float error = read_imagef(targets, defaultSampler, hiddenPosition).x - read_imagef(hiddenStatesPrev, defaultSampler, hiddenPosition).x;\n",
"\n",
"    int2 fieldLowerBound = visiblePositionCenter - (int2)(radius);\n",
"\n",
"    for (int dx = -radius; dx <= radius; dx++)\n",
"        for (int dy = -radius; dy <= radius; dy++) {\n",
"            int2 visiblePosition = visiblePositionCenter + (int2)(dx, dy);\n",
"\n",
"            if (inBounds0(visiblePosition, visibleSize)) {\n",
"                int2 offset = visiblePosition - fieldLowerBound;\n",
"\n",
"                int wi = offset.y + offset.x * (radius * 2 + 1);\n",
"\n",
"                float weightPrev = read_imagef(weightsBack, defaultSampler, (int4)(hiddenPosition.x, hiddenPosition.y, wi, 0)).x;\n",
"\n",
"                float visibleStatePrev = read_imagef(visibleStatesPrev, defaultSampler, visiblePosition).x;\n",
"\n",
"                float weight = weightPrev + alpha * error * visibleStatePrev;\n",
"\n",
"                write_imagef(weightsFront, (int4)(hiddenPosition.x, hiddenPosition.y, wi, 0), (float4)(weight, 0.0f, 0.0f, 0.0f));\n",
"            }\n",
"        }\n",
"}\n",
"\n",
"void kernel plThreshold(read_only image2d_t stimuli, write_only image2d_t thresholded) {\n",
"    int2 hiddenPosition = (int2)(get_global_id(0), get_global_id(1));\n",
"\n",
"    float stimulus = read_imagef(stimuli, defaultSampler, hiddenPosition).x;\n",
"\n",
"    write_imagef(thresholded, hiddenPosition, (float4)(stimulus > 0.5f ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f));//fmax(0.0f, fabs(stimulus) - 0.5f) * (stimulus > 0.0f ? 1.0f : -1.0f), 0.0f, 0.0f, 0.0f));\n",
"}\n",
};

#endif
