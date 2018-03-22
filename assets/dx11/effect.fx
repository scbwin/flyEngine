cbuffer cbModel
{
	float4x4 MVInvTranspose;
	float4x4 MVP;
	float4x4 M;
	float3 diffuseColor;
};

cbuffer cbConstant
{
	float4x4 P;
	float4x4 PInverse;
	float3 depthOfFieldDistances;
	float3 skyColor;
	float brightScale;
	float brightBias;
	float exposure;
  int ssrSteps;
  float ssrRayLenScale;
  float ssrMinRayLen;
};

cbuffer cbFrame
{
	float4x4 V;
	float4x4 VInverse;
	float4x4 VP;
	float4x4 VPBefore;
	float4x4 VPInverse;
	float4x4 VPInverseVPBefore;
	float4x4 lightVPs [4];
	float cascadeDistances [4];
	float4x4 lightMVP;
	float3 lightPosView;
	float3 lightColor;
	float time;
	float motionBlurStrength;
	float3 camPosWorld;
	int numCascades;
};

cbuffer cbShadowMap
{
	int cascadeIndex;
}

cbuffer cbMesh
{
	float windPivotMaxY;
	float windPivotMinY;
	float windStrength;
	float windFrequency;
};

cbuffer cbQuad
{
	float2 quadScale;
	float2 quadPos;
};

cbuffer cbFilters
{
	float2 texelSize;
	float minMaxArraySlice;
};

cbuffer cbParticles
{
	float4x4 MVPs [1024];
};

cbuffer cbBillboards
{
	float3 camUpWorld;
	float3 camRightWorld;
	float3 billboardPosWorld [1024];
	float2 billboardSize;
	float fades[1024];
};

struct VertexIn
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tan : TANGENT;
	float3 bitan : BITANGENT;
	uint instance_id : SV_InstanceID;
};

struct VertexOut
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos_view : POSITION;
	float3 normal_view : NORMAL;
	float3 tan_view : TANGENT;
	float3 bitan_view : BITANGENT;
};

Texture2D diffuseTexture;
Texture2D alphaTexture;
Texture2D normalMap;
Texture2D lightingTexture;
Texture2D depthTexture;
Texture2DArray shadowMap;
Texture2D textureToBlur;
Texture2D textureToCopy;
Texture2D lensflareTexture;
Texture2D dofTexture;
Texture2D vsNormalsTexture;
Texture2D vsZTexture;
Texture2DArray minMaxTexture;
Texture2D minMaxTexture2;

SamplerState samplerAni
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 16;
	AddressU = Wrap;
	AddressV = Wrap;
};

SamplerState samplerLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
};

SamplerState samplerPointClamp
{
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};

SamplerState samplerLinearClamp
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

SamplerComparisonState lightSampler
{
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	AddressW = Border;
	BorderColor = float4(1.f, 1.f, 1.f, 1.f);
	ComparisonFunc = LESS_EQUAL;
};

SamplerComparisonState shadowSampler
{
	Filter = COMPARISON_MIN_MAG_MIP_LINEAR;
	AddressU = Border;
	AddressV = Border;
	AddressW = Border;
	BorderColor = float4(1.f, 1.f, 1.f, 1.f);
	ComparisonFunc = GREATER_EQUAL;
};

float hash(float2 p) {
	return frac(sin(dot(p, float2(12.9898f, 78.233f))) * 43758.5453f);
}

float noise(float2 p)
{
	float2 start = floor(p);
	float2 end = start + 1.f;
	float2 weights = smoothstep(start, end, p);
	return lerp(lerp(hash(start), hash(float2(end.x, start.y)), weights.x), lerp(hash(float2(start.x, end.y)), hash(end), weights.x), weights.y);
}

float3 computeWind(uniform bool wind_x, uniform bool wind_z, float3 pos_model)
{
	float wind_weight = pow(1.f - smoothstep(windPivotMinY, windPivotMaxY, pos_model.y), 2.f);
	if (wind_x) {
		float n = noise(pos_model.yz * windFrequency + time) * 2.f - 1.f;
		return float3(pos_model.x + n * wind_weight * windStrength, pos_model.yz);
	}
	else if (wind_z) {
		float n = noise(pos_model.xy * windFrequency + time) * 2.f - 1.f;
		return float3(pos_model.xy, pos_model.z + n * wind_weight * windStrength);
	}
	else {
		return pos_model;
	}
}

