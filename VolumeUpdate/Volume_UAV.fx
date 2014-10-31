#include"D3DX_DXGIFormatConvert.inl"// this file provide utility funcs for format conversion
#include"header.h"

RWTexture3D<uint> g_txVolume : register(t0);// only uint format can achieve simultaneous read and write
static const int3 changeSpeed = int3( 3, 1, 2 )*5;
static const float3 voxelResolution = float3( VOLUME_SIZE, VOLUME_SIZE, VOLUME_SIZE );

struct GS_INPUT
{
};

struct PS_INPUT
{
	float4	Pos : SV_POSITION;
	float3  Coord : TEXCOORD0;
    uint	PrimID :SV_RenderTargetArrayIndex;
};

GS_INPUT VS( )
{
    GS_INPUT output = (GS_INPUT)0;
 
    return output;
}

[maxvertexcount(4)]
void GS(point GS_INPUT particles[1], uint primID : SV_PrimitiveID, inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.Pos=float4(-1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(-1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(-voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5);
	output.PrimID=primID;
	triStream.Append(output);

	output.Pos=float4(1.0f,-1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5);
	output.PrimID=primID;
	triStream.Append(output);


	output.Pos=float4(1.0f,1.0f,0.0f,1.0f);
	output.Coord=float3(voxelResolution.x/2.0,-voxelResolution.y/2.0,(float)primID-voxelResolution.z/2.0+0.5);
	output.PrimID=primID;
	triStream.Append(output);
}


//--------------------------------------------------------------------------------------
// Compute Shader
//--------------------------------------------------------------------------------------
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void CS(uint3 DTid: SV_DispatchThreadID, uint Tid : SV_GroupIndex)
{
	float3 Coord = DTid - voxelResolution*0.5;
	float pi = 3.1415926;
	float phase_x = Coord.x / voxelResolution.x * 3.14;
	float phase_y = Coord.y / voxelResolution.y * 3.14;
	float phase_z = Coord.z / voxelResolution.z * 3.14;
	float r = cos( phase_x ) * 0.5 + 0.5;
	float g = cos( phase_y + pi / 3.0 ) * 0.5 + 0.5;
	float b = cos( phase_z + 2.0 * pi / 3.0 ) * 0.5 + 0.5;

	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM( float4( r, g, b, 0 ));
	g_txVolume[ Coord + voxelResolution/2] = write;
}
[numthreads(THREAD_X, THREAD_Y, THREAD_Z)]
void CS_R(uint3 DTid: SV_DispatchThreadID, uint Tid : SV_GroupIndex)
{
	float3 Coord = DTid - voxelResolution*0.5;
	uint read = g_txVolume[ Coord + voxelResolution/2];
	float4 value = D3DX_R8G8B8A8_UNORM_to_FLOAT4( read );// depend on result format, choose different conversion func
	int3 col = value * 256; 
	col = ( col + changeSpeed*(abs(Coord/voxelResolution)+0.2) ) % 256;
	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM( float4( col / 256.0, 0 ));
	g_txVolume[ Coord + voxelResolution/2] = write;
}

