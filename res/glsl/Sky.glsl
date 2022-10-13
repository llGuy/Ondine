#ifndef SKY_UTILS_GLSL
#define SKY_UTILS_GLSL

#include "Utils.glsl"
#include "PlanetDef.glsl"

/* Some utility functions */
float clamp0To1(float a) {
  return clamp(a, -1.0, 1.0);
}

float clampPositive(float a) {
  return max(a, 0.0);
}

float clampRadius(in PlanetProperties sky, float radius) {
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
  in PlanetProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.topRadius * sky.topRadius;
  return clampPositive(-r * mu + safeSqrt(delta));
}

float distToGroundBoundary(
  in PlanetProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.bottomRadius * sky.bottomRadius;
  return clampPositive(-r * mu - safeSqrt(delta));
}

bool doesRayIntersectGround(
  in PlanetProperties sky,
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
  in PlanetProperties sky,
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
  in PlanetProperties sky, float centreToPointDist, float mu) {
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
  in PlanetProperties sky,
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
  in PlanetProperties sky,
  in vec2 uvs,
  out float centreToPointDist,
  out float mu) {
  float xMu = getUnitFromTextureCoord(uvs.x, TRANSMITTANCE_TEXTURE_WIDTH);
  float xR = getUnitFromTextureCoord(uvs.y, TRANSMITTANCE_TEXTURE_HEIGHT);

  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  float rho = h * xR;

  centreToPointDist = sqrt(rho * rho + sky.bottomRadius * sky.bottomRadius);

  float dMin = max(0.0, sky.topRadius - centreToPointDist);
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
  in PlanetProperties sky,
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
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  float centreToPointDist, float mu) {
  vec2 uv = getTransmittanceTextureUVFromRMu(sky, centreToPointDist, mu);
  return vec3(texture(transmittanceTexture, vec2(uv.x, 1.0 - uv.y)));
}

vec3 getTransmittance(
  in PlanetProperties sky,
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
  in PlanetProperties sky,
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
  in PlanetProperties sky,
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
  in PlanetProperties sky,
  float centreToPointDist, float mu, bool doesRMuIntersectGround) {
  if (doesRMuIntersectGround) {
    return distToGroundBoundary(sky, centreToPointDist, mu);
  }
  else {
    return distToSkyBoundary(sky, centreToPointDist, mu);
  }
}

void computeSingleScattering(
  in PlanetProperties sky,
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
  in PlanetProperties sky,
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
  in PlanetProperties sky, in vec4 uvwz,
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
  PlanetProperties sky, vec3 fragCoord,
  out float r, out float mu, out float muS, out float nu,
  out bool rayRMuIntersectsGround) {
  const vec4 SCATTERING_TEXTURE_SIZE = vec4(
    SCATTERING_TEXTURE_NU_SIZE - 1,
    SCATTERING_TEXTURE_MU_S_SIZE,
    SCATTERING_TEXTURE_MU_SIZE,
    SCATTERING_TEXTURE_R_SIZE);
  float fragCoordNu =
    floor(fragCoord.x / float(SCATTERING_TEXTURE_MU_S_SIZE));

  float fragCoordMuS =
    mod(fragCoord.x, float(SCATTERING_TEXTURE_MU_S_SIZE));

  vec4 uvwz =
    vec4(fragCoordNu, fragCoordMuS, fragCoord.y, fragCoord.z) /
    SCATTERING_TEXTURE_SIZE;

  getRMuMuSunNuFromScatteringTextureUVWZ(
    sky, uvwz, r, mu, muS, nu, rayRMuIntersectsGround);
  
  nu = clamp(
    nu, mu * muS - sqrt((1.0 - mu * mu) * (1.0 - muS * muS)),
    mu * muS + sqrt((1.0 - mu * mu) * (1.0 - muS * muS)));
}

