#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D uTexture;

layout (push_constant) uniform PushConstant {
  vec4 white;
  float exposure;
} uPushConstant;

void main() {
  vec3 radiance = texture(uTexture, inUVs).rgb;

  vec3 one = vec3(1.0);
  vec3 expValue =
    exp(-radiance / uPushConstant.white.rgb * uPushConstant.exposure);

  /* Gamma correction */
  vec3 diff = one - expValue;
  vec3 gamma = vec3(1.0 / 2.2);

  outColor.rgb = pow(diff, gamma);
  outColor.a = 1.0;
}
