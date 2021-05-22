#ifndef SKY_UTILS_GLSL
#define SKY_UTILS_GLSL

#include "sky_def.glsl"

#define IN(x) const in x
#define OUT(x) out x
#define TEMPLATE(x)
#define TEMPLATE_ARGUMENT(x)
#define assert(x)
#define COMBINED_SCATTERING_TEXTURES

#define Length float
#define Wavelength float
#define Angle float
#define SolidAngle float
#define Power float
#define LuminousPower float
#define Number float
#define InverseLength float
#define Area float
#define Volume float
#define NumberDensity float
#define Irradiance float
#define Radiance float
#define SpectralPower float
#define SpectralIrradiance float
#define SpectralRadiance float
#define SpectralRadianceDensity float
#define ScatteringCoefficient float
#define InverseSolidAngle float
#define LuminousIntensity float
#define Luminance float
#define Illuminance float
#define AbstractSpectrum vec3
#define DimensionlessSpectrum vec3
#define PowerSpectrum vec3
#define IrradianceSpectrum vec3
#define RadianceSpectrum vec3
#define RadianceDensitySpectrum vec3
#define ScatteringSpectrum vec3
#define Position vec3
#define Direction vec3
#define Luminance3 vec3
#define Illuminance3 vec3
                                                \
#define TransmittanceTexture sampler2D
#define AbstractScatteringTexture sampler3D
#define ReducedScatteringTexture sampler3D
#define ScatteringTexture sampler3D
#define ScatteringDensityTexture sampler3D
#define IrradianceTexture sampler2D
const Length m = 1.0;
const Wavelength nm = 1.0;
const Angle rad = 1.0;
const SolidAngle sr = 1.0;
const Power watt = 1.0;
const LuminousPower lm = 1.0;
const Length km = 1000.0 * m;
const Area m2 = m * m;
const Volume m3 = m * m * m;
const Angle pi = PI * rad;
const Angle deg = pi / 180.0;
const Irradiance watt_per_square_meter = watt / m2;
const Radiance watt_per_square_meter_per_sr = watt / (m2 * sr);
const SpectralIrradiance watt_per_square_meter_per_nm = watt / (m2 * nm);
const SpectralRadiance watt_per_square_meter_per_sr_per_nm =
    watt / (m2 * sr * nm);
const SpectralRadianceDensity watt_per_cubic_meter_per_sr_per_nm =
    watt / (m3 * sr * nm);
const LuminousIntensity cd = lm / sr;
const LuminousIntensity kcd = 1000.0 * cd;
const Luminance cd_per_square_meter = cd / m2;
const Luminance kcd_per_square_meter = kcd / m2;

#if 0
struct DensityOProfileLayer {
  Length width;
  Number exp_term;
  InverseLength exp_scale;
  InverseLength linear_term;
  Number constant_term;
};
struct DensityOProfile {
  DensityOProfileLayer layers[2];
};
struct AtmosphereParameters {
  IrradianceSpectrum solar_irradiance;
  Angle sun_angular_radius;
  Length bottom_radius;
  Length top_radius;
  DensityOProfile rayleigh_density;
  ScatteringSpectrum rayleigh_scattering;
  DensityOProfile mie_density;
  ScatteringSpectrum mie_scattering;
  ScatteringSpectrum mie_extinction;
  Number mie_phase_function_g;
  DensityOProfile absorption_density;
  ScatteringSpectrum absorption_extinction;
  DimensionlessSpectrum ground_albedo;
  Number mu_s_min;
};
#endif

#define AtmosphereParameters SkyProperties

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

#if 0
const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(
vec3(1.474000,1.850400,1.911980),
0.004675,
6360.000000,
6420.000000,
DensityOProfile(DensityOProfileLayer[2](DensityOProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityOProfileLayer(0.000000,1.000000,-0.125000,0.000000,0.000000))),
vec3(0.005802,0.013558,0.033100),
DensityOProfile(DensityOProfileLayer[2](DensityOProfileLayer(0.000000,0.000000,0.000000,0.000000,0.000000),DensityOProfileLayer(0.000000,1.000000,-0.833333,0.000000,0.000000))),
vec3(0.003996,0.003996,0.003996),
vec3(0.004440,0.004440,0.004440),
0.800000,
DensityOProfile(DensityOProfileLayer[2](DensityOProfileLayer(25.000000,0.000000,0.000000,0.066667,-0.666667),DensityOProfileLayer(0.000000,0.000000,0.000000,-0.066667,2.666667))),
vec3(0.000650,0.001881,0.000085),
vec3(0.100000,0.100000,0.100000),
-0.207912);
const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(114974.916437,71305.954816,65310.548555);
const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = vec3(98242.786222,69954.398112,66475.012354);
#endif

