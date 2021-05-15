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

cbuffer cbSurfaceColor : register(b0) //Pixel Shader constant buffer slot 0 - matches slot in psBilboard.hlsl
{
	float4 surfaceColor;
}

cbuffer cbLighting : register(b1) //Pixel Shader constant buffer slot 1
{
	Lighting lighting;
}

struct PSInput
{
	float4 pos : SV_POSITION;
	float3 worldPos : POSITION;
	float3 tex : TEXPOSITION;
	float3 norm : NORMAL;
	float3 view : VIEW;
	float clip : SV_ClipDistance0;
};

sampler samp : register(s0);
textureCUBE envMap : register(t0);

float4 main(PSInput i) : SV_TARGET
{
	float3 V = normalize(i.view);
	float3 N = normalize(i.norm);
	//float3 col = lighting.ambient.xyz * lighting.surface.x;
	float3 col = envMap.Sample(samp, i.tex).rgb;
	col = pow(col, 0.4545f);
	return float4(col,1.0f);
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

	return saturate(float4(col, surfaceColor.w + specAlpha));
}