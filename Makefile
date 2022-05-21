BUILD_DIR=build-Dedit-Desktop-Debug/

all:
	echo "Building project"
	cmake --build $(BUILD_DIR) --target all

clean:
	echo "Cleaning project"
	cmake --build $(BUILD_DIR) --target clean

reset:
	make clean
	make all

run:
	make reset
	$(BUILD_DIR)ded longfile.txt

