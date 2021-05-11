cbuffer cbProj : register(b0)
{
	matrix projMatrix;
};

struct GSInput
{
	float4 pos : POSITION;
	float age : TEXCOORD0;
	float size : TEXCOORD1;
	float4 lastPos : TEXCOORD2;
};

struct PSInput
{
	float4 pos : SV_POSITION;
	float2 tex: TEXCOORD0;
	float age : TEXCOORD1;
};

static const float TimeToLive = 2.0f;

[maxvertexcount(8)]
void main(point GSInput inArray[1], inout TriangleStream<PSInput> ostream)
{


	GSInput i = inArray[0];
	float4 pos = i.pos;
	float4 lastPos = i.lastPos;

	//lewy górny
	PSInput lg = (PSInput)0;
	lg.pos = pos;
	lg.tex.x = 0.0f;
	lg.tex.y = 1.0f;
	lg.age = i.age;

	//lewy dolny
	PSInput ld = (PSInput)0;
	ld.tex.x = 1.0f;
	ld.tex.y = 1.0f;
	ld.age = i.age;

	//prawy górny
	PSInput pg = (PSInput)0;
	pg.pos = lastPos;
	pg.tex.x = 0.0f;
	pg.tex.y = 0.0f;
	pg.age = i.age;

	//prawy dolny
	PSInput pd = (PSInput)0;
	pd.tex.x = 1.0f;
	pd.tex.y = 0.0f;
	pd.age = i.age;


	lg.pos = mul(projMatrix, lg.pos);
	ld.pos = mul(projMatrix, ld.pos);
	pg.pos = mul(projMatrix, pg.pos);
	pd.pos = mul(projMatrix, pd.pos);

	pd.pos.x = pg.pos.x + 0.02f;
	pd.pos.y = pg.pos.y + 0.02f;
	pd.pos.z = pg.pos.z;
	pd.pos.w = pg.pos.w;


	ld.pos.y = lg.pos.y + 0.02f;
	ld.pos.x = lg.pos.x + 0.02f;
	ld.pos.z = lg.pos.z;
	ld.pos.w = lg.pos.w;


	if (i.age > 0)
	{
		ostream.Append(ld);
		ostream.Append(lg);
		ostream.Append(pd);
		ostream.Append(pg);
	}
	ostream.RestartStrip();

	if (i.age > 0)
	{
		ostream.Append(ld);
		ostream.Append(pd);
		ostream.Append(lg);
		ostream.Append(pg);
	}
	ostream.RestartStrip();
}