#version 450

layout (location = 0) in vec2 vTexCoords;

layout(set = 0, binding = 0) uniform sampler2D uSamplerHDR;

layout(location = 0) out vec4 outColor;

// Filmic Tonemapping, aka that function everyone copies from the same place:
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;
vec3 Uncharted2Tonemap(vec3 x) {
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	// HDR color from the scene.
	vec3 hdr_color = texture(uSamplerHDR, vTexCoords).rgb;

	// Apply tonemapping.
	float exposure_bias = 2.0f;
	vec3 tonemapped = Uncharted2Tonemap(exposure_bias * hdr_color);

	vec3 white_scale = 1.0f / Uncharted2Tonemap(vec3(W));
  vec3 color = tonemapped * white_scale;

	// Apply gamma correction for the dispaly.
	vec3 gamma_corrected = pow(color, vec3(1.0/2.2));

	// Output.
	outColor = vec4(gamma_corrected, 1.0);
}