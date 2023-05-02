CXXFLAGS += -std=c++20 -I /usr/local/include/SEAL-4.1 -I ../../thirdparty/homfa/thirdparty/TFHEpp/thirdparties/cereal/include 
LDLIBS += -lahomfa_runner -lseal

CXX_DEBUG_FLAGS = -g -O0
CXX_RELEASE_FLAGS = -s -O3 -flto

LDFLAGS+= -L $(AHOMFA_BUILD_PATH)

# debug
.PHONY: debug
debug: CXXFLAGS+=$(CXX_DEBUG_FLAGS)
debug: AHOMFA_BUILD_PATH = ../../cmake-build-debug
debug: all

# release
.PHONY: release
release: CXXFLAGS+=$(CXX_RELEASE_FLAGS)
release: AHOMFA_BUILD_PATH = ../../cmake-build-release
release: all

all: $(AHOMFA_BUILD_PATH)/libahomfa_runner.a 
all: blood_glucose_one

blood_glucose_one: blood_glucose_one.cc

$(AHOMFA_BUILD_PATH)/libahomfa_runner.a:
	cmake --build $(AHOMFA_BUILD_PATH) --target libahomfa_runner.a

.PHONY: clean
clean:
	$(RM) blood_glucose_one
