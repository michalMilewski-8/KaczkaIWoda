cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

struct VSInput
{
	float3 pos : POSITION;
	float age : TEXCOORD0;
	float size : TEXCOORD1;
	float3 lastPos : TEXCOORD2;
};

struct GSInput
{
	float4 pos : POSITION;
	float age : TEXCOORD0;
	float size : TEXCOORD1;
	float4 lastPos : TEXCOORD2;
};

GSInput main(VSInput i)
{
	GSInput o = (GSInput)0;
	o.pos = float4(i.pos, 1.0f);
	o.pos = mul(viewMatrix, o.pos);
	o.age = i.age;
	o.size = i.size;
	o.lastPos = float4(i.lastPos, 1.0f);
	o.lastPos = mul(viewMatrix, o.lastPos);
	return o;
}