MGL_PATH = /Users/tnixc/Developer/MGL/build
# Raylib includes its own GLFW, so we don't link MGL's GLFW separately to avoid duplication
ELLE_LIBS ?= ./raylib/src/libraylib.a $(MGL_PATH)/libmgl.dylib -Wl,-rpath,$(MGL_PATH) -framework Cocoa -framework IOKit -framework CoreVideo -framework Metal
ELLE_FLAGS ?= -o raytracer

default: raytracer

.PHONY: run
run: raytracer
	$<

.PHONY: raytracer
raytracer: src/main.le
	ellec $< $(ELLE_FLAGS) $(foreach L,$(ELLE_LIBS),-z $(L))

.PHONY: burning_ship
burning_ship: examples/burning_ship.le
	ellec $< -o burning_ship $(foreach L,$(ELLE_LIBS),-z $(L))
