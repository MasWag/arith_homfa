all: move15.vrss.txt move16.vrss.txt

specs: reversed_spec spec

reversed_spec: vrss.reversed.spec

spec: vrss.spec

keys: ckks.key tfhe.key ckks.relinkey tfhe.bkey

%.spec: %.ltl ../../cmake-build-release/ahomfa_util
	../../cmake-build-release/ahomfa_util ltl2spec --formula "$(shell cat $<)" --num-vars $(shell grep -o p[0-9] $< | sort | uniq | wc -l) -o $@

%.reversed.spec: %.spec ../../cmake-build-release/ahomfa_util
	../../cmake-build-release/ahomfa_util spec2spec --reverse -i $< -o $@

../../cmake-build-release/ahomfa_util: ../../CMakeLists.txt
	cmake -S ../.. -B ../../cmake-build-release -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
	cmake --build ../../cmake-build-release -j 10

ckks.key: ../../cmake-build-release/ahomfa_util config.json
	../../cmake-build-release/ahomfa_util ckks genkey -c config.json -o $@

tfhe.key: ../../cmake-build-release/ahomfa_util
	../../cmake-build-release/ahomfa_util tfhe genkey -o $@

ckks.relinkey: ../../cmake-build-release/ahomfa_util config.json ckks.key
	../../cmake-build-release/ahomfa_util ckks genrelinkey -c config.json -K ./ckks.key -o $@

tfhe.bkey: ../../cmake-build-release/ahomfa_util config.json ckks.key tfhe.key
	../../cmake-build-release/ahomfa_util tfhe genbkey -c config.json -K ./tfhe.key -S ./ckks.key -o $@

%.vrss.ckks: %.vrss.txt ../../cmake-build-release/ahomfa_util ckks.key config.json
	../../cmake-build-release/ahomfa_util ckks enc -c ./config.json -K ./ckks.key -i $< -o $@

%.vrss.txt: %.json extract_preceding.py
	python3 ./extract_preceding.py < $< > $@

vrss.ltl: ./ltlconv vrss.xltl
	./ltlconv vrss.xltl -o $@

./ltlconv:
	(cd ../../thirdparty/ltlconv && opam install . --deps-only --with-test -y && dune build)
	cp ../../thirdparty/ltlconv/_build/default/bin/main.exe ./ltlconv

clean:
	$(RM) *.vrss.txt
	$(RM) vrss.ltl
	$(RM) ./ltlconv
