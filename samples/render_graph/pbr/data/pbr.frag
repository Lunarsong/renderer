#version 450
#extension GL_ARB_separate_shader_objects : enable

#define SHADOW_MAP_CASCADE_COUNT 4

layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec4 vViewPos;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform MaterialTextures {
	float uHasBaseColorTexture;
	float uHasMetallicRoughnessTexture;
	float uHasNormalTexture;
	float uHasOcclusionTexture;
	float uHasEmissiveTexture;
};

layout(set = 2, binding = 0) uniform GlobalData {
		mat4 uCascadeViewProjMatrices[4];
		vec4 uCascadeSplits;
    vec3 uCameraPosition;
		vec3 uLightDirection;
};

layout(set = 2, binding = 1) uniform samplerCube uIrradianceMap;
layout(set = 2, binding = 2) uniform samplerCube uPrefilteredMap;
layout(set = 2, binding = 3) uniform sampler2D uBrdfLookupTable;
// Shadow map.
layout (set = 2, binding = 4) uniform sampler2DArrayShadow uShadowMapSampler;

layout(set = 1, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D uNormalMap;
layout(set = 1, binding = 2) uniform sampler2D uAmbientOcclusionMap;
layout(set = 1, binding = 3) uniform sampler2D uMetallicRoughnessMap;
layout(set = 1, binding = 4) uniform sampler2D uEmissiveMap;

layout(set = 1, binding = 5) uniform MaterialData {
		vec4 uBaseColor;
		vec2 uMetallicRoughness;
		float uAmbientOcclusion;
};

#define PI 3.1415926535897932384626433832795

vec3 SRGBToLinear(vec3 srgb) {
	return pow(srgb, vec3(2.2));
}

vec4 SRGBToLinear(vec4 srgb) {
	return vec4(pow(srgb.rgb, vec3(2.2)), srgb.a);
}

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
vec3 GetNormalFromMap()
{
	if (uHasNormalTexture == 1.0f) {
    vec3 tangent_normal = texture(uNormalMap, vTexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(vWorldPosition);
    vec3 Q2  = dFdy(vWorldPosition);
    vec2 st1 = dFdx(vTexCoords);
    vec2 st2 = dFdy(vTexCoords);

    vec3 N   = normalize(vNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangent_normal);
	}
	return normalize(vNormal);
}
// ----------------------------------------------------------------------------

// Normal Distribution function --------------------------------------
float DistributionGGX(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return alpha2 / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float GeometrySmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 PrefilteredReflection(vec3 R, float roughness)
{
	const float MAX_REFLECTION_LOD = 9.0;
	float lod = roughness * MAX_REFLECTION_LOD;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	vec3 a = textureLod(uPrefilteredMap, R, lodf).rgb;
	vec3 b = textureLod(uPrefilteredMap, R, lodc).rgb;
	return mix(a, b, lod - lodf);
}


vec3 SpecularContribution(vec3 base_color, vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness)
{
	// Precalculate vectors and dot products
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	// Light color fixed
	vec3 light_color = vec3(10.0);

	vec3 color = vec3(0.0);

	if (dotNL > 0.0) {
		// D = Normal distribution (Distribution of the microfacets)
		float D = DistributionGGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
		float G = GeometrySmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
		vec3 F = FresnelSchlick(dotNV, F0);
		vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);
		// For energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - F.
		// Multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
		color += (kD * base_color / PI + spec) * light_color * dotNL;
	}
	return color;
}

float TextureProj(vec4 shadow_coord, vec2 off, uint cascade_index) {
	float shadow_epsilon = 0.005;
	return texture(uShadowMapSampler, vec4(shadow_coord.st + off, cascade_index, shadow_coord.z + shadow_epsilon)).r;
}

float FilterPCF(vec4 sc, uint cascade_index) {
	ivec3 texture_size = textureSize(uShadowMapSampler, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texture_size.x);
	float dy = scale * 1.0 / float(texture_size.y);

	float shadow_factor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++) {
		for (int y = -range; y <= range; y++) {
			shadow_factor += TextureProj(sc, vec2(dx*x, dy*y), cascade_index);
			count++;
		}
	
	}
	return shadow_factor / count;
}

// The Witness' Optimized PCF, from MJP's example: https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/
// More info: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
#define kFilterSize 7
#define kUsePlaneDepthBias
vec2 ComputeReceiverPlaneDepthBias(vec3 texCoordDX, vec3 texCoordDY) {
    vec2 biasUV;
    biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
    biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
    biasUV *= 1.0f / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));
    return biasUV;
}