void computeSingleScatteringTexture(
  in PlanetProperties sky,
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

vec3 getIrradiance(
  in PlanetProperties sky,
  in sampler2D irradianceTexture,
  float r, float muSun);

vec4 getScatteringTextureUvwzFromRMuMuSNu(
  in PlanetProperties sky,
  float r, float mu, float muS, float nu,
  bool rayRMuIntersectsGround) {
  float h = sqrt(
    sky.topRadius * sky.topRadius - sky.bottomRadius * sky.bottomRadius);

  float rho = safeSqrt(r * r - sky.bottomRadius * sky.bottomRadius);
  float uR = getTextureCoordFromUnit(rho / h, SCATTERING_TEXTURE_R_SIZE);

  float rMu = r * mu;

  float discriminant = rMu * rMu - r * r + sky.bottomRadius * sky.bottomRadius;

  float uMu;

  if (rayRMuIntersectsGround) {
    float d = -rMu - safeSqrt(discriminant);
    float dMin = r - sky.bottomRadius;
    float dMax = rho;
    uMu = 0.5 - 0.5 * getTextureCoordFromUnit(
      dMax == dMin ? 0.0 :
      (d - dMin) / (dMax - dMin), SCATTERING_TEXTURE_MU_SIZE / 2);
  }
  else {
    float d = -rMu + safeSqrt(discriminant + h * h);
    float dMin = sky.topRadius - r;
    float dMax = rho + h;
    uMu = 0.5 + 0.5 * getTextureCoordFromUnit(
      (d - dMin) / (dMax - dMin), SCATTERING_TEXTURE_MU_SIZE / 2);
  }

  float d = distToSkyBoundary(
    sky, sky.bottomRadius, muS);
  float dMin = sky.topRadius - sky.bottomRadius;
  float dMax = h;
  float a = (d - dMin) / (dMax - dMin);
  float A =
    -2.0 * sky.muSunMin * sky.bottomRadius / (dMax - dMin);
  float uMuS = getTextureCoordFromUnit(
    max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);

  float uNu = (nu + 1.0) / 2.0;
  return vec4(uNu, uMuS, uMu, uR);
}

vec3 getScattering(
  PlanetProperties sky,
  sampler3D scatteringTexture,
  float r, float mu, float muS, float nu,
  bool rayRMuIntersectsGround) {
  vec4 uvwz = getScatteringTextureUvwzFromRMuMuSNu(
    sky, r, mu, muS, nu, rayRMuIntersectsGround);

  float texCoordX = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);

  float texX = floor(texCoordX);
  float lerp = texCoordX - texX;

  vec3 uvw0 = vec3(
    (texX + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);

  vec3 uvw1 = vec3(
    (texX + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE), uvwz.z, uvwz.w);

  uvw0.y = 1.0 - uvw0.y;
  uvw1.y = 1.0 - uvw1.y;

  return vec3(
    texture(scatteringTexture, uvw0) * (1.0 - lerp) +
    texture(scatteringTexture, uvw1) * lerp);
}

vec3 getScattering(
  PlanetProperties sky,
  sampler3D singleRayleighScatteringTexture,
  sampler3D singleMieScatteringTexture,
  sampler3D multipleScatteringTexture,
  float r, float mu, float muS, float nu,
  bool rayRMuIntersectsGround,
  int scatteringOrder) {
  if (scatteringOrder == 1) {
    vec3 rayleigh = getScattering(
      sky, singleRayleighScatteringTexture, r, mu, muS, nu,
      rayRMuIntersectsGround);

    vec3 mie = getScattering(
      sky, singleMieScatteringTexture, r, mu, muS, nu,
      rayRMuIntersectsGround);

    return rayleigh * rayleighPhase(nu) +
      mie * miePhase(sky.miePhaseFunctionG, nu);
  }
  else {
    return getScattering(
      sky, multipleScatteringTexture, r, mu, muS, nu,
      rayRMuIntersectsGround);
  }
}

vec3 computeScatteringDensity(
  PlanetProperties sky,
  sampler2D transmittanceTexture,
  sampler3D singleRayleighScatteringTexture,
  sampler3D singleMieScatteringTexture,
  sampler3D multipleScatteringTexture,
  sampler2D irradianceTexture,
  float r, float mu, float muS, float nu, int scatteringOrder) {
  vec3 zenithDirection = vec3(0.0, 0.0, 1.0);
  vec3 omega = vec3(sqrt(1.0 - mu * mu), 0.0, mu);
  float sunDirX = omega.x == 0.0 ? 0.0 : (nu - mu * muS) / omega.x;
  float sunDirY = sqrt(max(1.0 - sunDirX * sunDirX - muS * muS, 0.0));
  vec3 omegaS = vec3(sunDirX, sunDirY, muS);

  const int SAMPLE_COUNT = 16;
  const float dphi = PI / float(SAMPLE_COUNT);
  const float dtheta = PI / float(SAMPLE_COUNT);
  vec3 rayleighMie = vec3(0.0);

  for (int l = 0; l < SAMPLE_COUNT; ++l) {
    float theta = (float(l) + 0.5) * dtheta;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    bool rayRThetaIntersectsGround = doesRayIntersectGround(sky, r, cosTheta);

    float distanceToGround = 0.0;
    vec3 transmittanceToGround = vec3(0.0);
    vec3 groundAlbedo = vec3(0.0);
    if (rayRThetaIntersectsGround) {
      distanceToGround = distToGroundBoundary(sky, r, cosTheta);

      transmittanceToGround = getTransmittance(
        sky, transmittanceTexture, r, cosTheta, distanceToGround, true);

      groundAlbedo = sky.groundAlbedo;
    }

    for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
      float phi = (float(m) + 0.5) * dphi;
      vec3 omegaI = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
      float domegaI = (dtheta) * (dphi) * sin(theta);

      float nu1 = dot(omegaS, omegaI);

      vec3 incidentRadiance = getScattering(
        sky,
        singleRayleighScatteringTexture, singleMieScatteringTexture,
        multipleScatteringTexture, r, omegaI.z, muS, nu1,
        rayRThetaIntersectsGround, scatteringOrder - 1);

      vec3 groundNormal =
          normalize(zenithDirection * r + omegaI * distanceToGround);

      vec3 groundIrradiance = getIrradiance(
          sky, irradianceTexture, sky.bottomRadius,
          dot(groundNormal, omegaS));

      incidentRadiance += transmittanceToGround *
          groundAlbedo * (1.0 / (PI)) * groundIrradiance;

      float nu2 = dot(omega, omegaI);

      float rayleighDensity = getProfileDensity(
          sky.rayleighDensity, r - sky.bottomRadius);

      float mieDensity = getProfileDensity(
          sky.mieDensity, r - sky.bottomRadius);

      rayleighMie += incidentRadiance * (
          sky.rayleighScatteringCoef * rayleighDensity *
              rayleighPhase(nu2) +
          sky.mieScatteringCoef * mieDensity *
              miePhase(sky.miePhaseFunctionG, nu2)) *
          domegaI;
    }
  }
  return rayleighMie;
}