struct VertexOutShadowMap
{
	float4 pos_world : POSITION;
	float2 uv : TEXCOORD;
};

struct GSOutShadowMap
{
	float4 pos_h : SV_POSITION;
	float2 uv : TEXCOORD;
	uint slice : SV_RenderTargetArrayIndex;
};

void vsShadowMap(VertexIn vin, uniform bool use_wind_x, uniform bool use_wind_z, out VertexOutShadowMap vout)
{
	vout.pos_world = mul(M, float4(computeWind(use_wind_x, use_wind_z, vin.pos), 1.f));
	vout.uv = vin.uv;
}

[maxvertexcount(12)]
void gsShadowMap(triangle VertexOutShadowMap gs_in [3], inout TriangleStream<GSOutShadowMap> tri_stream)
{
	for (int i = 0; i < numCascades; i++) {
		for (int j = 0; j < 3; j++) {
			GSOutShadowMap gs_out;
			gs_out.pos_h = mul(lightVPs[i], gs_in[j].pos_world);
			gs_out.uv = gs_in[j].uv;
			gs_out.slice = i;
			tri_stream.Append(gs_out);
		}
		tri_stream.RestartStrip();
	}
}

void psShadowMap(GSOutShadowMap pin, uniform bool useAlpha)
{
	if (useAlpha) {
		clip(alphaTexture.Sample(samplerLinear, pin.uv).r - 0.5f);
	}
}

void vertexShader(VertexIn vin, uniform bool use_wind_x, uniform bool use_wind_z, out VertexOut vout)
{	
	vout.pos = mul(MVP, float4(computeWind(use_wind_x, use_wind_z, vin.pos), 1.f));
	vout.uv = vin.uv;
	float4 pos_view_h = mul(PInverse, vout.pos);
	vout.pos_view = pos_view_h.xyz / pos_view_h.w;
	vout.normal_view = normalize(mul(MVInvTranspose, float4(vin.normal, 0.f)).xyz);
	vout.tan_view = normalize(mul(MVInvTranspose, float4(vin.tan, 0.f)).xyz);
	vout.bitan_view = normalize(mul(MVInvTranspose, float4(vin.bitan, 0.f)).xyz);
}

float computeShadow(float3 pos_world, int cascade_index)
{
	float4 shadow_coord = mul(lightVPs[cascade_index], float4(pos_world, 1.f));
	shadow_coord.xyz /= shadow_coord.w;
	shadow_coord.x = shadow_coord.x * 0.5f + 0.5f;
	shadow_coord.y = -shadow_coord.y * 0.5f + 0.5f;
  bool valid = shadow_coord.x >= 0.f && shadow_coord.x <= 1.f && shadow_coord.y >= 0.f && shadow_coord.y <= 1.f && shadow_coord.z >= 0.f && shadow_coord.z <= 1.f;
  if (valid)
    return shadowMap.SampleCmpLevelZero(shadowSampler, float3(shadow_coord.xy, cascade_index), shadow_coord.z).r;
  else
    return 0.f;
}

struct PSOut
{
	float4 color : SV_TARGET0;
	float3 cs_normal : SV_TARGET1;
	float cs_z : SV_TARGET2;
};

PSOut pixelShader(VertexOut pin, uniform bool useDiffuseTexture, uniform bool useAlpha, uniform bool useNormal)
{
	if (useAlpha) {
		clip(alphaTexture.Sample(samplerLinear, pin.uv).r - 0.5f);
	}
	float3 albedo;
	if (useDiffuseTexture) {
		albedo = diffuseTexture.Sample(samplerAni, pin.uv).xyz;
	}
	else {
		albedo = diffuseColor;
	}
	
	float3 n = normalize(pin.normal_view);
	if (useNormal) {
		float3x3 TBN = float3x3(normalize(pin.tan_view), normalize(pin.bitan_view), n);
		float3 normal_ts = normalMap.Sample(samplerAni, pin.uv).xyz * 2.f - 1.f;
		n = normalize(mul(normal_ts, TBN));
	}
	
	float3 l = normalize(lightPosView - pin.pos_view);
	float3 e = normalize(- pin.pos_view);
	float3 r = normalize(reflect(-l, n));
	float diff = saturate(dot(n, l));
	float spec = pow(saturate(dot(e, r)), 16.f);
	float amb = 0.01f;
	
	float cam_dist = length(pin.pos_view);
	float fade_start = cascadeDistances[1] * 1.5f;
	float3 pos_world = mul(VInverse, float4(pin.pos_view, 1.f)).xyz;
	float shadow0 = computeShadow(pos_world, 0);
	float shadow1 = computeShadow(pos_world, 1);
	float shadow = lerp(shadow0, shadow1, smoothstep(cascadeDistances[0] * 0.9f, cascadeDistances[0], cam_dist));
  float visibility = 1.f - 0.7f *
    shadow;// * (1.f - smoothstep(fade_start, fade_start + 1.f, cam_dist));
	
	PSOut ps_out;
	ps_out.color = float4(albedo * (diff + spec + amb) * visibility, 1.f);
	ps_out.cs_normal = n;
	ps_out.cs_z = pin.pos_view.z;
	return ps_out;
}

