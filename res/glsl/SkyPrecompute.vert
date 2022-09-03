#version 450

#extension GL_ARB_shader_viewport_layer_array : require

layout(push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

const vec2 POSITIONS[] = vec2[](
  vec2(-1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, -1.0),
  vec2(1.0, 1.0)
);

void main() {
  gl_Layer = uPushConstant.layer;
  gl_Position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);
}
