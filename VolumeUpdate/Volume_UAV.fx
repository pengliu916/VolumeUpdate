#include"D3DX_DXGIFormatConvert.inl"// this file provide utility funcs for format conversion
#include"header.h"

RWTexture3D<uint> g_txVolume : register(t0);// only uint format can achieve simultaneous read and write
static const int3 changeSpeed = int3( 1, 1, 1 )*2;
static const float3 voxelResolution = float3( VOLUME_SIZE, VOLUME_SIZE, VOLUME_SIZE );

//--------------------------------------------------------------------------------------
// Compute Shader
//--------------------------------------------------------------------------------------
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void CS(uint3 DTid: SV_DispatchThreadID, uint Tid : SV_GroupIndex)
{
	float3 Coord = DTid - voxelResolution*0.5;
	float3 initialCol = (Coord + voxelResolution * 0.5f) / voxelResolution;
	float density = 1.f - (dot(normalize(initialCol), float3(0.5573f, 0.5573f, 0.5573f)) - 0.8165f) / 0.1835f;
	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM(float4(initialCol, density));
	g_txVolume[Coord + voxelResolution*0.5f] = write;
}
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void CS_R(uint3 DTid: SV_DispatchThreadID, uint Tid : SV_GroupIndex)
{
	float3 Coord = DTid - voxelResolution*0.5;
	uint read = g_txVolume[Coord + voxelResolution*0.5f];
	float4 value = D3DX_R8G8B8A8_UNORM_to_FLOAT4(read);// depend on result format, choose different conversion func
	int3 col = value.xyz * 256;
	col = (col + changeSpeed) % 256;
	float3 ncol = col / 256.f;
	float density = 1.f - (dot(normalize(ncol), float3(0.5573f, 0.5573f, 0.5573f)) - 0.8165f) / 0.1835f;
	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM(float4(ncol, density));
	g_txVolume[Coord + voxelResolution*0.5] = write;
}

