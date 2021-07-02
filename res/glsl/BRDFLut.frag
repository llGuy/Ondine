#version 450

layout (location = 0) in vec2 inUVs;
layout (location = 0) out vec2 outColor;

const float PI = 3.14159265359f;

float geometryGGX(float ndotv, float roughness) {
  float a = roughness;
  float k = (a * a) / 2.0;

  float nom = ndotv;
  float denom = ndotv * (1.0 - k) + k;

  return nom / denom;
}

float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness) {
  float ndotv = max(dot(n, v), 0.0);
  float ndotl = max(dot(n, l), 0.0);
  float ggx2 = geometryGGX(ndotv, roughness);
  float ggx1 = geometryGGX(ndotl, roughness);

  return ggx1 * ggx2;
}

vec3 importanceSample(vec2 xi, vec3 n, float roughness) {
  float a = roughness * roughness;
        
  float phi = 2.0 * PI * xi.x;
  float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        
  vec3 h;
  h.x = cos(phi) * sinTheta;
  h.y = sin(phi) * sinTheta;
  h.z = cosTheta;
        
  // From tangent-space vector to world-space sample vector
  vec3 up = abs(n.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent   = normalize(cross(up, n));
  vec3 bitangent = cross(n, tangent);
        
  vec3 sampleVec = tangent * h.x + bitangent * h.y + n * h.z;
  return normalize(sampleVec);
}

float radicalInverse(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint n) {
  return vec2(float(i) / float(n), radicalInverse(i));
}  

vec2 integrate(float ndotv, float roughness) {
  vec3 v;
  v.x = sqrt(1.0 - ndotv * ndotv);
  v.y = 0.0;
  v.z = ndotv;

  float a = 0.0;
  float b = 0.0;

  vec3 n = vec3(0.0, 0.0, 1.0);

  const uint SAMPLE_COUNT = 1024u;
    
  for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
    vec2 xi = hammersley(i, SAMPLE_COUNT);
    vec3 h  = importanceSample(xi, n, roughness);
    vec3 l  = normalize(2.0 * dot(v, h) * h - v);

    float ndotl = max(l.z, 0.0);
    float ndoth = max(h.z, 0.0);
    float vdoth = max(dot(v, h), 0.0);

    if(ndotl > 0.0) {
      float g = geometrySmith(n, v, l, roughness);
      float gVis = (g * vdoth) / (ndoth * ndotv);
      float fc = pow(1.0 - vdoth, 5.0);

      a += (1.0 - fc) * gVis;
      b += fc * gVis;
    }
  }

  a /= float(SAMPLE_COUNT);
  b /= float(SAMPLE_COUNT);
  return vec2(a, b);
}

void main()  {
  outColor = integrate(inUVs.x, 1.0 - inUVs.y);
}