struct VSOutQuad
{
	float2 uv : TEX_COORD;
	float4 pos : SV_POSITION;
};

void vsFullScreenQuad(uint vertex_id : SV_VERTEXID, out VSOutQuad vout) 
{
	float2 pos = float2(vertex_id & 1, vertex_id >> 1);
	vout.pos = float4(pos * 2.f - 1.f, 0.f, 1.f);
	vout.uv = float2(pos.x, 1.f - pos.y);
}

void vsQuad(uint vertex_id : SV_VERTEXID, out VSOutQuad vout) 
{
	float2 pos = float2(vertex_id & 1, vertex_id >> 1);
	vout.pos = float4((pos * 2.f - 1.f) * quadScale + quadPos, 1.f, 1.f);
	vout.uv = float2(pos.x, 1.f - pos.y);
}

float4 psLightSource (VSOutQuad pin) : SV_TARGET
{
	float len = length(pin.uv * 2.f - 1.f);
	clip(len > 1.f ? -1.f : 1.f);
	return float4(lightColor, 1.f);
}

float4 vsLightParticle(VertexIn vin) : SV_POSITION
{
	return mul(MVPs[vin.instance_id], float4(vin.pos, 1.f));
}

float4 psLightParticle() : SV_TARGET
{
	return float4(diffuseColor, 1.f);
}

struct VSBillboardOut
{
	float3 pos_world : POSITION;
	float fade : COLOR;
};

VSBillboardOut vsBillboard(uint vertex_id : SV_VertexID)
{
	VSBillboardOut vs_out;
	vs_out.pos_world = billboardPosWorld[vertex_id];
	vs_out.fade = fades[vertex_id];
	return vs_out;
}

struct GSOut
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float fade : COLOR;
};

[maxvertexcount(4)]
void gsBillboard(point VSBillboardOut gs_in [1], inout TriangleStream<GSOut> tri_stream)
{
	GSOut gs_out0;
	gs_out0.uv = float2(0.f, 1.f);
	gs_out0.pos = mul(VP, float4(gs_in[0].pos_world - 0.5f * camRightWorld * billboardSize.x - 0.5f * camUpWorld * billboardSize.y, 1.f));
	gs_out0.fade = gs_in[0].fade;
	tri_stream.Append(gs_out0);
	
	GSOut gs_out1;
	gs_out1.uv = float2(1.f, 1.f);
	gs_out1.pos = mul(VP, float4(gs_in[0].pos_world + 0.5f * camRightWorld * billboardSize.x - 0.5f * camUpWorld * billboardSize.y, 1.f));
	gs_out1.fade = gs_in[0].fade;
	tri_stream.Append(gs_out1);
	
	GSOut gs_out2;
	gs_out2.uv = float2(0.f, 0.f);
	gs_out2.pos = mul(VP, float4(gs_in[0].pos_world - 0.5f * camRightWorld * billboardSize.x + 0.5f * camUpWorld * billboardSize.y, 1.f));
	gs_out2.fade = gs_in[0].fade;
	tri_stream.Append(gs_out2);
	
	GSOut gs_out3;
	gs_out3.uv = float2(1.f, 0.f);
	gs_out3.pos = mul(VP, float4(gs_in[0].pos_world + 0.5f * camRightWorld * billboardSize.x + 0.5f * camUpWorld * billboardSize.y, 1.f));
	gs_out3.fade = gs_in[0].fade;
	tri_stream.Append(gs_out3);
}

float4 psBillboard(GSOut pin) : SV_TARGET
{
	float4 col = diffuseTexture.Sample(samplerLinear, pin.uv);
	return float4(col.rgb, col.a * pin.fade);
}

