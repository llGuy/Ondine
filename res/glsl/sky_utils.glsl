#ifndef SKY_UTILS_GLSL
#define SKY_UTILS_GLSL

#include "sky_def.glsl"

/* Some utility functions */
float clamp0To1(float a) {
  return clamp(a, -1.0, 1.0);
}

float clampPositive(float a) {
  return max(a, 0.0);
}

float clampRadius(in SkyProperties sky, float radius) {
  return clamp(radius, sky.bottomRadius, sky.topRadius);
}

float safeSqrt(float a) {
  return sqrt(max(a, 0.0));
}

float getTextureCoordFromUnit(float x, int textureSize) {
  return 0.5 / float(textureSize) + x * (1.0 - 1.0 / float(textureSize));
}

float getUnitFromTextureCoord(float u, int textureSize) {
  return (u - 0.5 / float(textureSize)) / (1.0 - 1.0 / float(textureSize));
}

float distToSkyBoundary(
  in SkyProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.topRadius * sky.topRadius;
  return clampPositive(-r * mu + safeSqrt(delta));
}

float distToGroundBoundary(
  in SkyProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.bottomRadius * sky.bottomRadius;
  return clampPositive(-r * mu - safeSqrt(delta));
}

bool doesRayIntersectGround(
  in SkyProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.bottomRadius * sky.bottomRadius;

  return mu < 0.0 && delta >= 0.0;
}

float getLayerDensity(in DensityLayer layer, float altitude) {
  float density = layer.expTerm * exp(layer.expScale * altitude) +
    layer.linTerm * altitude + layer.constTerm;

  return clamp(density, 0.0, 1.0);
}

float getProfileDensity(in DensityProfile profile, float altitude) {
  return altitude < profile.layers[0].width ?
    getLayerDensity(profile.layers[0], altitude) :
    getLayerDensity(profile.layers[1], altitude);
}

/* Code for transmittance */

float computeOpticalLengthToTopSkyBoundary(
  in SkyProperties sky,
  in DensityProfile profile,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;

  const int SAMPLE_COUNT = 500;

  float dx = distToSkyBoundary(sky, r, mu) / float(SAMPLE_COUNT);

  float result = 0.0;
  for (int i = 0; i <= SAMPLE_COUNT; ++i) {
    float di = float(i) * dx;

    float centreToDi = sqrt(di * di + 2.0 * r * mu * di + r * r);

    float density = getProfileDensity(profile, centreToDi - sky.bottomRadius);

    float weight = ((i == 0) || (i == SAMPLE_COUNT)) ? 0.5 : 1.0;

    result += density * weight * dx;
  }

  return result;
}

vec3 computeTransmittanceToSkyBoundary(
  in SkyProperties sky, float centreToPointDist, float mu) {
  float r = centreToPointDist;

  return exp(
    -(sky.rayleighScatteringCoef *
        computeOpticalLengthToTopSkyBoundary(sky, sky.rayleighDensity, r, mu) +
      sky.mieExtinctionCoef *
        computeOpticalLengthToTopSkyBoundary(sky, sky.mieDensity, r, mu) +
      sky.absorptionExtinctionCoef *
        computeOpticalLengthToTopSkyBoundary(sky, sky.absorptionDensity, r, mu))
  );
}

vec2 getTransmittanceTextureUVFromRMu(
  in SkyProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;

  /*
    Distance from ground level to top sky boundary along a horizontal ray 
    tangent to the ground
    Simple Pythagoras
   */
  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  // Distance to the horizon
  float rho = safeSqrt(
    r * r - sky.bottomRadius * sky.bottomRadius);

  float d = distToSkyBoundary(sky, r, mu);
  float dMin = sky.topRadius - r;
  float dMax = rho + h;

  // The mapping for mu is done in terms of dMin and dMax (0 -> 1)
  float xMu = (d - dMin) / (dMax - dMin);
  float xR = rho / h;

  return vec2(
    getTextureCoordFromUnit(xMu, TRANSMITTANCE_TEXTURE_WIDTH),
    getTextureCoordFromUnit(xR, TRANSMITTANCE_TEXTURE_HEIGHT));
}

