#version 450

layout (location = 0) in float outBrightness;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

void main() {
  outAlbedo = vec4(outBrightness);
  outNormal = vec4(0.0);
  outPosition = vec4(0.0);
}
