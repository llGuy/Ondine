#version 450

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

layout(push_constant) uniform PushConstant {
  int layer;
  int scatteringOrder;
} uPushConstant;

void main() {
  gl_Position = gl_in[0].gl_Position;
  gl_Layer = uPushConstant.layer;
  EmitVertex();

  gl_Position = gl_in[1].gl_Position;
  gl_Layer = uPushConstant.layer;
  EmitVertex();

  gl_Position = gl_in[2].gl_Position;
  gl_Layer = uPushConstant.layer;
  EmitVertex();

  EndPrimitive();
}
