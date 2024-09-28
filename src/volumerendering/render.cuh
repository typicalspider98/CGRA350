#pragma once

#ifndef __CUDACC__
#define __CUDACC__
#endif

#include "radiancePredict.cuh"

extern __device__ int Resolution;
extern __device__ float maxDensity;
extern __device__ int frameNum;
extern __device__ int randNum;

extern __device__ float enviroment_exp;
extern __device__ float tr_scale;
extern __device__ float3 scatter_rate;
extern __device__ float IOR;

typedef float3(*SkyBoxFunc)(float3);
typedef float(*DensityFunc)(float3);


__device__ float3 SkyBox(float3 dir);

__device__ float Density(float3 pos);

__device__ int Hash(int a);

__device__ void InitRand(curandState* seed);

__device__ float Rand(curandState* seed);

__device__ float Tr(curandState* seed, float3 pos, float3 dir, float dis, float alpha = 1);

__device__ bool DeterminateNextVertex(curandState* seed, float alpha, float g, float3 pos, float3 dir, float dis, float3* nextPos, float3* nextDir);

__device__ float4 GetSamplePoint(curandState* seed, float3 ori, float3 dir, float alpha = 1);

__device__ float4 CalculateRadiance(float3 ori, float3 dir, float3 lightDir, float3 lightColor = { 1, 1, 1 }, float alpha = 1, int multiScatter = 1, float g = 0, int sampleNum = 1);

__device__ float3 ShadowTerm(float3 ori, float3 lightDir, float3 dir, float3 lightColor, float alpha, float g);

__device__ float3 GetSample(float3 ori, float3 dir, float3 lightDir, float3 lightColor = { 1, 1, 1 }, float scatter = 1.0f, float alpha = 1, int multiScatter = 1, float g = 0, int sampleNum = 1);

__device__ float3 ShadowTerm_TR(float3 ori, float3 dir, float3 lightDir, float alpha, float g);

__device__ float3 ShadowTerm_TRs(float3 ori, float3 dir, float3 lightDir, float3 lightColor, float alpha, float g, int sampleNum);

struct SampleBasis
{
    float3x3 Main, Light;
};

struct Task
{
    float3 pos;
    float weight;
    int lane;
};

__device__ float3x3 GetMatrixFromNormal(curandState* seed, float3 v1);

__device__ float3 Normal(float3 pos);

__device__ float GGXTerm(const float NdotH, const float roughness);

__device__ float Pow5(float x);

__device__ float SmithJointGGXVisibilityTerm(const float NdotL, const float NdotV, const float roughness);

__device__ float3 FresnelTerm(const float3 F0, const float cosA);

__device__ float PhysicsFresnel(float IOR, float3 i, float3 n);

__device__ float3 refract(float3 i, float3 n, float eta);

__device__ float3 reflect(float3 i, float3 n);

__device__ float4 BRDF(float3 normal, const float3 viewDir, const float3 lightDir);

enum Type {
    marching_MRPNN = -1,
    RPNN = 0,
    MRPNN = 1
};

template<int type = Type::MRPNN, int BatchSize = 4, int StepNum = 1024>
__device__ float4 NNPredict(float3 ori, float3 dir, float3 lightDir, float3 lightColor = { 1, 1, 1 }, float alpha = 1, float g = 0);

__device__ float3 Gamma(float3 color);

__device__ float3 unity_to_ACES(float3 x);

__device__ float3 ACES_to_ACEScg(float3 x);

__device__ float3 XYZ_2_xyY(float3 XYZ);

__device__ float3 xyY_2_XYZ(float3 xyY);

__device__ float3 darkSurround_to_dimSurround(float3 linearCV);

__device__ float3 ACES(float3 color);