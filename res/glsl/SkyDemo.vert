#version 450

layout (push_constant) uniform PushConstant {
  mat4 model_from_view;
  mat4 view_from_clip;
  vec3 camera;
  vec3 white_point;
  vec3 earth_center;
  vec3 sun_direction;
  vec2 sun_size;
  float exposure;
} uPushConstant;

layout (location = 0) out vec3 outViewRay;

const vec4 POSITIONS[] = vec4[](
  vec4(-1.0, -1.0, 0.0, 1.0),
  vec4(-1.0, 1.0, 0.0, 1.0),
  vec4(1.0, -1.0, 0.0, 1.0),
  vec4(1.0, 1.0, 0.0, 1.0)
);

void main() {
  vec4 vertex = POSITIONS[gl_VertexIndex];

  gl_Position = vertex;

  vertex.y *= -1.0;

  outViewRay =
    (uPushConstant.model_from_view *
     vec4((uPushConstant.view_from_clip *
           vertex).xyz, 0.0)).xyz;
}
