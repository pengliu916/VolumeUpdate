#include "header.h"
Texture3D g_txVolume : register(t0);

SamplerState samRaycast : register(s0);

static const float density = 0.02;

cbuffer cbChangesEveryFrame : register( b0 )
{
	matrix WorldViewProjection;
	float4 viewPos;
};

// TSDF related variable
static const float3 voxelResolution = float3(VOLUME_SIZE, VOLUME_SIZE, VOLUME_SIZE);
static const float3 boxMin = float3( -1.0, -1.0, -1.0 )*voxelResolution/2.0f;
static const float3 boxMax = float3( 1.0, 1.0, 1.0 )*voxelResolution/2.0f;
static const float3 reversedWidthHeightDepth = 1.0f/(voxelResolution);

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct GS_INPUT
{
};

struct PS_INPUT
{
	float4	projPos : SV_POSITION;
	float4	Pos : TEXCOORD0;
};

struct Ray
{
	float4 o;
	float4 d;
};

//--------------------------------------------------------------------------------------
// Utility Functions
//--------------------------------------------------------------------------------------
bool IntersectBox(Ray r, float3 boxmin, float3 boxmax, out float tnear, out float tfar)
{
	// compute intersection of ray with all six bbox planes
	float3 invR = 1.0 / r.d.xyz;
		float3 tbot = invR * (boxmin.xyz - r.o.xyz);
		float3 ttop = invR * (boxmax.xyz - r.o.xyz);

		// re-order intersections to find smallest and largest on each axis
		float3 tmin = min (ttop, tbot);
		float3 tmax = max (ttop, tbot);

		// find the largest tmin and the smallest tmax
		float2 t0 = max (tmin.xx, tmin.yz);
		tnear = max (t0.x, t0.y);
	t0 = min (tmax.xx, tmax.yz);
	tfar = min (t0.x, t0.y);

	return tnear<=tfar;
}
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
GS_INPUT VS( )
{
	GS_INPUT output = (GS_INPUT)0;

	return output;
}

//--------------------------------------------------------------------------------------
// Geometry Shader
//--------------------------------------------------------------------------------------
/*GS for rendering the volume on screen ------------texVolume Read, no half pixel correction*/
[maxvertexcount(18)]
void GS(point GS_INPUT particles[1], inout TriangleStream<PS_INPUT> triStream)
{
	PS_INPUT output;
	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

	triStream.RestartStrip();

	output.Pos=float4(1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

	triStream.RestartStrip();

	output.Pos=float4(1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,-1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);
	output.Pos=float4(-1.0f,-1.0f,1.0f,1.0f)*float4(voxelResolution/2,1);
	output.projPos=mul(output.Pos,WorldViewProjection);
	triStream.Append(output);

}


float4 PS(PS_INPUT input) : SV_Target
{
	float4 output = float4 ( 0, 0, 0, 0 );

	Ray eyeray;
	//world space
	eyeray.o = viewPos;
	eyeray.d = input.Pos-eyeray.o;
	eyeray.d = normalize(eyeray.d);
	eyeray.d.x = ( eyeray.d.x == 0.f ) ? 1e-15 : eyeray.d.x;
	eyeray.d.y = ( eyeray.d.y == 0.f ) ? 1e-15 : eyeray.d.y;
	eyeray.d.z = ( eyeray.d.z == 0.f ) ? 1e-15 : eyeray.d.z;

	// calculate ray intersection with bounding box
	float tnear, tfar;
	bool hit = IntersectBox(eyeray, boxMin, boxMax , tnear, tfar);
	if (!hit) return output;       

	// calculate intersection points
	float3 Pnear = eyeray.o.xyz + eyeray.d.xyz * tnear;
	float3 Pfar = eyeray.o.xyz + eyeray.d.xyz * tfar;

	float3 P = Pnear;
	float t = tnear;
	float tSmallStep = 5 ;
	float3 P_pre = Pnear;
	float3 PsmallStep = eyeray.d.xyz * tSmallStep;

	float3 currentPixPos;
	
	while ( t <= tfar ) {
		float3 txCoord = P * reversedWidthHeightDepth + 0.5;
		float4 value = g_txVolume.SampleLevel ( samRaycast, txCoord, 0 );
		
		output += value * density * value.a * value.a;
	
		P += PsmallStep;
		t += tSmallStep;
	}
	return output;       
	//return float4(1,1,0,0);       
}