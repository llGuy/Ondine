#ifndef UTILS_GLSL
#define UTILS_GLSL

float calculateFragDepth(
  in float far, in float near,
  in mat4 viewProjectionMatrix,
  // World space
  in vec3 wPosition) {
  // Clip space
  vec4 cPosition = viewProjectionMatrix * vec4(wPosition, 1.0);
  float ndcDepth = (cPosition.z / cPosition.w) * 0.5 + 0.5;
  return ndcDepth;

  // return ((ndcDepth) + 1.0) / 2.0;
}

vec3 getVPositionFromDepth(
  vec2 ndc, float depth,
  in mat4 inverseProjection) {
  vec4 temp = inverseProjection * vec4(ndc, depth, 1.0);
  vec3 vPos = temp.xyz / temp.w;
  return vPos;
}

float getPerceivedBrightness(vec3 color) {
  return (0.21 * color.r) + (0.72 * color.g) + (0.07 * color.b);
}

const int TOON_SHADING_INTENSITY_LEVEL_COUNT = 5;

float toonShadingIncidentIntensity(float incidentIntensity) {
  incidentIntensity *= float(TOON_SHADING_INTENSITY_LEVEL_COUNT);
  return floor(incidentIntensity) / float(TOON_SHADING_INTENSITY_LEVEL_COUNT);
}

vec3 readNormalFromMap(vec2 uvs, sampler2D normalMap) {
  vec3 rgb = texture(normalMap, uvs).rgb;
  vec3 normal = vec3(rgb.r * 2.0 - 1.0, rgb.b, rgb.g * 2.0 - 1.0);
  return normalize(normal);
}

vec3 nanSafeNormalize(vec3 v3) {
  float len = length(v3);

  return (len == 0.0) ? vec3(0.0) : v3 / len;
}

#endif
