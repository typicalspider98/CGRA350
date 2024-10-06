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

template<int type = MRPNN, int BatchSize = 4, int StepNum = 1024>
__device__ float4 NNPredict(float3 ori, float3 dir, float3 lightDir, float3 lightColor = { 1, 1, 1 }, float alpha = 1, float g = 0) {

    dir = normalize(dir);
    lightDir = normalize(lightDir);

    curandState seed;
    InitRand(&seed);

    if (type != -1) {
        float4 spoint = GetSamplePoint(&seed, ori, dir, alpha);
        float3 pos = make_float3(spoint);
        SampleBasis sb = { GetMatrixFromNormal(&seed, dir), GetMatrixFromNormal(&seed, lightDir) };

        bool active = spoint.w > 0;
        int lane_id = __lane_id();
        int aint = active ? 1 : 0;
        aint = __ballot_sync(0xFFFFFFFFU, aint);

        if (!aint)
            return make_float4(SkyBox(dir), -1);

        float3 predict;

        if (type == Type::MRPNN)
        {
            predict = RadiancePredict(&seed, spoint.w > 0, pos, lightDir,
                sb.Main.x, sb.Main.y, sb.Main.z,
                sb.Light.x, sb.Light.y, sb.Light.z,
                alpha, g, scatter_rate / 1.001);
        }
        else
        {
            predict = RadiancePredict_RPNN(&seed, spoint.w > 0, pos, lightDir,
                sb.Main.x, sb.Main.y, sb.Main.z,
                sb.Light.x, sb.Light.y, sb.Light.z,
                alpha, g, scatter_rate / 1.001);
        }
        {
            float dis = RayBoxDistance(pos, sb.Light.x);
            float phase = HenyeyGreenstein(dot(sb.Main.x, sb.Light.x), g);
            float tr = (Tr(&seed, pos, sb.Light.x, dis, alpha) * phase);
            predict = predict + tr;
        }

        /*
        {   // target function
            if (active > 0) {
                predict = GetSample(make_float3(spoint), dir, lightDir, lightColor, scatter_rate.x / 1.001, alpha, 512, g, 1);
            }
        }
        */

        return make_float4(active > 0 ? predict * lightColor * scatter_rate : SkyBox(dir), active > 0 ? distance(ori, pos) : -1);
    }
    else {
        bool unfinish = true;
        SampleBasis basis = { GetMatrixFromNormal(&seed, dir), GetMatrixFromNormal(&seed, lightDir) };

        float offset = RayBoxOffset(ori, dir);
        if (offset < 0)
            unfinish = false;

        ori = ori + dir * offset;
        float dis = RayBoxDistance(ori, dir);
        float inv = dis / (StepNum - 1);
        float transmittance = 1.0;

        float t = 0;
        int step = 0;

        bool hit = IOR > 0 ? false : true;
        float hitDis = -1;
        float3 ref = { 0, 0, 0 };
        float total_weight = 0;
        float3 pos = { 0,0,0 };
        for (; unfinish; step++)
        {
            if (transmittance < 0.005 || t >= dis || step > StepNum * 2)
                break;

            float3 samplePos = ori + dir * t;

            float voxel_data = Density(samplePos);
            hitDis = voxel_data > 0 && hitDis == -1 ? offset + t : hitDis;
            t += max(inv, -voxel_data);

            if (voxel_data <= 0)
                continue;

            if (!hit) {
                float3 n = Normal(samplePos);
                n = n - dir * max(0.f, dot(n, dir) + 0.01);
                float4 tmp = BRDF(n, -dir, lightDir);
                float dis = RayBoxDistance(samplePos, lightDir);
                ref = float3{ tmp.x,tmp.y,tmp.z } *Tr(&seed, samplePos, lightDir, dis, alpha) * lightColor;
                if (Rand(&seed) < tmp.w) {
                    ori = samplePos;
                    dir = reflect(dir, n);
                    t = 0;
                    dis = RayBoxDistance(ori, dir);
                    inv = dis / (StepNum - 1);
                    basis = { GetMatrixFromNormal(&seed, dir), GetMatrixFromNormal(&seed, lightDir) };
                }
                else {
                    ori = samplePos;
                    dir = refract(dir, n, 1.0f / IOR);
                    t = 0;
                    dis = RayBoxDistance(ori, dir);
                    inv = dis / (StepNum - 1);
                    basis = { GetMatrixFromNormal(&seed, dir), GetMatrixFromNormal(&seed, lightDir) };
                }
                hit = true;
            }

            float CurrentDensity = voxel_data * alpha;
            float t_rate = exp(-CurrentDensity * inv);

            float weight = transmittance * (1 - t_rate);
            total_weight += weight;

            float rate = weight / total_weight;
            if (Rand(&seed) < rate) {
                pos = samplePos;
            }

            transmittance *= t_rate;
        }

        // predict
        {
            bool active = hitDis >= 0;
            int lane_id = __lane_id();
            int aint = active ? 1 : 0;
            aint = __ballot_sync(0xFFFFFFFFU, aint);

            if (!aint)
                return make_float4(ref + SkyBox(dir), hitDis);

            float3 res;

            if (type == Type::marching_MRPNN)
            {
                res = RadiancePredict(&seed, active, pos, lightDir,
                    basis.Main.x, basis.Main.y, basis.Main.z,
                    basis.Light.x, basis.Light.y, basis.Light.z,
                    alpha, g, scatter_rate / 1.001);
            }
            else
            {
                res = RadiancePredict_RPNN(&seed, active, pos, lightDir,
                    basis.Main.x, basis.Main.y, basis.Main.z,
                    basis.Light.x, basis.Light.y, basis.Light.z,
                    alpha, g, scatter_rate / 1.001);
            }
            {
                float dis = RayBoxDistance(pos, basis.Light.x);
                float phase = HenyeyGreenstein(dot(basis.Main.x, basis.Light.x), g);
                float tr = (Tr(&seed, pos, basis.Light.x, dis, alpha) * phase);
                res = res + tr;
            }

            return make_float4(ref + lerp(res * lightColor, SkyBox(dir), transmittance), hitDis);
        }
    }
}

__device__ float3 Gamma(float3 color);

__device__ float3 unity_to_ACES(float3 x);

__device__ float3 ACES_to_ACEScg(float3 x);

__device__ float3 XYZ_2_xyY(float3 XYZ);

__device__ float3 xyY_2_XYZ(float3 xyY);

__device__ float3 darkSurround_to_dimSurround(float3 linearCV);

__device__ float3 ACES(float3 color);