#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 vWorldPosition;
layout(location = 1) in vec3 vColor;
layout(location = 2) in vec2 vTexCoords;
layout(location = 3) in vec3 vNormal;

layout(location = 0) out vec4 outColor;

/*layout(set = 0, binding = 0) uniform GlobalData {
    vec3 uCameraPosition;
};

layout(set = 1, binding = 1) uniform ColorUniform {
    vec4 color;
};
layout(binding = 2) uniform sampler2D uTexture0;*/


#define PI 3.1415926535897932384626433832795

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


vec3 SpecularContribution(vec3 base_color, vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness)
{
	// Precalculate vectors and dot products	
	vec3 H = normalize (V + L);
	float dotNH = clamp(dot(N, H), 0.0, 1.0);
	float dotNV = clamp(dot(N, V), 0.0, 1.0);
	float dotNL = clamp(dot(N, L), 0.0, 1.0);

	// Light color fixed
	vec3 light_color = vec3(300.0);

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


void main() {
    vec3 camera_pos = vec3(0.0f, 2.5f, -5.0f);
    vec3 light_pos = vec3(-1.0, 1.0, -1.0);

	vec3 base_color = vec3(1.0);
    float metallic = 0.0;
	float roughness = 1.0;
	float ambient_occlusion = 1.0f;

    vec3 N = normalize(vNormal);
	vec3 V = normalize(camera_pos - vWorldPosition);
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
		vec3 L = normalize(light_pos - vWorldPosition);
		Lo = SpecularContribution(base_color, L, V, N, F0, metallic, roughness);
	}

	// IBL.
	//vec2 brdf = texture(samplerBRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	//vec3 reflection = prefilteredReflection(R, roughness).rgb;	
	//vec3 irradiance = texture(samplerIrradiance, N).rgb;
	
	// Diffuse based on irradiance.
	float irradiance = 0.2; // temp
	vec3 diffuse = irradiance * base_color;	

	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
	
	// Specular reflectance.
	//vec3 specular = reflection * (F * brdf.x + brdf.y);;

	// Ambient part
	vec3 kD = 1.0 - F;
	kD *= 1.0 - metallic;
	vec3 ambient = (kD * diffuse /*+ specular*/) * ambient_occlusion;
	vec3 color = ambient + Lo;
	
	 // HDR tonemapping
    color = color / (color + vec3(1.0));
	// Gamma correction
	//color = pow(color, vec3(1.0f / vec3(1.0/2.2)));

    outColor = vec4(color, 1.0);
}
