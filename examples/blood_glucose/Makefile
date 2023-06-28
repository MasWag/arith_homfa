CXXFLAGS += -std=c++2a -I /usr/local/include/SEAL-4.1 -I ../../thirdparty/homfa/thirdparty/TFHEpp/thirdparties/cereal/include 
LDLIBS += -lahomfa_runner -fopenmp -lseal $(shell pkg-config libspot --libs) -ltfhe++ -ltbb -lspqlios -lranden

CXX_DEBUG_FLAGS = -g -O0
CXX_RELEASE_FLAGS = -s -O3 -flto

LDFLAGS+= -L $(AHOMFA_BUILD_PATH) -L $(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/src/ -L $(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/thirdparties/spqlios/ -L $(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/thirdparties/randen/

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

.PHONY: all
all: $(AHOMFA_BUILD_PATH)/libahomfa_runner.a $(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/src/libtfhe++.a $(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/thirdparties/spqlios/libspqlios.a blood_glucose_one

blood_glucose_one: blood_glucose_one.cc

$(AHOMFA_BUILD_PATH)/libahomfa_runner.a:
	cmake --build $(AHOMFA_BUILD_PATH) --target libahomfa_runner.a

$(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/src/libtfhe++.a:
	cmake --build $(AHOMFA_BUILD_PATH) --target thirdparty/homfa/thirdparty/TFHEpp/src/libtfhe++.a

$(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/thirdparties/spqlios/libspqlios.a:
	cmake --build $(AHOMFA_BUILD_PATH) --target thirdparty/homfa/thirdparty/TFHEpp/thirdparties/spqlios/libspqlios.a

$(AHOMFA_BUILD_PATH)/thirdparty/homfa/thirdparty/TFHEpp/thirdparties/randen/libranden.a:
	cmake --build $(AHOMFA_BUILD_PATH) --target /thirdparty/homfa/thirdparty/TFHEpp/thirdparties/randen/libranden.a

.PHONY: clean
clean:
	$(RM) blood_glucose_one
