#ifndef UTILS_GLSL
#define UTILS_GLSL

float calculateFragDepth(
  in float far, in float near,
  in mat4 viewProjectionMatrix,
  // World space
  in vec3 wPosition) {
  // Clip space
  vec4 cPosition = viewProjectionMatrix * vec4(wPosition, 1.0);
  float ndcDepth = cPosition.z / cPosition.w;
  return ndcDepth;

  // return (((1.0 - 0.0) * ndcDepth) + 0.0 + 1.0) / 2.0;
}

#endif
