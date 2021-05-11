Texture2D dropMap : register(t0);
SamplerState colorSampler : register(s0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
	float age : TEXCOORD1;
};

static const float TimeToLive = 3.0f;

float4 main(PSInput i) : SV_TARGET
{
	float4 color = dropMap.Sample(colorSampler, i.tex);
	float alpha = 1.0f - i.age / TimeToLive;
	if (alpha > 1.0f) alpha = 1.0f;
	if (alpha == 0.0f)
		discard;
	return float4(color.xyz,color.w * alpha);
};