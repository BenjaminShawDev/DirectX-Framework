//--------------------------------------------------------------------------------------
// File: DX11 Framework.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;

    float4 DiffuseMtrl;
    float4 DiffuseLight;
    float4 AmbientMtrl;
    float4 AmbientLight;
    float4 SpecularMtrl;
    float4 SpecularLight;
    float SpecularPower;
    float3 EyePosW;
    float3 LightVecW;
}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
};

//struct VS_INPUT
//{
//    float4 Pos : POSITION;
//    float2 Tex : TEXCOORD0;
//};
//
//struct PS_INPUT
//{
//    float4 Pos : SV_POSITION;
//    float2 Tex : TEXCOORD0;
//};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(float4 Pos : POSITION, float3 NormalL : NORMAL, float2 Tex : TEXCOORD0)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.Pos = mul(Pos, World);
    output.PosW = normalize(EyePosW - output.Pos.xyz);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    float3 normalW = mul(float4(NormalL, 0.0f), World).xyz;
    output.Norm = normalize(normalW);

    output.Tex = Tex;

    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_Target
{
    //Ambient
    float3 ambient = AmbientMtrl * AmbientLight;

    //Diffuse
    float diffuseAmount = max(dot(LightVecW, input.Norm), 0.0f);
    float3 diffuse = diffuseAmount * (DiffuseMtrl * DiffuseLight).rgb;

    //Specular
    float3 r = reflect(-LightVecW, input.Norm);
    float specularAmount = pow(max(dot(r, input.PosW), 0.0f), SpecularPower);
    float3 specular = specularAmount * (SpecularMtrl * SpecularLight).rgb;

    float4 textureColour = txDiffuse.Sample(samLinear, input.Tex);

    float4 outColour;
    outColour.rgb = (ambient + diffuse + specular) * textureColour;
    outColour.a = DiffuseMtrl.a;

    clip(outColour.a - 0.25f);

    return outColour;

    //return input.Color;
}
