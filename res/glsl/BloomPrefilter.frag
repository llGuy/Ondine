#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uTexture;

layout (push_constant) uniform PushConstant {
  vec4 intensity;
  vec4 scale;
  float threshold;
  bool horizontal;
} uPushConstant;

void main() {
  outColor = texture(uTexture, inUVs);
  float brightness = dot(outColor.rgb, vec3(0.2126, 0.7152, 0.0722));

  if (brightness < 1.0) {
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
  }
}
