#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

float distributionGGX(float ndoth, float roughness) {
  float a2 = roughness * roughness;
  float f = (ndoth * a2 - ndoth) * ndoth + 1.0f;
  return a2 / (PI * f * f);
}

float smithGGX(float ndotv, float ndotl, float roughness) {
  float r = roughness + 1.0;
  float k = (r * r) / 8.0;
  float ggx1 = ndotv / (ndotv * (1.0f - k) + k);
  float ggx2 = ndotl / (ndotl * (1.0f - k) + k);
  return ggx1 * ggx2;
}

vec3 fresnel(float hdotv, vec3 base) {
  return base + (1.0f - base) * pow(1.0f - clamp(hdotv, 0.0f, 1.0f), 5.0f);
}

vec3 fresnelRoughness(float ndotv, vec3 base, float roughness) {
  return base + (max(vec3(1.0f - roughness), base) - base) *
    pow(1.0f - ndotv, 5.0f);
}

struct GBufferData {
  vec4 wNormal;
  float depth;
  vec4 wPosition;
  vec4 albedo;
};

// Later on, add shadow factor (all in world space)
vec3 directionalRadianceBRDF(
  in GBufferData gbuffer,
  in vec3 baseReflectivity,
  in float roughness,
  in float metalness,
  in vec3 viewDirection,
  in vec3 incomingRadiance,
  in vec3 lightDirection) {
  vec3 halfway = normalize(lightDirection + viewDirection);

  float ndotv = max(dot(gbuffer.wNormal.xyz, viewDirection), 0.000001f);
  float ndotl = max(dot(gbuffer.wNormal.xyz, lightDirection), 0.000001f);
  float hdotv = max(dot(halfway, viewDirection), 0.000001f);
  float ndoth = max(dot(gbuffer.wNormal.xyz, halfway), 0.000001f);

  float distributionTerm = distributionGGX(ndoth, roughness);
  float smithTerm = smithGGX(ndotv, ndotl, roughness);
  vec3 fresnelTerm = fresnel(hdotv, baseReflectivity);

  vec3 specular = smithTerm * distributionTerm * fresnelTerm;
  specular /= 4.0 * ndotv * ndotl;

  vec3 kd = vec3(1.0) - fresnelTerm;

  kd *= 1.0f - metalness;

  return (kd * gbuffer.albedo.rgb / PI + specular) * incomingRadiance * ndotl;
}

#endif