vec3 computeMultipleScattering(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringDensityTexture,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround) {
  const int SAMPLE_COUNT = 50;

  float dx = distToNearestBoundary(
    sky, r, mu, doesRMuIntersectGround) / float(SAMPLE_COUNT);
  
  vec3 rayleighMieSum = vec3(0.0);
  for (int i = 0; i <= SAMPLE_COUNT; ++i) {
    float dI = float(i) * dx;

    float rI = clampRadius(sky, sqrt(dI * dI + 2.0 * r * mu * dI + r * r));
    float muI = clamp0To1((r * mu + dI) / rI);
    float muSI = clamp0To1((r * muSun + dI * nu) / rI);

    vec3 rayleighMieI =
      getScattering(
        sky, scatteringDensityTexture, rI, muI, muSI, nu,
        doesRMuIntersectGround) *
      getTransmittance(
        sky, transmittanceTexture, r, mu, dI,
        doesRMuIntersectGround) *
      dx;

    float weightI = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
    rayleighMieSum += rayleighMieI * weightI;
  }
  return rayleighMieSum;
}

vec3 computeScatteringDensityTexture(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  in sampler2D irradianceTexture,
  vec3 fragCoord, int scatteringOrder) {
  fragCoord.y = SCATTERING_TEXTURE_MU_SIZE - fragCoord.y;

  float r, mu, muSun, nu;

  bool doesRMuIntersectGround;

  getRMuMuSunNuFromScatteringTextureFragCoord(
    sky, fragCoord, r, mu, muSun, nu, doesRMuIntersectGround);

  return computeScatteringDensity(
    sky, transmittanceTexture, singleRayleighScatteringTexture,
    singleMieScatteringTexture, multipleScatteringTexture,
    irradianceTexture, r, mu, muSun, nu, scatteringOrder);
}