#if 0
float clamp0To1(float a) {
  return clamp(a, -1.0, 1.0);
}

float clampDistance(float a) {
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
  return clampDistance(-r * mu + safeSqrt(delta));
}

float distToGroundBoundary(
  in SkyProperties sky,
  float centreToPointDist,
  float mu) {
  float r = centreToPointDist;
  float delta = r * r * (mu * mu - 1.0) + sky.bottomRadius * sky.bottomRadius;
  return clampDistance(-r * mu - safeSqrt(delta));
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
#endif


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

#if 0
vec4 GetScatteringTextureUvwzFromRMuMuSNu(IN(AtmosphereParameters) atmosphere,
    Length r, Number mu, Number mu_s, Number nu,
    bool ray_r_mu_intersects_ground) {
  assert(r >= atmosphere.bottomRadius && r <= atmosphere.topRadius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(mu_s >= -1.0 && mu_s <= 1.0);
  assert(nu >= -1.0 && nu <= 1.0);
  Length H = sqrt(atmosphere.topRadius * atmosphere.topRadius -
      atmosphere.bottomRadius * atmosphere.bottomRadius);
  Length rho =
      safeSqrt(r * r - atmosphere.bottomRadius * atmosphere.bottomRadius);
  Number u_r = getTextureCoordFromUnit(rho / H, SCATTERING_TEXTURE_R_SIZE);
  Length r_mu = r * mu;
  Area discriminant =
      r_mu * r_mu - r * r + atmosphere.bottomRadius * atmosphere.bottomRadius;
  Number u_mu;
  if (ray_r_mu_intersects_ground) {
    Length d = -r_mu - safeSqrt(discriminant);
    Length d_min = r - atmosphere.bottomRadius;
    Length d_max = rho;
    u_mu = 0.5 - 0.5 * getTextureCoordFromUnit(d_max == d_min ? 0.0 :
        (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
  } else {
    Length d = -r_mu + safeSqrt(discriminant + H * H);
    Length d_min = atmosphere.topRadius - r;
    Length d_max = rho + H;
    u_mu = 0.5 + 0.5 * getTextureCoordFromUnit(
        (d - d_min) / (d_max - d_min), SCATTERING_TEXTURE_MU_SIZE / 2);
  }
  Length d = distToSkyBoundary(
      atmosphere, atmosphere.bottomRadius, mu_s);
  Length d_min = atmosphere.topRadius - atmosphere.bottomRadius;
  Length d_max = H;
  Number a = (d - d_min) / (d_max - d_min);
  Number A =
      -2.0 * atmosphere.muSunMin * atmosphere.bottomRadius / (d_max - d_min);
  Number u_mu_s = getTextureCoordFromUnit(
      max(1.0 - a / A, 0.0) / (1.0 + a), SCATTERING_TEXTURE_MU_S_SIZE);
  Number u_nu = (nu + 1.0) / 2.0;
  return vec4(u_nu, u_mu_s, u_mu, u_r);
}
#endif

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

  float xMuSun = getUnitFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
  float dMin = sky.topRadius - sky.bottomRadius;
  float dMax = h;
  float dMuSunMin = distToSkyBoundary(sky, sky.bottomRadius, sky.muSunMin);
  float aMuSunMin = (dMuSunMin - dMin) / (dMax - dMin);
  float a = (aMuSunMin - xMuSun * aMuSunMin) / (1.0 + xMuSun * aMuSunMin);
  float d = dMin + min(a, aMuSunMin) * (dMax - dMin);
  muSun = (d == 0.0) ? 1.0 :
    clamp0To1((h * h - d * d) / (2.0 * sky.bottomRadius * d));

  nu = clamp0To1(uvwz.x * 2.0 - 1.0);
}

#if 0
void GetRMuMuSNuFromScatteringTextureUvwz(IN(AtmosphereParameters) atmosphere,
    IN(vec4) uvwz, OUT(Length) r, OUT(Number) mu, OUT(Number) mu_s,
    OUT(Number) nu, OUT(bool) ray_r_mu_intersects_ground) {
  assert(uvwz.x >= 0.0 && uvwz.x <= 1.0);
  assert(uvwz.y >= 0.0 && uvwz.y <= 1.0);
  assert(uvwz.z >= 0.0 && uvwz.z <= 1.0);
  assert(uvwz.w >= 0.0 && uvwz.w <= 1.0);
  Length H = sqrt(atmosphere.topRadius * atmosphere.topRadius -
      atmosphere.bottomRadius * atmosphere.bottomRadius);
  Length rho =
      H * getUnitFromTextureCoord(uvwz.w, SCATTERING_TEXTURE_R_SIZE);
  r = sqrt(rho * rho + atmosphere.bottomRadius * atmosphere.bottomRadius);
  if (uvwz.z < 0.5) {
    Length d_min = r - atmosphere.bottomRadius;
    Length d_max = rho;
    Length d = d_min + (d_max - d_min) * getUnitFromTextureCoord(
        1.0 - 2.0 * uvwz.z, SCATTERING_TEXTURE_MU_SIZE / 2);
    mu = d == 0.0 * m ? Number(-1.0) :
        clamp0To1(-(rho * rho + d * d) / (2.0 * r * d));
    ray_r_mu_intersects_ground = true;
  } else {
    Length d_min = atmosphere.topRadius - r;
    Length d_max = rho + H;
    Length d = d_min + (d_max - d_min) * getUnitFromTextureCoord(
        2.0 * uvwz.z - 1.0, SCATTERING_TEXTURE_MU_SIZE / 2);
    mu = d == 0.0 * m ? Number(1.0) :
        clamp0To1((H * H - rho * rho - d * d) / (2.0 * r * d));
    ray_r_mu_intersects_ground = false;
  }
  Number x_mu_s =
      getUnitFromTextureCoord(uvwz.y, SCATTERING_TEXTURE_MU_S_SIZE);
  Length d_min = atmosphere.topRadius - atmosphere.bottomRadius;
  Length d_max = H;
  Number A =
      -2.0 * atmosphere.muSunMin * atmosphere.bottomRadius / (d_max - d_min);
  Number a = (A - x_mu_s * A) / (1.0 + x_mu_s * A);
  Length d = d_min + min(a, A) * (d_max - d_min);
  mu_s = d == 0.0 * m ? Number(1.0) :
     clamp0To1((H * H - d * d) / (2.0 * atmosphere.bottomRadius * d));
  nu = clamp0To1(uvwz.x * 2.0 - 1.0);
}
#endif

void GetRMuMuSNuFromScatteringTextureFragCoord(
    IN(AtmosphereParameters) atmosphere, IN(vec3) frag_coord,
    OUT(Length) r, OUT(Number) mu, OUT(Number) mu_s, OUT(Number) nu,
    OUT(bool) ray_r_mu_intersects_ground) {
  const vec4 SCATTERING_TEXTURE_SIZE = vec4(
      SCATTERING_TEXTURE_NU_SIZE - 1,
      SCATTERING_TEXTURE_MU_S_SIZE,
      SCATTERING_TEXTURE_MU_SIZE,
      SCATTERING_TEXTURE_R_SIZE);
  Number frag_coord_nu =
      floor(frag_coord.x / Number(SCATTERING_TEXTURE_MU_S_SIZE));
  Number frag_coord_mu_s =
      mod(frag_coord.x, Number(SCATTERING_TEXTURE_MU_S_SIZE));
  vec4 uvwz =
      vec4(frag_coord_nu, frag_coord_mu_s, frag_coord.y, frag_coord.z) /
          SCATTERING_TEXTURE_SIZE;
  getRMuMuSunNuFromScatteringTextureUVWZ(
      atmosphere, uvwz, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
  nu = clamp(nu, mu * mu_s - sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)),
      mu * mu_s + sqrt((1.0 - mu * mu) * (1.0 - mu_s * mu_s)));
}
void ComputeSingleScatteringTexture(IN(AtmosphereParameters) atmosphere,
    IN(TransmittanceTexture) transmittance_texture,vec3 frag_coord,
    OUT(IrradianceSpectrum) rayleigh, OUT(IrradianceSpectrum) mie) {
  frag_coord.y = SCATTERING_TEXTURE_MU_SIZE - frag_coord.y;

  Length r;
  Number mu;
  Number mu_s;
  Number nu;
  bool ray_r_mu_intersects_ground;
  GetRMuMuSNuFromScatteringTextureFragCoord(atmosphere, frag_coord,
      r, mu, mu_s, nu, ray_r_mu_intersects_ground);
  computeSingleScattering(atmosphere, transmittance_texture,
      r, mu, mu_s, nu, ray_r_mu_intersects_ground, rayleigh, mie);
}

#endif
