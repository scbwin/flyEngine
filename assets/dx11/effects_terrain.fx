struct VertexIn
{
	float2 pos : POSITION;
};

struct VertexOut
{
	float4 pos_h : SV_POSITION;
	float2 uv : TEX_COORD;
	float3 normal_world : NORMAL;
	float3 pos_world : POSITION;
};

cbuffer perFrame
{
	float3 lightPosWorld;
	float3 camPosWorld;
	float noiseFrequ;
	float heightScale;
	int numOctaves;
	float ampScale;
	float frequencyScale;
	float uvScaleDetails;
	float4x4 MVP;
	int terrainSize;
	float maxTessFactor;
	float maxTessDistance;
};

Texture2D noiseTexture;
Texture2D terrainTexture;
Texture2D terrainTexture2;
Texture2D terrainNormals;
Texture2D terrainNormals2;

SamplerState samplerLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

SamplerState samplerAni
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 16;
	AddressU = Wrap;
	AddressV = Wrap;
};

float hash(float2 p) {
	return frac(sin(dot(p, float2(12.9898f, 256.31f))) * 43758.5453f);
}

float noise(float2 p)
{
/*	float2 start = floor(p);
	float2 end = start + 1.f;
	float2 weights = smoothstep(start, end, p);
	return lerp(lerp(hash(start), hash(float2(end.x, start.y)), weights.x), lerp(hash(float2(start.x, end.y)), hash(end), weights.x), weights.y);*/
	return noiseTexture.SampleLevel(samplerLinear, p, 0).r;
}

float getHeight(float2 pos)
{
	float height = 0.f;
	float frequ = noiseFrequ;
	float amp = 1.f;
	float amp_sum = 0.f;
	for (int i = 0; i < numOctaves; i++) {
		height += amp * (1.f - abs(noise(pos * frequ) * 2.f - 1.f));
		amp_sum += amp;
		amp *= ampScale;
		frequ *= frequencyScale;
	}
	height /= amp_sum;
	height = pow(abs(height), 10.f);
	float temp_height = height;
	height += 0.1f * noise(pos * noiseFrequ * 0.2f);
	height += 0.046f * noise(pos * noiseFrequ * 0.4f);
	height += 0.012f * noise(pos * noiseFrequ * 0.6f);
	height -= lerp(0.0f, 0.05f, 1.f - smoothstep(0.04f, 0.055f, temp_height));
	height -= lerp(0.0f, 0.05f, 1.f - smoothstep(0.02f, 0.035f, temp_height));
	height -= lerp(0.0f, 0.025f, 1.f - smoothstep(0.01f, 0.02f, temp_height));
	//height += lerp(0.f, 2.f, pow(smoothstep(0.f, 8192.f, length(pos)), 10.f));
	height *= heightScale;
	return height;
}

struct VsOut
{
	float3 pos : POSITION;
};

VsOut vs(uint vertex_id : SV_VertexID) 
{
	VsOut vs_out;
	uint cp_id = vertex_id % 4u; // Control point id
	float2 pos = float2(cp_id & 1, cp_id >> 1); // This generates a control point (either [0,0], [1,0], [0,1] or [1,1]) depending on the id which is between 0 and 3
	uint patch_id = vertex_id / 4u;
	uint patches_per_dir = terrainSize / 64u;
	float2 patch_index = float2(patch_id % patches_per_dir, patch_id / patches_per_dir);
	pos += patch_index; // Offset by patch position
	pos *= 64.f; // Scale by patch size
	pos -= terrainSize * 0.5f; // Center around world origin
	vs_out.pos = float3(pos.x, 0.f, pos.y);
	return vs_out;
}

struct PatchTess
{
	float edgeTess [4] : SV_TessFactor;
	float insideTess [2] : SV_InsideTessFactor;
};

float computeTessFactor(float3 pos)
{
	return pow(2.f, lerp(maxTessFactor, 0.f, smoothstep(0.f, maxTessDistance, distance(pos.xz, camPosWorld.xz))));
}

// Frustum culling, this gives a massive performance boost
bool isVisible (InputPatch<VsOut, 4> patch)
{
	// Min max y is just an approximation, it can happen that some patches are culled which shouldn't, but this is fine for the editor. 
	float3 min = float3(patch[0].pos.x, -heightScale * 0.075f, patch[0].pos.z);
	float3 max = float3(patch[3].pos.x, heightScale * 1.1f, patch[3].pos.z);
	float3 aabb[8];
    aabb[0] = float3(min.x, min.y, min.z);
    aabb[1] = float3(max.x, min.y, min.z);
    aabb[2] = float3(min.x, max.y, min.z);
    aabb[3] = float3(min.x, min.y, max.z);
    aabb[4] = float3(max.x, max.y, min.z);
    aabb[5] = float3(min.x, max.y, max.z);
    aabb[6] = float3(max.x, min.y, max.z);
    aabb[7] = float3(max.x, max.y, max.z);
	bool inside_left = false, inside_right = false, inside_bottom = false, inside_top = false, inside_near = false, inside_far = false;
	for (int i = 0; i < 8; i++) {
		float4 pos_h = mul(MVP, float4(aabb[i], 1.f));
		inside_left = inside_left || pos_h.x >= -pos_h.w;
        inside_right = inside_right || pos_h.x <= pos_h.w;
        inside_bottom = inside_bottom || pos_h.y >= -pos_h.w;
        inside_top = inside_top || pos_h.y <= pos_h.w;
        inside_near = inside_near || pos_h.z >= 0.f;
        inside_far = inside_far || pos_h.z <= pos_h.w;
	}
    return inside_left && inside_right && inside_bottom && inside_top && inside_near && inside_far;
}

