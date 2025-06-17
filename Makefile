# print arbitrary variables with $ make print - < name>
print-%:; @echo $($*)

UNAME_S = $(shell uname -s)

CC = clang++
LD = clang++

# CONFIG_UBSAN = 1
CONFIG_ME = 1
# CONFIG_MOLD = 1
CONFIG_CCACHE = 1

# library paths
PATH_LIB		    = lib
PATH_GLM		    = $(PATH_LIB)/glm
PATH_FMT		    = $(PATH_LIB)/fmt
PATH_BGFX		    = $(PATH_LIB)/bgfx
PATH_BX			    = $(PATH_LIB)/bx
PATH_BIMG		    = $(PATH_LIB)/bimg
PATH_GLFW		    = $(PATH_LIB)/glfw
PATH_FCPP		    = $(PATH_LIB)/bgfx/3rdparty/fcpp
PATH_TOMLPLUSPLUS	= $(PATH_LIB)/tomlplusplus
PATH_NOISE		    = $(PATH_LIB)/noise
PATH_BACKWARD_CPP	= $(PATH_LIB)/backward-cpp
PATH_SOLOUD		    = $(PATH_LIB)/soloud
PATH_GLFW		    = $(PATH_LIB)/glfw
PATH_MOLTENVK		= $(PATH_LIB)/MoltenVK
PATH_ARCHIMEDES		= $(PATH_LIB)/archimedes
PATH_NAMEOF			= $(PATH_LIB)/nameof
PATH_LZ4 			= $(PATH_LIB)/lz4

ARCHIMEDES_PLUGIN	= $(PATH_ARCHIMEDES)/bin/libarchimedes-plugin.dylib

# allow for non-relative includes, based in src
INCFLAGS += -iquotesrc

# library include paths
INCFLAGS += -I$(PATH_GLM)
INCFLAGS += -I$(PATH_BGFX)/include
INCFLAGS += -I$(PATH_BX)/include
INCFLAGS += -I$(PATH_BIMG)/include
INCFLAGS += -I$(PATH_GLFW)/include
INCFLAGS += -I$(PATH_FCPP)
INCFLAGS += -I$(PATH_TOMLPLUSPLUS)
INCFLAGS += -I$(PATH_NOISE)
INCFLAGS += -I$(PATH_BACKWARD_CPP)
INCFLAGS += -I$(PATH_FMT)/include
INCFLAGS += -I$(PATH_SOLOUD)/include
INCFLAGS += -I$(PATH_ARCHIMEDES)/include
INCFLAGS += -I$(PATH_NAMEOF)/include
INCFLAGS += -I$(PATH_LZ4)/lib
INCFLAGS += -iquoteres/shaders

ifdef CCFLAGS_CUSTOM
	CCFLAGS = $(CCFLAGS_CUSTOM)
else
	CCFLAGS =
endif

CCFLAGS += -std=c++20 -stdlib=libc++
CCFLAGS += -O1 -g -Wall -Wextra -Wpedantic
CCFLAGS += -Wno-c99-extensions
CCFLAGS += -Wno-unused-parameter
CCFLAGS += -Wno-vla-extension
CCFLAGS += -Wno-c++11-extensions
CCFLAGS += -Wno-gnu-statement-expression
CCFLAGS += -Wno-gnu-zero-variadic-macro-arguments
CCFLAGS += -Wno-nested-anon-types
CCFLAGS += -Wno-gnu-anonymous-struct

ifdef CONFIG_UBSAN
CCFLAGS += -fsanitize=undefined,address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=undefined,address
endif

### TARGET SPECIFICS ###

# TODO : add more targets
# possible values : osx_arm64, osx_x86_64 < add more>
TARGET = osx_arm64

ifeq ($(TARGET),osx_arm64)
	BGFX_TARGET		    = osx-arm
	BGFX_DEPS_TARGET	= osx-arm64
	SHADER_TARGET		= spirv
	SHADER_PLATFORM		= linux
	LDFLAGS	+= $(PATH_GLFW)/src/libglfw3.a
endif

# OSX
ifeq ($(findstring osx,$(TARGET)),osx)
	FRAMEWORKS =
	FRAMEWORKS	+= -framework Foundation
	FRAMEWORKS	+= -framework QuartzCore
	FRAMEWORKS	+= -framework Cocoa
	FRAMEWORKS	+= -framework Carbon
	FRAMEWORKS	+= -framework Metal
	FRAMEWORKS	+= -framework CoreFoundation
	FRAMEWORKS	+= -framework IOKit
	FRAMEWORKS	+= -framework CoreServices
	FRAMEWORKS	+= -framework CoreAudio
	FRAMEWORKS	+= -framework CoreHaptics
	FRAMEWORKS	+= -framework AudioUnit
	FRAMEWORKS	+= -framework AudioToolbox
	FRAMEWORKS	+= -framework GameController
	FRAMEWORKS	+= -framework ForceFeedback
	CCFLAGS		+= -I/opt/homebrew/include
	LDFLAGS		+= -liconv
	USE_GLFW	    = 1
	USE_MOLTENVK	= 1
	USE_OPENMP	    = 1