vec3 computeMultipleScatteringTexture(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringDensityTexture,
  vec3 fragCoord, out float nu) {
  fragCoord.y = SCATTERING_TEXTURE_MU_SIZE - fragCoord.y;

  float r;
  float mu;
  float muSun;
  bool doesRMuIntersectGround;
  getRMuMuSunNuFromScatteringTextureFragCoord(
    sky, fragCoord,
    r, mu, muSun, nu, doesRMuIntersectGround);
  
  return computeMultipleScattering(
    sky, transmittanceTexture,
    scatteringDensityTexture, r, mu, muSun, nu,
    doesRMuIntersectGround);
}

vec3 computeDirectIrradiance(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  float r, float muSun) {
  float alphaSun = sky.solarAngularRadius;

  float averageCosineFactor =
    (muSun < -alphaSun) ? 0.0 :
    (muSun > alphaSun ? muSun :
     (muSun + alphaSun) * (muSun + alphaSun) / (4.0 * alphaSun));

  return sky.solarIrradiance * getTransmittanceToSkyBoundary(
    sky, transmittanceTexture, r, muSun) * averageCosineFactor;

}

vec3 computeIndirectIrradiance(
  in PlanetProperties sky,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  float r, float muSun, int scatteringOrder) {
  const int SAMPLE_COUNT = 32;

  float dphi = PI / float(SAMPLE_COUNT);
  float dtheta = PI / float(SAMPLE_COUNT);

  vec3 result = vec3(0.0);

  vec3 omegaSun = vec3(sqrt(1.0 - muSun * muSun), 0.0, muSun);

  for (int j = 0; j < SAMPLE_COUNT / 2; ++j) {
    float theta = (float(j) + 0.5) * dtheta;
    for (int i = 0; i < 2 * SAMPLE_COUNT; ++i) {
      float phi = (float(i) + 0.5) * dphi;

      vec3 omega = vec3(
        cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

      float domega = dtheta * dphi * sin(theta);

      float nu = dot(omega, omegaSun);

      result += getScattering(
        sky, singleRayleighScatteringTexture, singleMieScatteringTexture,
        multipleScatteringTexture,
        r, omega.z, muSun, nu, false, scatteringOrder) * omega.z * domega;
    }
  }

  return result;
}

vec2 getIrradianceTextureUVFromRMuSun(
  in PlanetProperties sky, float r, float muSun) {
  float rMapping = (r - sky.bottomRadius) /
    (sky.topRadius - sky.bottomRadius);
  float muSunMapping = muSun * 0.5 + 0.5;

  return vec2(
    getTextureCoordFromUnit(muSunMapping, IRRADIANCE_TEXTURE_WIDTH),
    getTextureCoordFromUnit(rMapping, IRRADIANCE_TEXTURE_HEIGHT));
}

void getRMuSunFromIrradianceTextureUV(
  in PlanetProperties sky,
  in vec2 uv, out float r, out float muSun) {
  float muSunMapping = getUnitFromTextureCoord(uv.x, IRRADIANCE_TEXTURE_WIDTH);
  float rMapping = getUnitFromTextureCoord(uv.y, IRRADIANCE_TEXTURE_HEIGHT);

  r = sky.bottomRadius +
    rMapping * (sky.topRadius - sky.bottomRadius);
  muSun = clamp0To1(2.0 * muSunMapping - 1.0);
}

const vec2 IRRADIANCE_TEXTURE_SIZE =
  vec2(IRRADIANCE_TEXTURE_WIDTH, IRRADIANCE_TEXTURE_HEIGHT);

vec3 computeDirectIrradianceTexture(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in vec2 fragCoord) {
  float r;
  float muS;
  
  vec2 uvFragCoord = fragCoord / IRRADIANCE_TEXTURE_SIZE;
  uvFragCoord.y = 1.0 - uvFragCoord.y;

  getRMuSunFromIrradianceTextureUV(
    sky, uvFragCoord, r, muS);
  return computeDirectIrradiance(sky, transmittanceTexture, r, muS);
}

vec3 computeIndirectIrradianceTexture(
  in PlanetProperties sky,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  in vec2 fragCoord, int scatteringOrder) {
  float r;
  float muSun;

  vec2 uvFragCoord = fragCoord / IRRADIANCE_TEXTURE_SIZE;
  uvFragCoord.y = 1.0 - uvFragCoord.y;

  getRMuSunFromIrradianceTextureUV(
    sky, uvFragCoord, r, muSun);

  return computeIndirectIrradiance(
    sky,
    singleRayleighScatteringTexture, singleMieScatteringTexture,
    multipleScatteringTexture, r, muSun, scatteringOrder);
}

vec3 getIrradiance(
  in PlanetProperties sky,
  in sampler2D irradianceTexture,
  float r, float muSun) {
  vec2 uv = getIrradianceTextureUVFromRMuSun(sky, r, muSun);
  return vec3(texture(irradianceTexture, vec2(uv.x, 1.0 - uv.y)));
}

vec3 getSolarRadiance(in PlanetProperties sky) {
  return sky.solarIrradiance /
    (PI * sky.solarAngularRadius * sky.solarAngularRadius);
}

vec3 getExtrapolatedSingleMieScattering(
  in PlanetProperties sky, in vec4 scattering) {
  if (scattering.r == 0.0) {
    return vec3(0.0);
  }

  return scattering.rgb * scattering.a / scattering.r *
    (sky.rayleighScatteringCoef.r / sky.mieScatteringCoef.r) *
    (sky.mieScatteringCoef / sky.rayleighScatteringCoef);
}

vec3 getCombinedScattering(
  in PlanetProperties sky,
  in sampler3D scatteringTexture,
  in sampler3D singleMieScatteringTexture,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround,
  out vec3 singleMieScattering) {
  vec4 uvwz = getScatteringTextureUVWZFromRMuMuSunNu(
      sky, r, mu, muSun, nu, doesRMuIntersectGround);

  float texCoordX = uvwz.x * float(SCATTERING_TEXTURE_NU_SIZE - 1);
  float texX = floor(texCoordX);
  float lerp = texCoordX - texX;
  vec3 uvw0 = vec3((texX + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  vec3 uvw1 = vec3((texX + 1.0 + uvwz.y) / float(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);

  uvw0.y = 1.0 - uvw0.y;
  uvw1.y = 1.0 - uvw1.y;

  vec4 combinedScattering =
      texture(scatteringTexture, uvw0) * (1.0 - lerp) +
      texture(scatteringTexture, uvw1) * lerp;
  vec3 scattering = vec3(combinedScattering);

  singleMieScattering = getExtrapolatedSingleMieScattering(
    sky, combinedScattering);
  return scattering;
}

vec3 getSkyRadiance(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringTexture,
  in sampler3D singleMieScateringTexture,
  vec3 camera, vec3 viewRay, float shadowLength,
  vec3 sunDirection, out vec3 transmittance) {
  float r = length(camera);
  float rMu = dot(camera, viewRay);
  
  float distToTopSkyBoundary = -rMu -
    sqrt(rMu * rMu - r * r + sky.topRadius * sky.topRadius);

  if (distToTopSkyBoundary > 0.0) {
    camera = camera + viewRay * distToTopSkyBoundary;
    r = sky.topRadius;
    rMu += distToTopSkyBoundary;
  }
  else if (r > sky.topRadius) {
    transmittance = vec3(1.0);
    // SPACE!
    return vec3(0.0);
  }

  // retrieve cos of the zenith angle
  float mu = rMu / r;
  float muSun = dot(camera, sunDirection) / r;
  float nu = dot(viewRay, sunDirection);
  bool doesRMuIntersectGround = doesRayIntersectGround(sky, r, mu);

  transmittance = doesRMuIntersectGround ? vec3(0.0) :
    getTransmittanceToSkyBoundary(sky, transmittanceTexture, r, mu);

  vec3 singleMieScattering;
  vec3 scattering;

  if (shadowLength == 0.0) {
    scattering = getCombinedScattering(
      sky, scatteringTexture, singleMieScateringTexture,
      r, mu, muSun, nu, doesRMuIntersectGround, singleMieScattering);
  }
  else {
    float d = shadowLength;
    float rP = clampRadius(sky, sqrt(d * d + 2.0 * r * mu * d + r * r));
    float muP = (r * mu + d) / rP;
    float muSunP = (r * muSun + d * nu) / rP;

    scattering = getCombinedScattering(
      sky, scatteringTexture, singleMieScateringTexture,
      rP, muP, muSunP, nu, doesRMuIntersectGround, singleMieScattering);

    vec3 shadowTransmittance = getTransmittance(
      sky, transmittanceTexture, r, mu, shadowLength, doesRMuIntersectGround);

    scattering = scattering * shadowTransmittance;
    singleMieScattering = singleMieScattering * shadowTransmittance;
  }

  return scattering * rayleighPhase(nu) + singleMieScattering *
    miePhase(sky.miePhaseFunctionG, nu);
}

vec3 getSkyRadianceToPoint(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringTexture,
  in sampler3D singleMieScatteringTexture,
  vec3 camera, vec3 point, float shadowLength,
  vec3 sunDirection, out vec3 transmittance) {
  vec3 viewRay = normalize(point - camera);
  float r = length(camera);
  float rmu = dot(camera, viewRay);
  float distToTopSkyBoundary = -rmu -
      sqrt(rmu * rmu - r * r + sky.topRadius * sky.topRadius);

  if (distToTopSkyBoundary > 0.0) {
    camera = camera + viewRay * distToTopSkyBoundary;
    r = sky.topRadius;
    rmu += distToTopSkyBoundary;
  }

  float mu = rmu / r;
  float muSun = dot(camera, sunDirection) / r;
  float nu = dot(viewRay, sunDirection);
  float d = length(point - camera);
  bool doesRMuIntersectGround = doesRayIntersectGround(sky, r, mu);

  transmittance = getTransmittance(
    sky, transmittanceTexture, r, mu, d, doesRMuIntersectGround);

  vec3 singleMieScattering;
  vec3 scattering = getCombinedScattering(
    sky, scatteringTexture, singleMieScatteringTexture,
    r, mu, muSun, nu, doesRMuIntersectGround, singleMieScattering);

  d = max(d - shadowLength, 0.0);
  float rP = clampRadius(sky, sqrt(d * d + 2.0 * r * mu * d + r * r));
  float muP = (r * mu + d) / rP;
  float muSunP = (r * muSun + d * nu) / rP;

  vec3 singleMieScatteringP;
  vec3 scatteringP = getCombinedScattering(
    sky, scatteringTexture, singleMieScatteringTexture,
    rP, muP, muSunP, nu, doesRMuIntersectGround, singleMieScatteringP);

  vec3 shadowTransmittance = transmittance;
  if (shadowLength > 0.0) {
    shadowTransmittance = getTransmittance(
      sky, transmittanceTexture,
      r, mu, d, doesRMuIntersectGround);
  }

  scattering = scattering - shadowTransmittance * scatteringP;
  singleMieScattering = singleMieScattering -
    shadowTransmittance * singleMieScatteringP;

  singleMieScattering = getExtrapolatedSingleMieScattering(
    sky, vec4(scattering, singleMieScattering.r));

  singleMieScattering = singleMieScattering * smoothstep(0.0, 0.01f, muSun);

  return scattering * rayleighPhase(nu) + singleMieScattering *
    miePhase(sky.miePhaseFunctionG, nu);
}

vec3 getSunAndSkyIrradiance(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler2D irradianceTexture,
  vec3 point, vec3 normal, vec3 sunDirection,
  out vec3 skyIrradiance) {
  float r = length(point);
  float muSun = dot(point, sunDirection) / r;

  skyIrradiance = getIrradiance(sky, irradianceTexture, r, muSun) *
    (1.0 + dot(normal, point) / r) * 0.5;

  float incidentIntensity = max(dot(normal, sunDirection), 0.0);

  return sky.solarIrradiance *
    getTransmittanceToSun(sky, transmittanceTexture, r, muSun) *
    max(dot(normal, sunDirection), 0.0);
}

vec3 getSunAndSkyIrradianceToon(
  in PlanetProperties sky,
  in sampler2D transmittanceTexture,
  in sampler2D irradianceTexture,
  vec3 point, vec3 normal, vec3 sunDirection,
  out vec3 skyIrradiance) {
  float r = length(point);
  float muSun = dot(point, sunDirection) / r;

  skyIrradiance = getIrradiance(sky, irradianceTexture, r, muSun) *
    (1.0 + dot(normal, point) / r) * 0.5;

  float incidentIntensity = max(dot(normal, sunDirection), 0.0);
  float toonIntensity = toonShadingIncidentIntensity(incidentIntensity);

  return sky.solarIrradiance *
    getTransmittanceToSun(sky, transmittanceTexture, r, muSun) *
    toonIntensity;
}

#endif