void getRMuFromTransmittanceTextureUv(
  in SkyProperties sky,
  in vec2 uvs,
  out float centreToPointDist,
  out float mu) {
  float xMu = getUnitFromTextureCoord(uvs.x, TRANSMITTANCE_TEXTURE_WIDTH);
  float xR = getUnitFromTextureCoord(uvs.y, TRANSMITTANCE_TEXTURE_HEIGHT);

  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  float rho = h * xR;

  centreToPointDist = sqrt(rho * rho + sky.bottomRadius * sky.bottomRadius);

  float dMin = sky.topRadius - centreToPointDist;
  float dMax = rho + h;

  float d = dMin + xMu * (dMax - dMin);
  mu =
    (d == 0.0) ?
    1.0 :
    (h * h - rho * rho - d * d) / (2.0 * centreToPointDist * d);
  mu = clamp0To1(mu);
}

// Used for precomputing the transmittance
vec3 computeTransmittanceToSkyBoundaryTexture(
  in SkyProperties sky,
  in vec2 fragCoord) {
  const vec2 TRANSMITTANCE_TEXTURE_SIZE =
    vec2(TRANSMITTANCE_TEXTURE_WIDTH, TRANSMITTANCE_TEXTURE_HEIGHT);

  // Courtesy to Vulkan flipping the y value
  vec2 uvFragCoord = fragCoord / TRANSMITTANCE_TEXTURE_SIZE;
  uvFragCoord.y = 1.0 - uvFragCoord.y;

  float centreToPointDist;
  float mu;

  getRMuFromTransmittanceTextureUv(
    sky,
    uvFragCoord,
    centreToPointDist,
    mu);

  return computeTransmittanceToSkyBoundary(sky, centreToPointDist, mu);
}

vec3 getTransmittanceToSkyBoundary(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  float centreToPointDist, float mu) {
  vec2 uv = getTransmittanceTextureUVFromRMu(sky, centreToPointDist, mu);
  return vec3(texture(transmittanceTexture, uv));
}

vec3 getTransmittance(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  float r, float mu,
  float d,
  bool doesRMuIntersectGround) {
  float rD = clampRadius(
    sky,
    sqrt(d * d + 2.0 * r * mu * d + r * r));

  float muD = clamp0To1((r * mu + d) / rD);

  if (doesRMuIntersectGround) {
    return min(
      getTransmittanceToSkyBoundary(
        sky, transmittanceTexture, rD, -muD) /
      getTransmittanceToSkyBoundary(
        sky, transmittanceTexture, r, -mu),
      vec3(1.0));
  }
  else {
    return min(
      getTransmittanceToSkyBoundary(
        sky, transmittanceTexture, r, mu) /
      getTransmittanceToSkyBoundary(
        sky, transmittanceTexture, rD, muD),
      vec3(1.0));
  }
}

vec3 getTransmittanceToSun(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  float r, float muSun) {
  float sinThetaH = sky.bottomRadius / r;
  float cosThetaH = -sqrt(max(1.0 - sinThetaH * sinThetaH, 0.0));

  float visibleFactor = smoothstep(
    -sinThetaH * sky.solarAngularRadius,
    sinThetaH * sky.solarAngularRadius,
    muSun - cosThetaH);

  return getTransmittanceToSkyBoundary(
    sky, transmittanceTexture, r, muSun) * visibleFactor;
}

