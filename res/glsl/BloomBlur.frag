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

const float WEIGHTS[4] = float[](
  20.0 / 64.0,
  15.0 / 64.0,
  6.0 / 64.0,
  1.0 / 64.0
);

void main() {
  // outColor = texture(uTexture, inUVs);

  vec2 texOffset = uPushConstant.scale.xy;
  vec3 color = texture(uTexture, inUVs).rgb * WEIGHTS[0];

  if (bool(uPushConstant.horizontal)) {
    for (int i = 1; i < 4; ++i) {
      color += texture(uTexture, inUVs + vec2(texOffset.x * i, 0.0f)).rgb * WEIGHTS[i];
      color += texture(uTexture, inUVs - vec2(texOffset.x * i, 0.0f)).rgb * WEIGHTS[i];
    }
  }
  else {
    for (int i = 1; i < 4; ++i){
      color += texture(uTexture, inUVs + vec2(0.0f, texOffset.y * i)).rgb * WEIGHTS[i];
      color += texture(uTexture, inUVs - vec2(0.0f, texOffset.y * i)).rgb * WEIGHTS[i];
    }
  }

  outColor = vec4(color.rgb, 1.0);
}