PatchTess constHS(InputPatch<VsOut, 4> patch)
{
	PatchTess pt;
	if (isVisible(patch)) {
		pt.edgeTess[0] = computeTessFactor(0.5f * (patch[2].pos + patch[0].pos));
		pt.edgeTess[1] = computeTessFactor(0.5f * (patch[2].pos + patch[3].pos));
		pt.edgeTess[2] = computeTessFactor(0.5f * (patch[1].pos + patch[3].pos));
		pt.edgeTess[3] = computeTessFactor(0.5f * (patch[0].pos + patch[1].pos));
		pt.insideTess[0] = computeTessFactor(0.25f * (patch[0].pos + patch[1].pos + patch[2].pos + patch[3].pos));
		pt.insideTess[1] = pt.insideTess[0];
	}
	else {
		pt.edgeTess[0] = pt.edgeTess[1] = pt.edgeTess[2] = pt.edgeTess[3] = pt.insideTess[0] = pt.insideTess[1] = 0.f;
	}
	return pt;
}

struct HsOut
{
	float3 pos : POSITION;
};

[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("constHS")]
[maxtessfactor(64.f)]
HsOut hs(InputPatch<VsOut, 4> patch, uint i : SV_OutputControlPointID)
{
	HsOut hs_out;
	hs_out.pos = patch[i].pos;
	return hs_out;
}

struct DomainOut
{
	float4 pos_h : SV_Position;
	float3 pos_world : POSITION;
	float3 normal_world : NORMAL;
	float2 uv : TEXCOORD;
};

[domain("quad")]
DomainOut ds (PatchTess pt, float2 uv : SV_DomainLocation, const OutputPatch<HsOut, 4> quad)
{
	DomainOut ds_out;
	ds_out.pos_world = lerp(lerp(quad[0].pos, quad[1].pos, uv.x), lerp(quad[2].pos, quad[3].pos, uv.x), 1.f - uv.y);
	ds_out.pos_world.y = getHeight(ds_out.pos_world.xz);
	ds_out.pos_h = mul(MVP, float4(ds_out.pos_world, 1.f));
	float height_ddx = ds_out.pos_world.y - getHeight(ds_out.pos_world.xz + float2(1.f, 0.f));
	float height_ddy = ds_out.pos_world.y - getHeight(ds_out.pos_world.xz + float2(0.f, 1.f));
	ds_out.normal_world = normalize(float3(height_ddx, 1.f, height_ddy));
	ds_out.uv = float2(uv.x, 1.f - uv.y) * uvScaleDetails;
	/*ds_out.uv0 = ds_out.pos_world.yz * uvScaleDetails;
	ds_out.uv1 = ds_out.pos_world.xz * uvScaleDetails;
	ds_out.uv2 = ds_out.pos_world.xy * uvScaleDetails;*/
	return ds_out;
}

float4 ps(DomainOut pin) : SV_Target
{
	float ambient = 0.01f;
	float3 l = normalize(lightPosWorld - pin.pos_world);
	float3 n = pin.normal_world;
	float3 tangent = normalize(float3(-n.y, n.x, 0.f));
	float3 bitangent = cross(n, tangent);
	float3x3 TBN = float3x3(tangent, bitangent, n);
	//float3 weights = n * n;
	//float3 normal_ts = weights.x * terrainNormals.Sample(samplerAni, pin.uv0).xyz + weights.y * terrainNormals.Sample(samplerAni, pin.uv1).xyz + weights.z * terrainNormals.Sample(samplerAni, pin.uv2).xyz;
	//n = normalize(mul(TBN, normal_ts * 2.f - 1.f));
	n = normalize(mul(TBN, terrainNormals.Sample(samplerAni, pin.uv).xyz * 2.f - 1.f));
	float3 e = normalize(camPosWorld - pin.pos_world);
	float3 r = reflect(-l, n);
	float diff = saturate(dot(n, l));
	//float spec = saturate(dot(e, r));
	//spec = pow(abs(spec), 64.f);
	float3 dirt_color = float3(0.309f, 0.168f, 0.07f);
	float3 snow_color = float3(5.f, 5.f, 5.f);
	float3 grass_color = float3(0.070f, 0.278f, 0.023f);
	float steepness_high = 1.f - smoothstep(0.f, 1.f, pin.normal_world.y);
	float steepness_low = pow(abs(1.f - smoothstep(0.f, 1.f, pin.normal_world.y)), 0.2f);
	float snow_level = 200.f;
	float3 albedo_low = lerp(grass_color, dirt_color, steepness_low);
	float3 albedo_high = lerp(dirt_color, snow_color, steepness_high);
	float3 albedo = lerp(albedo_low, albedo_high, smoothstep(snow_level, snow_level * 1.7f, pin.pos_world.y));
	//albedo = terrainNormals.Sample(samplerAni, pin.uv);
	return float4((diff + ambient) * albedo, 1.f);
}

float4 psWireframe(DomainOut pin) : SV_Target
{
	return float4(1.f, 1.f, 1.f, 1.f);
}

technique11 terrainTechWireframe
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetHullShader(CompileShader(hs_5_0, hs()));
		SetDomainShader(CompileShader(ds_5_0, ds()));
		SetPixelShader(CompileShader(ps_5_0, psWireframe()));
	}
}

technique11 terrainTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vs()));
		SetHullShader(CompileShader(hs_5_0, hs()));
		SetDomainShader(CompileShader(ds_5_0, ds()));
		SetPixelShader(CompileShader(ps_5_0, ps()));
	}
}