void computeSingleScatteringIntegrand(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  float r, float mu,
  float muSun,
  float nu, float d,
  bool doesRMuIntersectGround,
  out vec3 rayleigh, out vec3 mie) {
  float rD = clampRadius(
    sky, sqrt(d * d + 2.0 * r * mu * d + r * r));

  float muSD = clamp0To1((r * muSun + d * nu) / rD);

  vec3 t =
    getTransmittance(
      sky, transmittanceTexture,
      r, mu, d, doesRMuIntersectGround) *
    getTransmittanceToSun(
      sky, transmittanceTexture, rD, muSD);

  rayleigh = t * getProfileDensity(
    sky.rayleighDensity, rD - sky.bottomRadius);

  mie = t * getProfileDensity(
    sky.mieDensity, rD - sky.bottomRadius);
}

// Ground or Sky
float distToNearestBoundary(
  in SkyProperties sky,
  float centreToPointDist, float mu, bool doesRMuIntersectGround) {
  if (doesRMuIntersectGround) {
    return distToGroundBoundary(sky, centreToPointDist, mu);
  }
  else {
    return distToSkyBoundary(sky, centreToPointDist, mu);
  }
}

void computeSingleScattering(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  float r, float mu,
  float muSun, float nu,
  bool doesRMuIntersectGround,
  out vec3 rayleigh, out vec3 mie) {
  const int SAMPLE_COUNT = 50;

  float dx = distToNearestBoundary(
    sky, r, mu, doesRMuIntersectGround) / float(SAMPLE_COUNT);

  vec3 totalRayleigh = vec3(0.0);
  vec3 totalMie = vec3(0.0);

  for (int i = 0; i <= SAMPLE_COUNT; ++i) {
    float currentDist = float(i) * dx;

    vec3 currentRayleigh;
    vec3 currentMie;

    computeSingleScatteringIntegrand(
      sky, transmittanceTexture,
      r, mu, muSun, nu, currentDist,
      doesRMuIntersectGround, currentRayleigh, currentMie);

    float weight = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
    totalRayleigh += currentRayleigh * weight;
    totalMie += currentMie * weight;
  }

  rayleigh = totalRayleigh * dx * sky.solarIrradiance *
    sky.rayleighScatteringCoef;

  mie = totalMie * dx * sky.solarIrradiance * sky.mieScatteringCoef;
}

float rayleighPhase(float nu) {
  float k = 3.0 / (16.0 * PI);
  return k * (1.0 + nu * nu);
}

float miePhase(float g, float nu) {
  float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
  return k * (1.0 + nu * nu) / pow(1.0 + g * g - 2.0 * g * nu, 1.5);
}

