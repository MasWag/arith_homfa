# Makefile for Blood Glucose Monitoring Example
# Handles key generation, data encryption, and spec file generation

AHOMFA_UTIL := ../../build/src/ahomfa_util
BUILD_DIR := ../../build

all: keys specs

keys: ckks.key tfhe.key ckks.relinkey tfhe.bkey

specs: bg1.spec bg1.reversed.spec

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

# Encrypt blood glucose data (expects .bg.txt input file)
%.bg.ckks: %.bg.txt $(AHOMFA_UTIL) ckks.key config.json
	$(AHOMFA_UTIL) ckks enc -c ./config.json -K ./ckks.key -i $< -o $@

# Generate sample blood glucose data
sample_data:
	@echo "Generating sample blood glucose data..."
	@echo "85" > adult_sample.bg.txt
	@echo "92" >> adult_sample.bg.txt
	@echo "78" >> adult_sample.bg.txt
	@echo "110" >> adult_sample.bg.txt
	@echo "95" >> adult_sample.bg.txt
	@echo "88" >> adult_sample.bg.txt
	@echo "72" >> adult_sample.bg.txt
	@echo "105" >> adult_sample.bg.txt
	@echo "98" >> adult_sample.bg.txt
	@echo "82" >> adult_sample.bg.txt
	@echo "90" >> adult_sample.bg.txt
	@echo "75" >> adult_sample.bg.txt
	@echo "115" >> adult_sample.bg.txt
	@echo "93" >> adult_sample.bg.txt
	@echo "87" >> adult_sample.bg.txt
	@echo "102" >> adult_sample.bg.txt
	@echo "80" >> adult_sample.bg.txt
	@echo "96" >> adult_sample.bg.txt
	@echo "108" >> adult_sample.bg.txt
	@echo "71" >> adult_sample.bg.txt
	@for i in {1..480}; do echo "$$((70 + RANDOM % 50))" >> adult_sample.bg.txt; done
	@echo "Sample data created: adult_sample.bg.txt (500 readings)"

# Encrypt sample data
encrypt_sample: sample_data adult_sample.bg.ckks
	@echo "Sample data encrypted: adult_sample.bg.ckks"

# Extract data from CSV (if CSV files are available)
%.bg.txt: data/%.csv
	cut -d , -f 2 $< | sed '1 d' > $@

clean:
	$(RM) *.key *.bkey *.relinkey *.spec *.ckks *.txt

.PHONY: all keys specs sample_data encrypt_sample clean
