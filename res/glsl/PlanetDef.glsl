#ifndef SKY_DEF_GLSL
#define SKY_DEF_GLSL

/* 
   mu (greek letter) = cos of view zenith angle
   This corresponds to the angle between a view vector, and the vector
   going from the position to the point on the atmosphere just above
*/

struct DensityLayer {
  float width;
  float expTerm;
  float expScale;
  float linTerm;
  float constTerm;
};

struct DensityProfile {
  DensityLayer layers[2];
};

struct PlanetProperties {
  vec3 solarIrradiance;

  float solarAngularRadius;

  float bottomRadius;
  float topRadius;

  DensityProfile rayleighDensity;
  /* Coefficient where air molecules density is at maximum */
  vec3 rayleighScatteringCoef;

  DensityProfile mieDensity;
  /* Coefficients where aerosol density is at maximum */
  vec3 mieScatteringCoef;
  vec3 mieExtinctionCoef;
  float miePhaseFunctionG;

  DensityProfile absorptionDensity;
  vec3 absorptionExtinctionCoef;

  vec3 groundAlbedo;

  /*
    Cos of the max sun zenith angle for which atmosphere params are 
    precomputed. In this engine's case, to replicate Earth, it's -0.2
    (cos(102 degrees))
   */
  float muSunMin;

  vec3 wPlanetCenter;
};
const int TRANSMITTANCE_TEXTURE_WIDTH = 256;

const int TRANSMITTANCE_TEXTURE_HEIGHT = 64;

const int SCATTERING_TEXTURE_R_SIZE = 32;
const int SCATTERING_TEXTURE_MU_SIZE = 128;
const int SCATTERING_TEXTURE_MU_S_SIZE = 32;
const int SCATTERING_TEXTURE_NU_SIZE = 8;

const int SCATTERING_TEXTURE_WIDTH =
    SCATTERING_TEXTURE_NU_SIZE * SCATTERING_TEXTURE_MU_S_SIZE;
const int SCATTERING_TEXTURE_HEIGHT = SCATTERING_TEXTURE_MU_SIZE;
const int SCATTERING_TEXTURE_DEPTH = SCATTERING_TEXTURE_R_SIZE;

const int IRRADIANCE_TEXTURE_WIDTH = 64;
const int IRRADIANCE_TEXTURE_HEIGHT = 16;

const float PI = 3.14159265358979323846;

#endif