vec4 getScatteringTextureUVWZFromRMuMuSunNu(
  in SkyProperties sky,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround) {
  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  float rho = safeSqrt(r * r - sky.bottomRadius * sky.bottomRadius);

  float rMapping = getTextureCoordFromUnit(rho / h, SCATTERING_TEXTURE_R_SIZE);

  float rMu = r * mu;
  float delta = rMu * rMu - r * r + sky.bottomRadius * sky.bottomRadius;

  float muMapping;

  if (doesRMuIntersectGround) {
    float d = -rMu - safeSqrt(delta);
    float dMin = r - sky.bottomRadius;
    float dMax = rho;
    muMapping = 0.5 - 0.5 * getTextureCoordFromUnit(
      dMax == dMin ? 0.0 : (d - dMin) / (dMax - dMin),
      SCATTERING_TEXTURE_MU_SIZE / 2);
  }
  else {
    float d = -rMu + safeSqrt(delta + h * h);
    float dMin = sky.topRadius - r;
    float dMax = rho + h;
    muMapping = 0.5 + 0.5 * getTextureCoordFromUnit(
      (d - dMin) / (dMax - dMin), SCATTERING_TEXTURE_MU_SIZE / 2);
  }

  float d = distToSkyBoundary(sky, sky.bottomRadius, muSun);
  float dMin = sky.topRadius - sky.bottomRadius;
  float dMax = h;
  float a = (d - dMin) / (dMax - dMin);
  float dMuSunMin = distToSkyBoundary(sky, sky.bottomRadius, sky.muSunMin);
  float aMuSunMin = (dMuSunMin - dMin) / (dMax - dMin);

  float muSunMapping = getTextureCoordFromUnit(
    max(1.0 - a / aMuSunMin, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);

  float nuMapping = (nu + 1.0) / 2.0;

  return vec4(nuMapping, muSunMapping, muMapping, rMapping);
}

void getRMuMuSunNuFromScatteringTextureUVWZ(
  in SkyProperties sky, in vec4 uvwz,
  out float r, out float mu, out float muSun, out float nu,
  out bool doesRMuIntersectGround) {
  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  float rho = h * getUnitFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);

  r = sqrt(rho * rho + sky.bottomRadius * sky.bottomRadius);

  if (uvwz.z < 0.5) {
    float dMin = r - sky.bottomRadius;
    float dMax = rho;

    float d = dMin + (dMax - dMin) * getUnitFromTextureCoord(
      1.0 - 2.0 * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);

    mu = (d == 0.0) ? -1.0 : clamp0To1(-(rho * rho + d * d) / (2.0 * r * d));

    doesRMuIntersectGround = true;
  }
  else {
    float dMin = sky.topRadius - r;
    float dMax = rho + h;
    float d = dMin + (dMax - dMin) * getUnitFromTextureCoord(
      2.0 * uvwz.z - 1.0, SCATTERING_TEXTURE_MU_SIZE / 2);

    mu = (d == 0.0) ? 1.0 :
      clamp0To1((h * h - rho * rho - d * d) / (2.0 * r * d));

    doesRMuIntersectGround = false;
  }

  float xMuSun =
    getUnitFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
  float dMin = sky.topRadius - sky.bottomRadius;
  float dMax = h;
  float A =
    -2.0 * sky.muSunMin * sky.bottomRadius / (dMax - dMin);
  float a = (A - xMuSun * A) / (1.0 + xMuSun * A);
  float d = dMin + min(a, A) * (dMax - dMin);
  muSun = d == 0.0 ? float(1.0) :
    clamp0To1((h * h - d * d) / (2.0 * sky.bottomRadius * d));
  nu = clamp0To1(uvwz.x * 2.0 - 1.0);
}

void getRMuMuSunNuFromScatteringTextureFragCoord(
  in SkyProperties sky, in vec3 fragCoord,
  out float r, out float mu, out float muSun, out float nu,
  out bool doesRMuIntersectGround) {
  const vec4 SCATTERING_TEXTURE_SIZE = vec4(
    SCATTERING_TEXTURE_NU_SIZE - 1,
    SCATTERING_TEXTURE_MU_S_SIZE,
    SCATTERING_TEXTURE_MU_SIZE,
    SCATTERING_TEXTURE_R_SIZE);

  float fragCoordNu =
    floor(fragCoord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));
  float fragCoordMuSun =
    mod(fragCoord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));

  vec4 uvwz = vec4(fragCoordNu, fragCoordMuSun, fragCoord.y, fragCoord.z) /
    SCATTERING_TEXTURE_SIZE;

  getRMuMuSunNuFromScatteringTextureUVWZ(
    sky, uvwz, r, mu, muSun, nu, doesRMuIntersectGround);

  nu = clamp(
    nu,
    mu * muSun - sqrt((1.0 - mu * mu) * (1.0 - muSun * muSun)),
    mu * muSun + sqrt((1.0 - mu * mu) * (1.0 - muSun * muSun)));
}

void computeSingleScatteringTexture(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  vec3 fragCoord,
  out vec3 rayleigh, out vec3 mie) {
  float r;
  float mu;
  float muSun;
  float nu;
  bool doesRMuIntersectGround;

  // Vulkan flipping y
  fragCoord.y = SCATTERING_TEXTURE_MU_SIZE - fragCoord.y;

  getRMuMuSunNuFromScatteringTextureFragCoord(
    sky, fragCoord, r, mu, muSun, nu, doesRMuIntersectGround);

  computeSingleScattering(
    sky, transmittanceTexture,
    r, mu, muSun, nu, doesRMuIntersectGround, rayleigh, mie);
}

#endif
