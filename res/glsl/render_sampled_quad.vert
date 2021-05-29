#version 450

layout (location = 0) out vec2 outUVs;

const vec2 POSITIONS[] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, 1.0)
);

void main() {
  gl_Position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);

  outUVs = POSITIONS[gl_VertexIndex] / 2.0 + 0.5;
}
