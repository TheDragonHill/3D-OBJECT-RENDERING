struct Light
{
	float3 dir;
	float3 pos;
	float range;
	float3 att;
	float4 ambient;
	float4 diffuse;
};

cbuffer cbPerFrame
{
	Light light;
};

cbuffer cbPerObject
{
	float4x4 WVP;
	float4x4 World;
};

Texture2D ObjTexture;
SamplerState ObjSampler;

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 posWorld : POSITION;
	float2 tex : TEXCOORD;
	float3 normal : NORMAL;
};

VS_OUT VS(float4 inPos : POSITION, float2 inTex : TEXCOORD, float3 inNormal : NORMAL)
{
	VS_OUT output;
	output.pos = mul(inPos, WVP);
	output.posWorld = mul(inPos, World);
	output.tex = inTex;
    output.normal = mul(float4(inNormal, 1), World);
	return  output;
}

float4 PS(VS_OUT input) : SV_TARGET
{
	input.normal = normalize(input.normal);
	float4 diffRefl = ObjTexture.Sample(ObjSampler, input.tex);
	float3 col;

	float3 lightToPixel = light.pos - input.posWorld.xyz;
	float dist = length(lightToPixel);
	float3 ambient = diffRefl + light.ambient;

	if (dist > light.range)
	{
		return float4(ambient, diffRefl.a);
	}
	
	lightToPixel /= dist;

	float howMuchIsTheLight = dot(lightToPixel, input.normal);

	if (howMuchIsTheLight > 0)
	{
		col = howMuchIsTheLight + diffRefl + light.diffuse;
		col /= light.att[0] + (light.att[1] * dist) + (light.att[2] * dist * dist);
	}

	col = saturate(col + ambient); 

	return float4(col, diffRefl.a);
	// return float4(1, 0, 0, 1);
}