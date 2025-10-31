MGL_PATH = /Users/tnixc/Developer/MGL/build
ELLE_LIBS ?= ./raylib/src/libraylib.a $(MGL_PATH)/libmgl.dylib $(MGL_PATH)/libglfw.dylib -Wl,-rpath,$(MGL_PATH) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework Metal
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