endif


# me-specific
ifdef CONFIG_ME
	CC = $(shell brew --prefix llvm)/bin/clang++
	LD = $(CC)
	LDFLAGS += -L$(shell brew --prefix llvm)/lib/
endif

ifdef CONFIG_CCACHE
	CCACHE = ccache
else
	CCACHE =
endif

### END TARGET SPECIFICS ###

ifdef USE_OPENMP
CCFLAGS += -fopenmp=libomp
LDFLAGS += -fopenmp=libomp -lomp
endif

ifdef CONFIG_MOLD
LDFLAGS += -fuse-ld=/opt/mold/ld64
endif

BIN = bin

SRC = $(shell find src -name "*.cpp")
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)

TEST_DIR 			= test
TEST_SRC 			= $(shell find $(TEST_DIR) -name "*.test.cpp")
TEST_DEP 			= $(TEST_SRC:.cpp=.d)
TEST_OBJ 			= $(TEST_SRC:.cpp=.o)
TEST_OUT 			= $(TEST_SRC:.test.cpp=)
TEST_OUT_NAMES 		= $(notdir $(TEST_OUT))

TEST_RUNNER_SRC = $(TEST_DIR)/test.cpp
TEST_RUNNER_DEP = $(TEST_RUNNER_SRC:.cpp=.d)
TEST_RUNNER_OUT = $(TEST_RUNNER_SRC:.cpp=)

# largest header (util.hpp) gets compiled into a pch to reduce parsing times
PCHHDR	= src/util/util.hpp
PCH	    = $(PCHHDR:.hpp=.hpp.pch)
PCH_DEP	= $(PCHHDR:.hpp=.hpp.d)
PCHINC	= $(addprefix -include-pch ,$(PCH))

BGFX_BIN	= $(PATH_BGFX)/.build/$(BGFX_DEPS_TARGET)/bin
BGFX_CONFIG	= Debug

