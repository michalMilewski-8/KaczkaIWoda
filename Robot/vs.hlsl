cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0 - matches slot in vsBilboard.hlsl
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1 - matches slot in vsBilboard.hlsl
{
	matrix viewMatrix;
	matrix invViewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2 - matches slot in vsBilboard.hlsl
{
	matrix projMatrix;
};

cbuffer cbPlane :register(b3)
{
	float4 planePos;
	float4 planeDir;
};

struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION;
	float3 tex : TEXPOSITION;
	float3 norm : NORMAL;
	float3 view : VIEW;
	float clip : SV_ClipDistance0;
};
PSInput main(VSInput i)
{
	PSInput o;
	o.worldPos = mul(worldMatrix, float4(i.pos, 1.0f)).xyz;
	o.tex = normalize(o.worldPos);

	o.pos = mul(viewMatrix, float4(o.worldPos, 1.0f));
	o.pos = mul(projMatrix, o.pos);

	o.norm = mul(worldMatrix, float4(i.norm, 0.0f)).xyz;
	o.norm = normalize(o.norm);

	float3 camPos = mul(invViewMatrix, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	o.view = camPos - o.worldPos;
	o.clip = dot(o.worldPos - float3(planePos.x, planePos.y, planePos.z), float3(planeDir.x, planeDir.y, planeDir.z)) + 0.0001f;
	return o;
}