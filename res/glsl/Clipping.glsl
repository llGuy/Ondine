#ifndef CLIPPING_GLSL
#define CLIPPING_GLSL

struct Clipping {
  /* 
     For ocean planar reflections
     1.0 = everything under the planet gets discarded,
     -1.0 = everything above the planet gets discarded
  */
  float clipUnderPlanet;
  float clippingRadius;
};

#endif