//-------------------------------------------------------------------------------------------------
// Helper function for SampleShadowMapOptimizedPCF
//-------------------------------------------------------------------------------------------------
float SampleShadowMap(vec2 base_uv, float u, float v, vec2 inverse_shadow_map_size,
											float light_depth, vec2 receiver_plane_depth_bias, uint cascade_index) {
    vec2 uv = base_uv + vec2(u, v) * inverse_shadow_map_size;

#ifdef kUsePlaneDepthBias
        light_depth = light_depth + dot(vec2(u, v) * inverse_shadow_map_size, receiver_plane_depth_bias);
#endif

    return texture(uShadowMapSampler, vec4(uv, cascade_index, light_depth)).r;
}

//-------------------------------------------------------------------------------------------------
// The method used in The Witness
//-------------------------------------------------------------------------------------------------
float SampleShadowMapOptimizedPCF(vec3 space_pos, vec3 space_pos_dx, vec3 space_pos_dy, uint cascade_index) {
    vec2 shadow_map_size = textureSize(uShadowMapSampler, 0).xy;
    float light_depth = space_pos.z;

    const float kShadowBias = 0.005;

#ifdef kUsePlaneDepthBias
		vec2 texel_size = 1.0f / shadow_map_size;

		vec2 receiver_plane_depth_bias = ComputeReceiverPlaneDepthBias(space_pos_dx, space_pos_dy);

		// Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
		float fractional_sampling_error = 2 * dot(vec2(1.0f, 1.0f) * texel_size, abs(receiver_plane_depth_bias));
		light_depth -= min(fractional_sampling_error, 0.01f);
#else
		vec2 receiver_plane_depth_bias;
		light_depth += kShadowBias;
#endif

    vec2 uv = space_pos.xy * shadow_map_size; // 1 unit - 1 texel

    vec2 inverse_shadow_map_size = 1.0 / shadow_map_size;

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    float s = (uv.x + 0.5 - base_uv.x);
    float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5, 0.5);
    base_uv *= inverse_shadow_map_size;

    float sum = 0;

#if kFilterSize == 2
		return texture(uShadowMapSampler, vec4(space_pos.xy, cascade_index, light_depth)).r;
#elif kFilterSize == 3

		float uw0 = (3 - 2 * s);
		float uw1 = (1 + 2 * s);

		float u0 = (2 - s) / uw0 - 1;
		float u1 = s / uw1 + 1;

		float vw0 = (3 - 2 * t);
		float vw1 = (1 + 2 * t);

		float v0 = (2 - t) / vw0 - 1;
		float v1 = t / vw1 + 1;

		sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		return sum * 1.0f / 16;

#elif kFilterSize == 5

		float uw0 = (4 - 3 * s);
		float uw1 = 7;
		float uw2 = (1 + 3 * s);

		float u0 = (3 - 2 * s) / uw0 - 2;
		float u1 = (3 + s) / uw1;
		float u2 = s / uw2 + 2;

		float vw0 = (4 - 3 * t);
		float vw1 = 7;
		float vw2 = (1 + 3 * t);

		float v0 = (3 - 2 * t) / vw0 - 2;
		float v1 = (3 + t) / vw1;
		float v2 = t / vw2 + 2;

		sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		return sum * 1.0f / 144;

#else // kFilterSize == 7

		float uw0 = (5 * s - 6);
		float uw1 = (11 * s - 28);
		float uw2 = -(11 * s + 17);
		float uw3 = -(5 * s + 1);

		float u0 = (4 * s - 5) / uw0 - 3;
		float u1 = (4 * s - 16) / uw1 - 1;
		float u2 = -(7 * s + 5) / uw2 + 1;
		float u3 = -s / uw3 + 3;

		float vw0 = (5 * t - 6);
		float vw1 = (11 * t - 28);
		float vw2 = -(11 * t + 17);
		float vw3 = -(5 * t + 1);

		float v0 = (4 * t - 5) / vw0 - 3;
		float v1 = (4 * t - 16) / vw1 - 1;
		float v2 = -(7 * t + 5) / vw2 + 1;
		float v3 = -t / vw3 + 3;

		sum += uw0 * vw0 * SampleShadowMap(base_uv, u0, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw0 * SampleShadowMap(base_uv, u1, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw0 * SampleShadowMap(base_uv, u2, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw3 * vw0 * SampleShadowMap(base_uv, u3, v0, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		sum += uw0 * vw1 * SampleShadowMap(base_uv, u0, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw1 * SampleShadowMap(base_uv, u1, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw1 * SampleShadowMap(base_uv, u2, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw3 * vw1 * SampleShadowMap(base_uv, u3, v1, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		sum += uw0 * vw2 * SampleShadowMap(base_uv, u0, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw2 * SampleShadowMap(base_uv, u1, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw2 * SampleShadowMap(base_uv, u2, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw3 * vw2 * SampleShadowMap(base_uv, u3, v2, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		sum += uw0 * vw3 * SampleShadowMap(base_uv, u0, v3, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw1 * vw3 * SampleShadowMap(base_uv, u1, v3, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw2 * vw3 * SampleShadowMap(base_uv, u2, v3, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);
		sum += uw3 * vw3 * SampleShadowMap(base_uv, u3, v3, inverse_shadow_map_size, light_depth, receiver_plane_depth_bias, cascade_index);

		return sum * 1.0f / 2704;

#endif
}

uint GetCascadeIndex() {
	// Get cascade index for the current fragment's view position
	uint cascade_index = 0;
	for(uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; ++i) {
		if(vViewPos.z > uCascadeSplits[i]) {	
			cascade_index = i + 1;
		}
	}
	return cascade_index;
}

float CalculateShadowTerm(uint cascade_index) {
	// Depth compare for shadowing.
	vec4 shadow_coord = (uCascadeViewProjMatrices[cascade_index]) * vec4(vWorldPosition, 1.0);	
	shadow_coord = shadow_coord / shadow_coord.w;
	
	//return FilterPCF(shadow_coord, cascade_index);

	vec3 space_pos_dx = dFdx(shadow_coord.xyz);
	vec3 space_pos_dy = dFdy(shadow_coord.xyz);
	return SampleShadowMapOptimizedPCF(shadow_coord.xyz, space_pos_dx, space_pos_dy, cascade_index);
}

void main() {
	vec3 base_color = uBaseColor.rgb;
	if (uHasBaseColorTexture == 1.0f) {
		base_color *= SRGBToLinear(texture(uAlbedoMap, vTexCoords).rgb);
	}

	vec3 N = GetNormalFromMap();
	vec3 V = normalize(uCameraPosition - vWorldPosition);
	vec3 R = -normalize(reflect(V, N));

	float metallic = uMetallicRoughness.x;
	float roughness = uMetallicRoughness.y;
	if (uHasMetallicRoughnessTexture == 1.0f) {
		vec3 metallic_roughness_texture = texture(uMetallicRoughnessMap, vTexCoords).rgb;
		metallic *= metallic_roughness_texture.b;
		roughness *= clamp(metallic_roughness_texture.g, 0.04, 1.0);
	}

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow).
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, base_color, metallic);

	vec3 Lo = vec3(0.0);
	uint cascade_index = GetCascadeIndex();
	// Per light:
	{
		// Calculate per-light radiance
		vec3 direction_to_light = -uLightDirection; // For point light use normalize(light_pos - vWorldPosition);
		Lo = SpecularContribution(base_color, direction_to_light, V, N, F0, metallic, roughness);

		// Gather if this fragment is visible from the light's perspective.
		float shadow_map_term = CalculateShadowTerm(cascade_index);
		Lo *= shadow_map_term;
	}

	// IBL.
	vec2 brdf = texture(uBrdfLookupTable, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = PrefilteredReflection(R, roughness).rgb;
	vec3 irradiance = texture(uIrradianceMap, N).rgb;

	// Diffuse based on irradiance.
	vec3 diffuse = irradiance * base_color;

	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance.
	vec3 specular = reflection * (F * brdf.x + brdf.y);

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;
	vec3 ambient = (kD * diffuse + specular) * uAmbientOcclusion;
	if (uHasOcclusionTexture == 1.0f) {
		ambient *= texture(uAmbientOcclusionMap, vTexCoords).r;
	}
	vec3 color = ambient + Lo;

	outColor = vec4(color, 1.0);
	if (uHasEmissiveTexture == 1.0f) {
		outColor.rgb += SRGBToLinear(texture(uEmissiveMap, vTexCoords).rgb);
	}

#ifdef VISUALIZE_CASCADES
	const vec3 CascadeColors[SHADOW_MAP_CASCADE_COUNT] = {
			vec3(1.0f, 0.0, 0.0f),
			vec3(0.0f, 1.0f, 0.0f),
			vec3(0.0f, 0.0f, 1.0f),
			vec3(1.0f, 1.0f, 0.0f)
	};
	outColor += vec4(CascadeColors[cascade_index] * 0.1, 1.0);
#endif // VISUALIZE_CASCADES
}
