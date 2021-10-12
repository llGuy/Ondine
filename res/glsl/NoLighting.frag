#version 450

struct GBufferData {
  vec4 wNormal;
  float depth;
  vec4 wPosition;
  vec4 albedo;
};

layout (location = 0) in vec3 inViewRay;
layout (location = 1) in vec2 inUVs;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uAlbedo;
layout (set = 0, binding = 1) uniform sampler2D uNormal;
layout (set = 0, binding = 2) uniform sampler2D uPosition;
layout (set = 0, binding = 3) uniform sampler2D uDepth;

void main() {
  GBufferData gbuffer = GBufferData(
    texture(uNormal, inUVs),
    texture(uDepth, inUVs).r,
    texture(uPosition, inUVs),
    texture(uAlbedo, inUVs));

  outColor = gbuffer.albedo;
}
