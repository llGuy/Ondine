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
/* 
   !!! Make sure to check that line 499 and 500 don't cause problems
   because of flipped Y coordinate of UVS
*/

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
  return vec3(texture(transmittanceTexture, vec2(uv.x, 1.0 - uv.y)));
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

vec3 getScattering(
  in SkyProperties sky,
  in sampler3D scatteringTexture,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround) {
  vec4 uvwz = getScatteringTextureUVWZFromRMuMuSunNu(
    sky, r, mu, muSun, nu, doesRMuIntersectGround);

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
  in SkyProperties sky,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround,
  int scatteringOrder) {
  if (scatteringOrder == 1) {
    vec3 rayleigh = getScattering(
      sky, singleRayleighScatteringTexture, r, mu, muSun, nu,
      doesRMuIntersectGround);

    vec3 mie = getScattering(
      sky, singleMieScatteringTexture, r, mu, muSun, nu,
      doesRMuIntersectGround);

    return rayleigh * rayleighPhase(nu) +
      mie * miePhase(sky.miePhaseFunctionG, nu);
  }
  else {
    return getScattering(
      sky, multipleScatteringTexture, r, mu,
      muSun, nu, doesRMuIntersectGround);
  }
}

#if 0
vec3 getScattering(
    in SkyProperties atmosphere,
    sampler3D scattering_texture,
    Length r, Number mu, Number mu_s, Number nu,
    bool ray_r_mu_intersects_ground) {
  vec4 uvwz = getScatteringTextureUVWZFromRMuMuSunNu(
      atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
  Number tex_coord_x = uvwz.x * Number(SCATTERING_TEXTURE_NU_SIZE - 1);
  Number tex_x = floor(tex_coord_x);
  Number lerp = tex_coord_x - tex_x;
  vec3 uvw0 = vec3((tex_x + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  return AbstractSpectrum(texture(scattering_texture, uvw0) * (1.0 - lerp) +
      texture(scattering_texture, uvw1) * lerp);
}

RadianceSpectrum getScattering(
    in SkyProperties atmosphere,
    in ReducedScatteringTexture single_rayleigh_scattering_texture,
    in ReducedScatteringTexture single_mie_scattering_texture,
    in ScatteringTexture multiple_scattering_texture,
    Length r, Number mu, Number mu_s, Number nu,
    bool ray_r_mu_intersects_ground,
    int scattering_order) {
  if (scattering_order == 1) {
    IrradianceSpectrum rayleigh = getScattering(
        atmosphere, single_rayleigh_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    IrradianceSpectrum mie = getScattering(
        atmosphere, single_mie_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    return rayleigh * rayleighPhase(nu) +
        mie * miePhase(atmosphere.miePhaseFunctionG, nu);
  } else {
    return getScattering(
        atmosphere, multiple_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
  }
}
#endif

vec3 getIrradiance(
  in SkyProperties sky,
  in sampler2D irradianceTexture,
  float r, float muSun);

#if 0
vec3 computeScatteringDensity(
    in SkyProperties atmosphere,
    in sampler2D transmittance_texture,
    in sampler3D single_rayleigh_scattering_texture,
    in sampler3D single_mie_scattering_texture,
    in sampler3D multiple_scattering_texture,
    in sampler2D irradiance_texture,
    float r, float mu, float mu_s, float nu, int scattering_order) {
  // Compute unit direction vectors for the zenith, the view direction omega and
  // and the sun direction omega_s, such that the cosine of the view-zenith
  // angle is mu, the cosine of the sun-zenith angle is mu_s, and the cosine of
  // the view-sun angle is nu. The goal is to simplify computations below.
  vec3 zenith_direction = vec3(0.0, 0.0, 1.0);
  vec3 omega = vec3(sqrt(1.0 - mu * mu), 0.0, mu);
  Number sun_dir_x = omega.x == 0.0 ? 0.0 : (nu - mu * mu_s) / omega.x;
  Number sun_dir_y = sqrt(max(1.0 - sun_dir_x * sun_dir_x - mu_s * mu_s, 0.0));
  vec3 omega_s = vec3(sun_dir_x, sun_dir_y, mu_s);

  const int SAMPLE_COUNT = 16;
  const Angle dphi = pi / Number(SAMPLE_COUNT);
  const Angle dtheta = pi / Number(SAMPLE_COUNT);
  RadianceDensitySpectrum rayleigh_mie =
      RadianceDensitySpectrum(0.0 * watt_per_cubic_meter_per_sr_per_nm);

  // Nested loops for the integral over all the incident directions omega_i.
  for (int l = 0; l < SAMPLE_COUNT; ++l) {
    Angle theta = (Number(l) + 0.5) * dtheta;
    Number cos_theta = cos(theta);
    Number sin_theta = sin(theta);
    bool ray_r_theta_intersects_ground =
        doesRayIntersectGround(atmosphere, r, cos_theta);

    // The distance and transmittance to the ground only depend on theta, so we
    // can compute them in the outer loop for efficiency.
    Length distance_to_ground = 0.0 * m;
    DimensionlessSpectrum transmittance_to_ground = DimensionlessSpectrum(0.0);
    DimensionlessSpectrum ground_albedo = DimensionlessSpectrum(0.0);
    if (ray_r_theta_intersects_ground) {
      distance_to_ground =
          distToGroundBoundary(atmosphere, r, cos_theta);
      transmittance_to_ground =
          getTransmittance(atmosphere, transmittance_texture, r, cos_theta,
              distance_to_ground, true /* ray_intersects_ground */);
      ground_albedo = atmosphere.groundAlbedo;
    }

    for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
      Angle phi = (Number(m) + 0.5) * dphi;
      vec3 omega_i =
          vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);
      SolidAngle domega_i = (dtheta / rad) * (dphi / rad) * sin(theta) * sr;

      // The radiance L_i arriving from direction omega_i after n-1 bounces is
      // the sum of a term given by the precomputed scattering texture for the
      // (n-1)-th order:
      Number nu1 = dot(omega_s, omega_i);
      RadianceSpectrum incident_radiance = getScattering(atmosphere,
          single_rayleigh_scattering_texture, single_mie_scattering_texture,
          multiple_scattering_texture, r, omega_i.z, mu_s, nu1,
          ray_r_theta_intersects_ground, scattering_order - 1);

      // and of the contribution from the light paths with n-1 bounces and whose
      // last bounce is on the ground. This contribution is the product of the
      // transmittance to the ground, the ground albedo, the ground BRDF, and
      // the irradiance received on the ground after n-2 bounces.
      vec3 ground_normal =
          normalize(zenith_direction * r + omega_i * distance_to_ground);
      IrradianceSpectrum ground_irradiance = getIrradiance(
          atmosphere, irradiance_texture, atmosphere.bottomRadius,
          dot(ground_normal, omega_s));
      incident_radiance += transmittance_to_ground *
          ground_albedo * (1.0 / (PI * sr)) * ground_irradiance;

      // The radiance finally scattered from direction omega_i towards direction
      // -omega is the product of the incident radiance, the scattering
      // coefficient, and the phase function for directions omega and omega_i
      // (all this summed over all particle types, i.e. Rayleigh and Mie).
      Number nu2 = dot(omega, omega_i);
      Number rayleigh_density = getProfileDensity(
          atmosphere.rayleighDensity, r - atmosphere.bottomRadius);
      Number mie_density = getProfileDensity(
          atmosphere.mieDensity, r - atmosphere.bottomRadius);
      rayleigh_mie += incident_radiance * (
          atmosphere.rayleighScatteringCoef * rayleigh_density *
              rayleighPhase(nu2) +
          atmosphere.mieScatteringCoef * mie_density *
              miePhase(atmosphere.miePhaseFunctionG, nu2)) *
          domega_i;
    }
  }
  return rayleigh_mie;
}
#endif

vec3 computeScatteringDensity(
  in SkyProperties sky, in sampler2D transmittanceTexture,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  in sampler2D irradianceTexture,
  float r, float mu, float muSun, float nu, int scatteringOrder) {
  vec3 zenithDirection = vec3(0.0, 0.0, 1.0);
  vec3 omega = vec3(sqrt(1.0 - mu * mu), 0.0, mu);

  float sunDirX = (omega.x == 0.0) ? 0.0 : (nu - mu * muSun) / omega.x;
  float sunDirY = sqrt(max(1.0 - sunDirX * sunDirX - muSun * muSun, 0.0));

  vec3 omegaSun = vec3(sunDirX, sunDirY, muSun);

  const int SAMPLE_COUNT = 16;

  float dphi = PI / float(SAMPLE_COUNT);
  float dtheta = PI / float(SAMPLE_COUNT);

  vec3 rayleighMie = vec3(0.0);

  for (int l = 0; l < SAMPLE_COUNT; ++l) {
    float theta = (float(l) + 0.5) * dtheta;
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    bool doesRThetaIntersectGround = doesRayIntersectGround(sky, r, cosTheta);

    float distToGround = 0.0;
    vec3 transmittanceToGround = vec3(0.0);
    vec3 groundAlbedo = vec3(0.0);

    if (doesRThetaIntersectGround) {
      distToGround = distToGroundBoundary(sky, r, cosTheta);

      transmittanceToGround = getTransmittance(
        sky, transmittanceTexture, r, cosTheta, distToGround, true);

      groundAlbedo = sky.groundAlbedo;
    }

    for (int m = 0; m < 2 * SAMPLE_COUNT; ++m) {
      float phi = (float(m) + 0.5) * dphi;

      vec3 omegaI = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

      float domegaI = dtheta * dphi * sin(theta);

      float nu1 = dot(omegaSun, omegaI);

      vec3 incidentRadiance = getScattering(
        sky, singleRayleighScatteringTexture,
        singleMieScatteringTexture, multipleScatteringTexture,
        r, omegaI.z, muSun, nu1, doesRThetaIntersectGround, scatteringOrder - 1);

      vec3 groundNormal = normalize(zenithDirection * r + omegaI * distToGround);

      vec3 groundIrradiance = getIrradiance(
        sky, irradianceTexture, sky.bottomRadius,
        dot(groundNormal, omegaSun));

      incidentRadiance += transmittanceToGround * groundAlbedo *
        (1.0 / PI) * groundIrradiance;

      float nu2 = dot(omega, omegaI);

      float rayleighDensity = getProfileDensity(
        sky.rayleighDensity, r - sky.bottomRadius);

      float mieDensity = getProfileDensity(
        sky.mieDensity, r - sky.bottomRadius);

      rayleighMie += incidentRadiance * (
        sky.rayleighScatteringCoef * rayleighDensity *
        rayleighPhase(nu2) +
        sky.mieScatteringCoef * mieDensity *
        miePhase(sky.miePhaseFunctionG, nu2)) * domegaI;
    }
  }

  return rayleighMie;
}

vec3 computeMultipleScattering(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringDensityTexture,
  float r, float mu, float muSun, float nu,
  bool doesRMuIntersectGround) {
  const int SAMPLE_COUNT = 50;

  float dx = distToNearestBoundary(
    sky, r, mu, doesRMuIntersectGround) / float(SAMPLE_COUNT);

  vec3 rayleighMieSum = vec3(0.0);

  for (int i = 0; i <= SAMPLE_COUNT; ++i) {
    float currentD = float(i) * dx;

    float currentR = clampRadius(
      sky, sqrt(currentD * currentD + 2.0 * r * mu * currentD + r * r));

    float currentMu = clamp0To1((r * mu * currentD) / currentR);
    float currentMuSun = clamp0To1((r * muSun + currentD * nu) / currentR);

    vec3 rayleighMieI =
      getScattering(
        sky, scatteringDensityTexture, currentR, currentMu,
        currentMuSun, nu, doesRMuIntersectGround) *
      getTransmittance(
        sky, transmittanceTexture, r, mu, currentD, doesRMuIntersectGround) * dx;

    float weightI = (i == 0 || i == SAMPLE_COUNT) ? 0.5 : 1.0;
    rayleighMieSum += rayleighMieI * weightI;
  }

  return rayleighMieSum;
}

vec3 computeScatteringDensityTexture(
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D singleRayleighScatteringTexture,
  in sampler3D singleMieScatteringTexture,
  in sampler3D multipleScatteringTexture,
  in sampler2D irradianceTexture,
  in vec3 fragCoord, int scatteringOrder) {
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
  in SkyProperties sky,
  in sampler2D transmittanceTexture,
  in sampler3D scatteringDensityTexture,
  in vec3 fragCoord, out float nu) {
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
  in SkyProperties sky,
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
  in SkyProperties sky,
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
  in SkyProperties sky, float r, float muSun) {
  float rMapping = (r - sky.bottomRadius) /
    (sky.topRadius - sky.bottomRadius);
  float muSunMapping = muSun * 0.5 + 0.5;

  return vec2(
    getTextureCoordFromUnit(muSunMapping, IRRADIANCE_TEXTURE_WIDTH),
    getTextureCoordFromUnit(rMapping, IRRADIANCE_TEXTURE_HEIGHT));
}

void getRMuSunFromIrradianceTextureUV(
  in SkyProperties sky,
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
  in SkyProperties sky,
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
  in SkyProperties sky,
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
  in SkyProperties sky,
  in sampler2D irradianceTexture,
  float r, float muSun) {
  vec2 uv = getIrradianceTextureUVFromRMuSun(sky, r, muSun);
  return vec3(texture(irradianceTexture, vec2(uv.x, 1.0 - uv.y)));
}

#endif