float hash(float3 p) {
	return frac(sin(dot(p, float3(12.9898f, 78.233f, 45.164f))) * 43758.5453f);
}

static const uint NUM_WEIGHTS = 5;
static const float gaussWeights [NUM_WEIGHTS] = {0.204164f, 0.180174f, 0.123832f, 0.066282f, 0.027631f}; 

float4 psGaussFilter(VSOutQuad pin, uniform bool horizontal) : SV_TARGET
{
	float4 color = textureToBlur.SampleLevel(samplerLinearClamp, pin.uv, 0) * gaussWeights[0];
	[unroll]
	for (uint i = 1; i < NUM_WEIGHTS; i++) {
		if (horizontal) {
			color += textureToBlur.SampleLevel(samplerLinearClamp, pin.uv + float2(i * texelSize.x, 0.f), 0) * gaussWeights[i];
			color += textureToBlur.SampleLevel(samplerLinearClamp, pin.uv - float2(i * texelSize.x, 0.f), 0) * gaussWeights[i];
		}
		else {
			color += textureToBlur.SampleLevel(samplerLinearClamp, pin.uv + float2(0.f, i * texelSize.y), 0) * gaussWeights[i];
			color += textureToBlur.SampleLevel(samplerLinearClamp, pin.uv - float2(0.f, i * texelSize.y), 0) * gaussWeights[i];
		}
	}
	return color;
}

float4 psBrightPass(VSOutQuad pin) : SV_TARGET
{
	float4 color = lightingTexture.SampleLevel(samplerLinearClamp, pin.uv, 0);
	return max(float4(0.f, 0.f, 0.f, 0.f), color * brightScale - brightBias);
}

float4 psCopy(VSOutQuad pin) : SV_TARGET
{
	return textureToCopy.SampleLevel(samplerLinearClamp, pin.uv, 0);
}

float4 psSSR(VSOutQuad pin) : SV_TARGET
{
	float3 normal_view_space = vsNormalsTexture.SampleLevel(samplerPointClamp, pin.uv, 0).xyz;
	float4 pos_view_space_h = mul(PInverse, float4(float2(pin.uv.x, 1.f - pin.uv.y) * 2.f - 1.f, depthTexture.SampleLevel(samplerPointClamp, pin.uv, 0).r, 1.f));
	float3 pos_view_space = pos_view_space_h.xyz / pos_view_space_h.w;
	float3 ray_dir = reflect(normalize(pos_view_space), normal_view_space);
	float ray_len = max(-pos_view_space.z * ssrRayLenScale, ssrMinRayLen);
	float3 ray = ray_dir * ray_len;
  float3 delta = ray / ssrSteps;
	float3 hit_pos_cs = pos_view_space + delta;
	bool is_valid = true;
	[loop]
	for (int i = 0; i < ssrSteps && is_valid; i++, hit_pos_cs += delta) {
		float4 hit_pos_h = mul(P, float4(hit_pos_cs, 1.f));
		float3 hit_pos_ndc = hit_pos_h.xyz / hit_pos_h.w;
		float2 uv = float2(hit_pos_ndc.x * 0.5f + 0.5f, -hit_pos_ndc.y * 0.5f + 0.5f);
		float depth = vsZTexture.SampleLevel(samplerLinearClamp, uv, 0).r;
		is_valid = hit_pos_ndc.x >= -1.f && hit_pos_ndc.x <= 1.f && hit_pos_ndc.y >= -1.f && hit_pos_ndc.y <= 1.f && hit_pos_ndc.z >= 0.f && hit_pos_ndc.z <= 1.f;
		if (hit_pos_cs.z < depth && is_valid) {
			[unroll]
			for (int i = 0; i < 6; i++) { // Binary search refinement
				hit_pos_h = mul(P, float4(hit_pos_cs, 1.f));
				hit_pos_ndc = hit_pos_h.xyz / hit_pos_h.w;
				uv = float2(hit_pos_ndc.x * 0.5f + 0.5f, -hit_pos_ndc.y * 0.5f + 0.5f);
				depth = vsZTexture.SampleLevel(samplerLinearClamp, uv, 0).r;
				delta *= 0.5f;
				[flatten]
				if (hit_pos_cs.z < depth) {
					hit_pos_cs -= delta;
				}
				else {
					hit_pos_cs += delta;
				}
			}
			return lightingTexture.SampleLevel(samplerLinearClamp, uv, 0);
		}
	}
	return lightingTexture.SampleLevel(samplerPointClamp, pin.uv, 0);
}

