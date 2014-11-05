#include"D3DX_DXGIFormatConvert.inl"// this file provide utility funcs for format conversion
#include"header.h"

RWTexture3D<uint> g_txVolume : register(t0);// only uint format can achieve simultaneous read and write
static const int3 changeSpeed = int3( 1, 1, 1 )*2;
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
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
	float3 initialCol = (input.Coord + voxelResolution * 0.5f) / voxelResolution;
	float density = 1.f - (dot(normalize(initialCol),float3(0.5573f,0.5573f,0.5573f))-0.8165f) / 0.1835f;
	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM( float4( initialCol, density ));
	g_txVolume[ input.Coord + voxelResolution*0.5f] = write;

	return float4( 0,0,0, 0 );
	//return float4( 1,0,0, 0 );
}
float4 PS_R(PS_INPUT input) : SV_Target
{
	uint read = g_txVolume[ input.Coord + voxelResolution*0.5f];
	float4 value = D3DX_R8G8B8A8_UNORM_to_FLOAT4( read );// depend on result format, choose different conversion func
	int3 col = value.xyz * 256; 
	col = ( col + changeSpeed ) % 256;
	float3 ncol = col / 256.f;
	float density = 1.f - (dot(normalize(ncol),float3(0.5573f,0.5573f,0.5573f))-0.8165f) / 0.1835f;
	uint write = D3DX_FLOAT4_to_R8G8B8A8_UNORM( float4( ncol, density ));
	g_txVolume[ input.Coord + voxelResolution*0.5] = write;
	return float4( 1,0,0, 0 );
}

