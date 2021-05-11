Texture2D shadowMap : register(t1);
SamplerState colorSampler : register(s0);

struct Light
{
	float4 position;
	float4 color;
};

struct Lighting
{
	float4 ambient;
	float4 surface;
	Light lights[3];
};

cbuffer cbSurfaceColor : register(b0)
{
	float4 surfaceColor;
}

cbuffer cbLighting : register(b1) //Pixel Shader constant buffer slot 1
{
	Lighting lighting;
}


cbuffer cbMapTransform : register(b2)
{
	matrix mapMtx;
}

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION;
	float3 norm : NORMAL;
	float3 view : VIEW;
	float clip : SV_ClipDistance0;
};

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);

//fourth element set to 0 so there will be no specular reflection
static const float3 defLightColor = float3(0.3f, 0.3f, 0.3f);

static const float kd = 0.5, ks = 0.2f, m = 100.0f;

float4 main(PSInput i) : SV_TARGET
{
	float4 texPos = mul(mapMtx,float4(i.worldPos,1.0f));
	texPos /= texPos.w;

	float3 V = normalize(i.view);
	float3 N = normalize(i.norm);
	float3 col = lighting.ambient.xyz * lighting.surface.x;
	float specAlpha = 0.0f;
	for (int k = 0; k < 3; ++k)
	{
		Light li = lighting.lights[k];
		if (li.color.w != 0)
		{
			float3 L = normalize(li.position.xyz - i.worldPos);
			float3 H = normalize(V + L);
			col += li.color.xyz * surfaceColor.xyz * lighting.surface.y * clamp(dot(N, L), 0.0f, 1.0f);
			float nh = dot(N, H);
			nh = clamp(nh, 0.0f, 1.0f);
			nh = pow(nh, lighting.surface.w);
			specAlpha += nh;
			col += li.color.xyz * nh;
		}
	}

	// TODO : 1.18 Include shadow map in light color calculation
	float4 shadow = shadowMap.Sample(colorSampler, texPos.xy);
	if (shadow.r > 0)
		col = defLightColor;

	return saturate(float4(col, surfaceColor.w + specAlpha));
}