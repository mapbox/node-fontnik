BUILD_TYPE ?= Release

default: release

release:
	$(MAKE) build BUILD_TYPE=Release

debug:
	$(MAKE) build BUILD_TYPE=Debug

build:
	cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	cmake --build build

coverage: clean
	export CC=clang CXX=clang++; \
	export CXXFLAGS="-fprofile-instr-generate -fcoverage-mapping $${CXXFLAGS:-}"; \
	export LDFLAGS="-fprofile-instr-generate $${LDFLAGS:-}"; \
	cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug && \
	cmake --build build && \
	rm -f *profraw *gcov *profdata; \
	LLVM_PROFILE_FILE="code-%p.profraw" npm test; \
	llvm-profdata merge -output=code.profdata code-*.profraw; \
	llvm-cov report ./build/fontnik.node -instr-profile=code.profdata -use-color; \
	llvm-cov show ./build/fontnik.node -instr-profile=code.profdata src/*.cpp -filename-equivalence -use-color; \
	llvm-cov show ./build/fontnik.node -instr-profile=code.profdata src/*.cpp -filename-equivalence -use-color --format html > /tmp/coverage.html; \
	echo "open /tmp/coverage.html for HTML version of this report"

sanitize: clean
	export CC=clang CXX=clang++; \
	export CXXFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all $${CXXFLAGS:-}"; \
	export LDFLAGS="-fsanitize=address,undefined $${LDFLAGS:-}"; \
	cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug && \
	cmake --build build && \
	export ASAN_OPTIONS=detect_leaks=0:fast_unwind_on_malloc=0:$${ASAN_OPTIONS:-}; \
	if [ "$$(uname -s)" = Darwin ]; then \
		DYLD_INSERT_LIBRARIES=$$(clang -fsanitize=address -print-file-name=libclang_rt.asan_osx_dynamic.dylib) \
			node ./node_modules/.bin/tape test/**/*.test.js; \
	else \
		LD_PRELOAD=$$(clang -fsanitize=address -print-file-name=libclang_rt.asan-$$(uname -m).so) \
			node ./node_modules/.bin/tape test/**/*.test.js; \
	fi

tidy:
	@command -v clang-tidy >/dev/null 2>&1 || { echo "clang-tidy not found; install LLVM (e.g. brew install llvm)" >&2; exit 1; }
	rm -rf build
	cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build build
	cd build && (run-clang-tidy -p . -fix "../src/*.cpp" 2>/dev/null || \
		clang-tidy -p . -fix "../src/glyphs.cpp" "../src/node_fontnik.cpp")
	@dirty=$$(git ls-files --modified src/); \
	if [ -n "$$dirty" ]; then echo "$$dirty"; git diff; exit 1; fi

format:
	@command -v clang-format >/dev/null 2>&1 || { echo "clang-format not found; install LLVM (e.g. brew install llvm)" >&2; exit 1; }
	find src -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) | xargs clang-format -i -style=file
	@dirty=$$(git ls-files --modified src/ include/); \
	if [ -n "$$dirty" ]; then echo "$$dirty"; git diff; exit 1; fi

clean:
	rm -rf build
	rm -f *.profraw *.profdata

distclean: clean
	rm -rf node_modules
	rm -rf prebuilds

test:
	npm test

test-linux:
	./scripts/docker-linux-test.sh

test-linux-x64:
	./scripts/docker-linux-test.sh x64

test-linux-arm64:
	./scripts/docker-linux-test.sh arm64

testpack:
	rm -f ./*tgz
	npm pack

testpacked: testpack
	rm -rf /tmp/fontnik-package
	mkdir -p /tmp/fontnik-package
	tar -xf fontnik-*.tgz -C /tmp/fontnik-package --strip-components=1
	cp -r test fonts node_modules /tmp/fontnik-package/
	(cd /tmp/fontnik-package && npm run rebuild && npm test)

.PHONY: release debug build test testpack testpacked coverage tidy format sanitize clean distclean test-linux test-linux-x64 test-linux-arm64
