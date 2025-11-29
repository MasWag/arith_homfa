# Makefile for Vehicle RSS example
# Handles key generation, data encryption, and spec file generation

AHOMFA_UTIL := ../../build/src/ahomfa_util
BUILD_DIR := ../../build

all: keys specs

keys: ckks.key tfhe.key ckks.relinkey tfhe.bkey

specs: vrss.spec vrss.reversed.spec

# Generate LTL spec from formula
%.spec: %.ltl $(AHOMFA_UTIL)
	$(AHOMFA_UTIL) ltl2spec --formula "$$(cat $<)" --num-vars $$(grep -o p[0-9] $< | sort | uniq | wc -l) -o $@

# Generate reversed spec for reverse monitoring
%.reversed.spec: %.spec $(AHOMFA_UTIL)
	$(AHOMFA_UTIL) spec2spec --reverse -i $< -o $@

# Build ahomfa_util if not exists
$(AHOMFA_UTIL):
	@echo "Building ahomfa_util..."
	@mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=Release
	cd $(BUILD_DIR) && make ahomfa_util -j4

# Generate CKKS key
ckks.key: $(AHOMFA_UTIL) config.json
	$(AHOMFA_UTIL) ckks genkey -c config.json -o $@

# Generate TFHE key
tfhe.key: $(AHOMFA_UTIL)
	$(AHOMFA_UTIL) tfhe genkey -o $@

# Generate CKKS relinearization key
ckks.relinkey: $(AHOMFA_UTIL) config.json ckks.key
	$(AHOMFA_UTIL) ckks genrelinkey -c config.json -K ./ckks.key -o $@

# Generate TFHE bootstrapping key
tfhe.bkey: $(AHOMFA_UTIL) config.json ckks.key tfhe.key
	$(AHOMFA_UTIL) tfhe genbkey -c config.json -K ./tfhe.key -S ./ckks.key -o $@

# Encrypt vehicle data (expects move15.vrss.txt input file)
%.vrss.ckks: %.vrss.txt $(AHOMFA_UTIL) ckks.key config.json
	$(AHOMFA_UTIL) ckks enc -c ./config.json -K ./ckks.key -i $< -o $@

# Generate sample data file
sample_data:
	@echo "Generating sample vehicle data..."
	@echo "25.5 15.2 3.1 2.8" > move15.vrss.txt
	@echo "28.3 12.5 2.8 3.2" >> move15.vrss.txt
	@echo "30.1 11.0 2.5 3.5" >> move15.vrss.txt
	@echo "32.5 9.8 2.2 4.1" >> move15.vrss.txt
	@echo "29.8 10.5 2.6 3.0" >> move15.vrss.txt
	@echo "27.2 13.0 2.9 2.5" >> move15.vrss.txt
	@echo "26.0 14.5 3.0 2.2" >> move15.vrss.txt
	@echo "24.5 16.0 3.2 1.8" >> move15.vrss.txt
	@echo "23.0 18.5 3.4 1.5" >> move15.vrss.txt
	@echo "22.5 20.0 3.5 1.2" >> move15.vrss.txt
	@echo "21.8 22.0 3.6 1.0" >> move15.vrss.txt
	@echo "20.5 24.5 3.7 0.8" >> move15.vrss.txt
	@echo "19.2 27.0 3.8 0.5" >> move15.vrss.txt
	@echo "18.0 30.0 3.9 0.3" >> move15.vrss.txt
	@echo "17.5 32.5 4.0 0.1" >> move15.vrss.txt
	@echo "Sample data created: move15.vrss.txt"

# Encrypt sample data
encrypt_sample: sample_data move15.vrss.ckks
	@echo "Sample data encrypted: move15.vrss.ckks"

clean:
	$(RM) *.key *.bkey *.relinkey *.spec *.ckks *.txt

.PHONY: all keys specs sample_data encrypt_sample clean