SHADERS_PATH	= res/shaders
SHADERS		    = $(shell find $(SHADERS_PATH)/* -maxdepth 1 | grep -E ".*/(vs|fs).*.sc")
SHADERS_DEP	    = $(SHADERS:.sc=.spirv.bin.d)
SHADERS_OUT	    = $(SHADERS:.sc=.$(SHADER_TARGET).bin)
SHADERC		    = $(PATH_BGFX)/.build/$(BGFX_DEPS_TARGET)/bin/shaderc$(BGFX_CONFIG)

# add defines for specific platforms, shader targets
CCFLAGS += -DGAME_TARGET_$(TARGET)
CCFLAGS += -DSHADER_TARGET_$(SHADER_TARGET)
CCFLAGS += -DSHADER_PLATFORM_$(SHADER_PLATFORM)

LDFLAGS += -lm
LDFLAGS += -lstdc++
LDFLAGS += $(BGFX_BIN)/libbgfx$(BGFX_CONFIG).a
LDFLAGS += $(BGFX_BIN)/libbimg$(BGFX_CONFIG).a
LDFLAGS += $(BGFX_BIN)/libbx$(BGFX_CONFIG).a
LDFLAGS += $(BGFX_BIN)/libfcpp$(BGFX_CONFIG).a
LDFLAGS += $(PATH_NOISE)/libnoise.a
LDFLAGS += $(PATH_FMT)/build/libfmt.a
LDFLAGS += $(PATH_SOLOUD)/lib/libsoloud_static.a
LDFLAGS += $(PATH_ARCHIMEDES)/bin/libarchimedes.a
LDFLAGS += $(PATH_LZ4)/lib/liblz4.a

ifdef FRAMEWORKS
LDFLAGS	+= $(FRAMEWORKS)
export LD_PATH := $(FRAMEWORKS):$$LD_PATH
export LD_LIBRARY_PATH := $(FRAMEWORKS):$$LD_LIBRARY_PATH
endif

all: dirs libs shaders build

lib-frameworks:
ifdef FRAMEWORKS
	export LD_PATH="$(FRAMEWORKS)"
	export LD_LIBRARY_PATH="$(FRAMEWORKS)"
	export LIBRARY_PATH="$(FRAMEWORKS)"
endif

lib-bx:
	cd $(PATH_BX) && make $(BGFX_DEPS_TARGET)

lib-bimg:
	cd $(PATH_BIMG) && make $(BGFX_DEPS_TARGET)

lib-bgfx:
	cd $(PATH_BGFX) && make $(BGFX_TARGET)

lib-noise:
	cd $(PATH_NOISE) && make

lib-fmt:
	cd $(PATH_FMT) && mkdir -p build && cd build && cmake .. && make

lib-soloud:
	# remove "NoRTTI" from soloud build files
	sed -i '' 's/\, \"NoRTTI"\"//g' lib/soloud/build/genie.lua
	cd $(PATH_SOLOUD) && genie --file=build/genie.lua gmake && cd build/gmake && make SoloudStatic

lib-glfw:
ifdef USE_GLFW
	cd $(PATH_GLFW) && cmake . && make
endif

lib-moltenvk:
ifdef USE_MOLTENVK
	cd $(PATH_MOLTENVK) && make macos
	ln -s $(PATH_MOLTENVK)/MoltenVK/dylib/macOS/libMoltenVK.dylib libvulkan.dylib
endif

lib-archimedes:
	cd $(PATH_ARCHIMEDES) && make static plugin

lib-lz4:
	cd $(PATH_ARCHIMEDES) && make

libs: lib-frameworks lib-bx lib-bimg lib-bgfx lib-noise lib-fmt lib-soloud lib-glfw lib-moltenvk lib-archimedes lib-lz4

$(BIN):
	mkdir -p $@

dirs: $(BIN)

%.$(SHADER_TARGET).bin: %.sc
	$(SHADERC)	--type $(shell echo $(notdir $@) | cut -c 1)	\
			-i $(PATH_BGFX)/src				                    \
			-i res/shaders					                    \
			-i src						                        \
			--platform $(SHADER_PLATFORM)			            \
			--profile $(SHADER_TARGET)			                \
			--varyingdef $(dir $@)varying.def.sc		        \
			--depends					                        \
			-f $<						                        \
			-o $@

shaders: $(SHADERS_OUT)

-include $(DEP) $(PCH_DEP) $(SHADERS_DEP) $(TEST_DEP) $(TEST_RUNNER_DEP)

$(OBJ): %.o: %.cpp $(PCH)
	$(CCACHE) $(CC) -x c++ -o $(<:.cpp=.o) -MMD -c $(PCHINC) $(CCFLAGS) $(INCFLAGS) 	 \
		-fplugin=$(ARCHIMEDES_PLUGIN) 							                         \
		-fplugin-arg-archimedes-config-archimedes.txt 					                 \
		-fplugin-arg-archimedes-file-$<							                         \
		-fplugin-arg-archimedes-file-$(<:.cpp=.hpp)                                      \
		-fplugin-arg-archimedes-out-$(<:.cpp=.types.o)					                 \
		$<

$(TEST_OBJ): %.o: %.cpp $(PCH)
	$(CCACHE) $(CC) -x c++ -o $(<:.cpp=.o) -MMD -c $(CCFLAGS) -O0 $(INCFLAGS) $<
		# -fsanitize=undefined,address -fno-omit-frame-pointer  			  	  \
		$<

$(TEST_OUT): %: %.test.o
	$(LD) -o $@ $(LDFLAGS)           $<
		# -fsanitize=undefined,address \

$(TEST_RUNNER_OUT): %: %.cpp
	$(CCACHE) $(CC) -o $@ $(CCFLAGS) $<

test: $(TEST_RUNNER_OUT) $(TEST_OUT) FORCE
	./$(TEST_RUNNER_OUT) $(TEST_DIR)

test-%: $(TEST_DIR)/% $(TEST_RUNNER_OUT)
	./$(TEST_RUNNER_OUT) $(TEST_DIR) $(notdir $<)

%.hpp.pch: %.hpp
	$(CCACHE) $(CC) -x c++-header -o $@ -MMD -c $< $(CCFLAGS) $(INCFLAGS)

run: build
	$(BIN)/game

build: shaders $(OBJ) | dirs
	$(LD) -o $(BIN)/game $(filter %.o,$^) $(shell find src -name "*.types.o") $(LDFLAGS)

deps: $(DEPS) $(PCH_DEP) $(SHADERS_DEP)

clean-shaders:
	cd res && find . -name "*.bin" -delete
	cd res/shaders && find . -name "*.d" -delete

clean-deps:
	cd src && find . -name "*.d" -delete

clean-pch:
	cd src && find . -name "*.hpp.pch" -delete

clean-tmp:
	cd src && find . -name "*.tmp" -delete

clean-test:
	cd $(TEST_DIR) && find . -name "*.o" -delete
	cd $(TEST_DIR) && find . -type f ! -name '*.*' -delete

clean: clean-tmp clean-shaders clean-deps clean-pch clean-test
	cd src && find . -name "*.o" -delete
	rm -rf $(BIN)

time-trace:
	make clean
	ClangBuildAnalyzer --start src
	make build -j 10 CCFLAGS_CUSTOM=-ftime-trace
	ClangBuildAnalyzer --stop src out.json
	ClangBuildAnalyzer --analyze out.json
	rm out.json
	rm src/ClangBuildAnalyzerSession.txt
	find src -name "*.json" -delete

FORCE:
