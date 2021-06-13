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

#endif
