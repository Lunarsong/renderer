#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in vec3 vNormal;
layout(location = 4) in vec4 vShadowCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform GlobalData {
    vec3 uCameraPosition;
		vec3 uLightDirection;
};

layout(set = 1, binding = 1) uniform samplerCube uIrradianceMap;
layout(set = 1, binding = 2) uniform samplerCube uPrefilteredMap;
layout(set = 1, binding = 3) uniform sampler2D uBrdfLookupTable;
// Shadow map.
layout (set = 1, binding = 4) uniform sampler2D uShadowMapSampler;

layout(set = 2, binding = 0) uniform sampler2D uAlbedoMap;
layout(set = 2, binding = 1) uniform sampler2D uNormal;
layout(set = 2, binding = 2) uniform sampler2D uAmbientOcclusionMap;
layout(set = 2, binding = 3) uniform sampler2D uMetallicRoughnessMap;


#define PI 3.1415926535897932384626433832795

vec3 SRGBToLinear(vec3 srgb) {
	return pow(srgb, vec3(2.2));
}

vec4 SRGBToLinear(vec4 srgb) {
	return vec4(pow(srgb.rgb, vec3(2.2)), srgb.a);
}

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
/*vec3 GetNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}*/
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

float textureProj(vec4 P, vec2 off)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 )
	{
		float dist = texture( uShadowMapSampler, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z )
		{
			shadow = 0.0;
		}
	}
	return shadow;
}

void main() {
	vec3 base_color = SRGBToLinear(vec3(253.0, 181.0, 21.0) / vec3(255.0));
    float metallic = 1.0;
	float roughness = 0.25;
	float ambient_occlusion = 1.0f;

    vec3 N = normalize(vNormal);
	vec3 V = normalize(uCameraPosition - vWorldPosition);
	vec3 R = -normalize(reflect(V, N));

	/*if (material.hasMetallicRoughnessTexture == 1.0f) {
		metallic *= texture(metallicMap, vTexCoords).b;
		roughness *= clamp(texture(metallicMap, vTexCoords).g, 0.04, 1.0);
	}*/

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow).
    vec3 F0 = vec3(0.04);
	F0 = mix(F0, base_color, metallic);

	vec3 Lo = vec3(0.0);
	// Per light:
	{
		// Calculate per-light radiance
		vec3 direction_to_light = -uLightDirection; // normalize(light_pos - vWorldPosition);
		Lo = SpecularContribution(base_color, direction_to_light, V, N, F0, metallic, roughness) * textureProj(vShadowCoord / vShadowCoord.w, vec2(0.0));
	}

	// IBL.
	vec2 brdf = texture(uBrdfLookupTable, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 reflection = PrefilteredReflection(R, roughness).rgb;
	vec3 irradiance = texture(uIrradianceMap, N).rgb;

	// Diffuse based on irradiance.
	vec3 diffuse = irradiance * base_color;

	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	// Specular reflectance.
	vec3 specular = reflection * (F * brdf.x + brdf.y);;

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;
	vec3 ambient = (kD * diffuse + specular) * ambient_occlusion;
	vec3 color = ambient + Lo;

	outColor = vec4(color, 1.0);
}