float2 psMinMax(VSOutQuad pin) : SV_TARGET
{
	float depth0 = minMaxTexture.Sample(samplerPointClamp, float3(pin.uv + float2(0.f, texelSize.y), minMaxArraySlice)).r;
	float depth1 = minMaxTexture.Sample(samplerPointClamp, float3(pin.uv + float2(texelSize.x, 0.f), minMaxArraySlice)).r;
	float depth2 = minMaxTexture.Sample(samplerPointClamp, float3(pin.uv - float2(0.f, texelSize.y), minMaxArraySlice)).r;
	float depth3 = minMaxTexture.Sample(samplerPointClamp, float3(pin.uv - float2(texelSize.x, 0.f), minMaxArraySlice)).r;
	float min_depth = min(depth0, min(depth1, min(depth2, depth3)));
	float max_depth = max(depth0, max(depth1, max(depth2, depth3)));
	return float2(min_depth, max_depth);
}

float2 psMinMax2(VSOutQuad pin) : SV_TARGET
{
	float depth0 = minMaxTexture2.Sample(samplerPointClamp, pin.uv + float2(0.f, texelSize.y)).r;
	float depth1 = minMaxTexture2.Sample(samplerPointClamp, pin.uv + float2(texelSize.x, 0.f)).r;
	float depth2 = minMaxTexture2.Sample(samplerPointClamp, pin.uv - float2(0.f, texelSize.y)).r;
	float depth3 = minMaxTexture2.Sample(samplerPointClamp, pin.uv - float2(texelSize.x, 0.f)).r;
	float min_depth = min(depth0, min(depth1, min(depth2, depth3)));
	float max_depth = max(depth0, max(depth1, max(depth2, depth3)));
	return float2(min_depth, max_depth);
}

float4 psComposite(VSOutQuad pin, uniform bool dof, uniform bool lensflare, uniform bool motion_blur, uniform bool lightVolumes) : SV_TARGET
{
	float3 color = lightingTexture.SampleLevel(samplerPointClamp, pin.uv, 0).rgb;
	float blur_weight;
	float4 pos_ndc;
	if (motion_blur || dof || lightVolumes) {
		pos_ndc = float4(float2(pin.uv.x, 1.f - pin.uv.y) * 2.f - 1.f, depthTexture.SampleLevel(samplerPointClamp, pin.uv, 0).r, 1.f);
	}
	float3 pos_world;
	if (lightVolumes) {
		float4 pos_world_h = mul(VPInverse, pos_ndc);
		pos_world = pos_world_h.xyz / pos_world_h.w;
	}
	if (motion_blur) {
		float4 pos_ndc_prev = mul(VPInverseVPBefore, pos_ndc);
		pos_ndc_prev /= pos_ndc_prev.w;
		pos_ndc_prev = pos_ndc_prev * 0.5f + 0.5f;
		pos_ndc_prev.y = 1.f - pos_ndc_prev.y;
		float2 blur_vec = pin.uv - pos_ndc_prev.xy;
		const float samples = 8.f;
		for (float i = 1.f; i < samples; i++) {
			float2 offset = blur_vec * (i / (samples - 1.f) - 0.5f) * motionBlurStrength;
			color += lightingTexture.SampleLevel(samplerLinearClamp, pos_ndc_prev.xy + offset, 0).rgb;
		}
		color /= samples;
	}

	if (dof) {
		float4 pos_view_space_h = mul(PInverse, pos_ndc);
		float3 pos_view_space = pos_view_space_h.xyz / pos_view_space_h.w;
		[flatten]
		if (-pos_view_space.z >= depthOfFieldDistances.y) {
			blur_weight = smoothstep(depthOfFieldDistances.y, depthOfFieldDistances.z, -pos_view_space.z);
		}
		else {
			blur_weight = smoothstep(depthOfFieldDistances.y, depthOfFieldDistances.x, -pos_view_space.z);
		}
		color = lerp(color, dofTexture.SampleLevel(samplerLinearClamp, pin.uv, 0).rgb, blur_weight);
	}
	
	if (lensflare) {
		color += lensflareTexture.SampleLevel(samplerLinearClamp, pin.uv, 0).rgb;
	}
	if (lightVolumes) {
		//return float4(minMaxTexture2.SampleLevel(samplerPointClamp, pin.uv, 0).rg, 0.f, 1.f);
		float4 pos_world_near_h = mul(VPInverse, float4(pos_ndc.xy, 0.f, 1.f));
		float3 pos_world_near = pos_world_near_h.xyz / pos_world_near_h.w;
		float3 ray = pos_world - pos_world_near;
		const float steps = 128.f;
		float3 delta = ray / steps;
		float3 p = pos_world_near;
		float light_ray = 0.f;
		//float long_step_scale = 4.f;
		for (float i = 0.f; i < steps; i++, p += delta) {
			int cascade_index = 1 - (distance(camPosWorld, p) < cascadeDistances[0]);
			float4 shadow_coord = mul(lightVPs[cascade_index], float4(p, 1.f));
			shadow_coord.x = shadow_coord.x * 0.5f + 0.5f;
			shadow_coord.y = -shadow_coord.y * 0.5f + 0.5f;
			light_ray += shadowMap.SampleCmpLevelZero(lightSampler, float3(shadow_coord.xy, cascade_index), shadow_coord.z).r;
			/*float fine_sample = shadow_coord.z < shadowMap.Sample(samplerPointClamp, float3(shadow_coord.xy, cascade_index)).r;
			float3 step_end = p + delta * long_step_scale;
			float4 shadow_coord_end = mul(lightVPs[cascadeIndex], float4(step_end, 1.f));
			float2 min_max = minMaxTexture2.Sample(samplerPointClamp, shadow_coord.xy).rg;
			float comparison = max(shadow_coord.z, shadow_coord_end.z);
			float is_lit = comparison < min_max.x;
			comparison = min(shadow_coord.z, shadow_coord_end.z);
			float is_shadow = comparison > min_max.y;
			float is_longstep = is_lit + is_shadow;
			float scale_factor = is_longstep * long_step_scale;
			light_ray += fine_sample * (1.f + scale_factor);
			i += scale_factor;
			p += delta * (1.f + scale_factor);*/
		}
		color += light_ray / steps * length(ray) * 0.1f;
	}
	if (dof) {
		color += hash(float3(pin.uv, round(frac(time) * 16.f))) * 0.025f * blur_weight;
	}
	color *= exposure;
	color = color / (1.f + color); // tone mapping
	color = pow(abs(color), float3(1.f / 2.2f, 1.f / 2.2f, 1.f / 2.2f)); // gamma correction
	return float4(color, 1.f);
}

