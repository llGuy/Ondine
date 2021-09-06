#pragma once

namespace Ondine::Graphics {

/* Caching system used to avoid needing to precompute stuff over and over */
extern const char *const DRAW_CACHE_DIRECTORY;
extern const char *const SKY_CACHE_DIRECTORY;

/* Not path */
extern const char *const SKY_TRANSMITTANCE_CACHE_FILENAME;
extern const char *const SKY_SCATTERING_CACHE_FILENAME;
extern const char *const SKY_MIE_SCATTERING_CACHE_FILENAME;
extern const char *const SKY_IRRADIANCE_CACHE_FILENAME;
 
}
