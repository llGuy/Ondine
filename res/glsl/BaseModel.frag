#version 450

layout (location = 0) in VS_DATA {
  // World space
  vec4 wPosition;
  vec4 wNormal;
} inFS;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outPosition;

void main() {
  outAlbedo = vec4(1.0);
  outNormal = vec4(normalize(inFS.wNormal.xyz), 1.0);
  // Stuff is stored in Km
  outPosition = inFS.wPosition;
  outPosition.a = 1.0;
}