struct VertexOutSkybox
{
	float4 pos_h : SV_Position;
	float3 pos_local : POSITION;
};

VertexOutSkybox vsSkybox(VertexIn vin)
{
	VertexOutSkybox vs_out;
	vs_out.pos_h = mul(MVP, float4(vin.pos, 1.f));
	vs_out.pos_h.z = vs_out.pos_h.w;
	vs_out.pos_local = vin.pos;
	return vs_out;
}

float4 psSkyBox(VertexOutSkybox pin) : SV_Target
{
//	float factor = pow(abs(pin.pos_local.y), 0.5f);
	//float factor = exp(smoothstep(0.f, 0.175f, abs(pin.pos_local.y)));
	float factor = pow(smoothstep(0.f, 0.175f, abs(pin.pos_local.y)), 0.25f);
	return lerp(float4(1.f, 1.f, 1.f, 1.f), float4(skyColor, 1.f), factor);
}

technique11 skyboxTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsSkybox()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSkyBox()));
	}
}

technique11 ssrTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psSSR()));
	}
}

technique11 lightSourceTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psLightSource()));
	}
}

technique11 copyTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psCopy()));
	}
}

technique11 gaussTech
{
	pass passHorizontal
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psGaussFilter(true)));
	}
	
	pass passVertical
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psGaussFilter(false)));
	}
}
technique11 brightPassTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psBrightPass()));
	}
}
technique11 lightParticleTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsLightParticle()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psLightParticle()));
	}
}
technique11 billboardTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsBillboard()));
		SetGeometryShader(CompileShader(gs_5_0, gsBillboard()));
		SetPixelShader(CompileShader(ps_5_0, psBillboard()));
	}
}
technique11 minMaxTech
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psMinMax()));
	}
}
technique11 minMaxTech2
{
	pass pass0
	{
		SetVertexShader(CompileShader(vs_5_0, vsFullScreenQuad()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psMinMax2()));
	}
}