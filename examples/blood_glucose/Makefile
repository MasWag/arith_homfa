all: adult\#001_night.bg.txt adult\#001_2nd_7days.bg.txt

reversed_spec: bg1.reversed.spec bg2.reversed.spec bg7.reversed.spec bg8.reversed.spec bg10.reversed.spec bg11.reversed.spec

spec: bg1.spec bg2.spec bg4.spec bg5.spec bg7.spec bg8.spec bg10.spec bg11.spec

%.spec: %.ltl ../cmake-build-release/arith_homfa/ahomfa_util
	../cmake-build-release/arith_homfa/ahomfa_util ltl2spec --formula "$(shell cat $<)" --num-vars $(shell grep -o p[0-9] $< | sort | uniq | wc -l) -o $@

%.reversed.spec: %.spec ../cmake-build-release/arith_homfa/ahomfa_util
	../cmake-build-release/arith_homfa/ahomfa_util spec2spec --reverse -i $< -o $@

../cmake-build-release/arith_homfa/ahomfa_util: ../CMakeLists.txt
	cd ..
	cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
	CC=/usr/bin/gcc-10 CXX=/usr/bin/g++-10 cmake --build cmake-build-release -j 10

ckks.key: ../cmake-build-release/arith_homfa/ahomfa_util config.json
	../cmake-build-release/arith_homfa/ahomfa_util ckks genkey -c config.json -o $@

tfhe.key: ../cmake-build-release/arith_homfa/ahomfa_util
	../cmake-build-release/arith_homfa/ahomfa_util tfhe genkey -o $@

ckks.relinkey: ../cmake-build-release/arith_homfa/ahomfa_util config.json ckks.key
	../cmake-build-release/arith_homfa/ahomfa_util ckks genrelinkey -c config.json -K ./ckks.key -o $@

tfhe.bkey: ../cmake-build-release/arith_homfa/ahomfa_util config.json ckks.key tfhe.key
	../cmake-build-release/arith_homfa/ahomfa_util tfhe genbkey -c config.json -K ./tfhe.key -S ./ckks.key -o $@

%.bg.ckks: %.bg.txt ../cmake-build-release/arith_homfa/ahomfa_util ckks.key config.json
	../cmake-build-release/arith_homfa/ahomfa_util ckks enc -c ./config.json -K ./ckks.key -i $< -o $@

%.bg.txt: homfa-experiment/ap9/%.csv
	cut -d , -f 2 $< | sed '1 d' > $@

clean:
	$(RM) adult\#001_night.bg.txt adult\#001_2nd_7days.bg.txt
