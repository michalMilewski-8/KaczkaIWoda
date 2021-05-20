//struct Light
//{
//	float4 position;
//	float4 color;
//};
//
//struct Lighting
//{
//	float4 ambient;
//	float4 surface;
//	Light lights[3];
//};
//
//cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0 - matches slot in psBilboard.hlsl
//{
//	float4 surfaceColor;
//}
//
//cbuffer cbLighting : register(b1) //Pixel Shader constant buffer slot 1
//{
//	Lighting lighting;
//}

static const float3 lightPos = float3(1, 0.5f, 0);
static const float3 lightCol = float3(1, 1, 1);

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION;
	float2 tex : TEXCOORD;
	float3 norm : NORMAL;
	float3 view : VIEW;
};

sampler samp : register(s0);
texture2D kaczor : register(t0);

float4 main(PSInput i) : SV_TARGET
{
	float3 V = normalize(i.view);
	float3 N = normalize(i.norm);
	float3 T = normalize(cross(N,float3(0.0f,1.0f,0.0f)));
	T = normalize(cross(T, N));
	//float3 col = lighting.ambient.xyz * lighting.surface.x;
	float3 surfaceCol = kaczor.Sample(samp, i.tex).rgb;
	surfaceCol = pow(surfaceCol, 0.4545f);

	float3 col = surfaceCol*0.3f;

	float3 L = normalize(lightPos.xyz - i.worldPos);
	float3 H = normalize(V + L);
	float nh = dot(T, H);
	
	nh = pow(nh, 2.0f);
	nh = sqrt(1 - nh);
	nh = clamp(nh, 0.0f, 1.0f);
	nh = pow(nh, 50.0f);

	col += 0.5f * lightCol.xyz * surfaceCol * nh;

	col += 0.2f * lightCol.xyz * surfaceCol * clamp(dot(L,N), 0.0f, 1.0f);;

	return float4(col, 1.0f);

	//return saturate(float4(col, surfaceColor.w + specAlpha));
}