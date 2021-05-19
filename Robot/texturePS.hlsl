

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
};

//cbuffer cbLights : register(b1)
//{
//	float4 lightPos;
//};



Texture2D colorMap : register(t0);
textureCUBE envMap  : register(t1);
SamplerState colorSampler : register(s0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: NORMAL;
	float3 worldPos : POSITION0;
	float3 viewVec : TEXCOORD0;
};


float3 intersectRay(float3 p, float3 d) {
	float tx = max((1 - p.x) / d.x, (-1 - p.x) / d.x);
	float ty = max((1 - p.y) / d.y, (-1 - p.y) / d.y);
	float tz = max((1 - p.z) / d.z, (-1 - p.z) / d.z);
	float t = min(tx, min(ty, tz));
	return p + d * t;
}
float fresnel(float3 N, float3 V) {
	float n2 = 4.0f / 3.0f;
	float n1 = 1.0f;
	//float Fo = (n2 - n1) / (n2 + n1);
	//Fo = Fo * Fo;
	float Fo = 0.17f;
	if (dot(N, V) < 0) N = -N;
	float cs = max(dot(N, V), 0);
	return Fo + (1 - Fo) * (1 - cs) * (1 - cs) * (1 - cs) * (1 - cs) * (1 - cs);
}


static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float kd = 0.5, ks = 0.2f, m = 100.0f;
static const float4 lightPos = float4(0.0f, 0.5f, 1.0f, 1.0f);

float4 main(PSInput i) : SV_TARGET
{
	float3 viewVec = normalize(i.viewVec);
	float2 tex = i.tex;
	tex.x = tex.x * surfaceColor.y;
	float3 normal = normalize(colorMap.SampleLevel(colorSampler, tex, 0).rgb *2.0f -1.0f);
	//return float4(normal, 1.0f);
	float wsp = 0.14f;
	if (dot(normal, viewVec) < 0) {
		normal = -normal;
		wsp = 4.0f / 3.0f;
	}
	float3 d = reflect(-viewVec, normal);
	float3 p = refract(-viewVec, normal, wsp);
	float4 colorD = envMap.Sample(colorSampler, intersectRay(i.worldPos, d));
	float4 colorP = envMap.Sample(colorSampler, intersectRay(i.worldPos, p));
	float f = fresnel(normal, viewVec);
	float4 color = lerp(colorP, colorD, f);
	if (!any(p)) {
		color = colorD;
	}


	color = pow(color, 0.4545f);
	return color;


	//float3 color = surfaceColor.rgb * ambientColor;
	//float3 lightPosition = lightPos.xyz;
	//float3 lightVec = normalize(lightPosition - i.worldPos);
	//float3 halfVec = normalize(viewVec + lightVec);
	//color += lightColor * surfaceColor.xyz * kd * saturate(dot(normal, lightVec)); //diffuse color
	//float nh = dot(normal, halfVec);
	//nh = saturate(nh);
	//nh = pow(nh, m);
	//nh *= ks;
	//color += lightColor * nh;
	//return float4(saturate(color), surfaceColor.a);
}