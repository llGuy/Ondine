# Building "Ondine"

## Application

Make sure to call `git submodule update --init --recursive` after cloning.

TODO (CMake).

## Shaders

Need to build using `glslc` compiler.

Here is an example Makefile.

```makefile
GLSLC := glslc

VERTSOURCE := $(wildcard *.vert)
GEOMSOURCE := $(wildcard *.geom)
FRAGSOURCE := $(wildcard *.frag)

SPVDIR := ../spv/

VERTSPV := $(patsubst %,$(SPVDIR)%,$(VERTSOURCE:.vert=.vert.spv))
GEOMSPV := $(patsubst %,$(SPVDIR)%,$(GEOMSOURCE:.geom=.geom.spv))
FRAGSPV := $(patsubst %,$(SPVDIR)%,$(FRAGSOURCE:.frag=.frag.spv))

all: clean $(VERTSPV) $(GEOMSPV) $(FRAGSPV)
	

$(SPVDIR)%.vert.spv: %.vert
	$(GLSLC) -c --target-env=vulkan -fshader-stage=vert -o $@ $^

$(SPVDIR)%.geom.spv: %.geom
	$(GLSLC) -c --target-env=vulkan -fshader-stage=geom -o $@ $^

$(SPVDIR)%.frag.spv: %.frag
	$(GLSLC) -c --target-env=vulkan -fshader-stage=frag -o $@ $^

clean:
	rm ../spv/*spv

run:
	(cd ../../build && ./Ondine)
```
