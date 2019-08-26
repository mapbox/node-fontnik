MODULE_NAME := $(shell node -e "console.log(require('./package.json').binary.module_name)")

# Whether to turn compiler warnings into errors
export WERROR ?= false

default: release

./node_modules/.bin/node-pre-gyp:
	# install deps but for now ignore our own install script
	# so that we can run it directly in either debug or release
	npm install --ignore-scripts

release: ./node_modules/.bin/node-pre-gyp
	V=1 ./node_modules/.bin/node-pre-gyp configure build --error_on_warnings=$(WERROR) --loglevel=error
	@echo "run 'make clean' for full rebuild"

debug: ./node_modules/.bin/node-pre-gyp
	V=1 ./node_modules/.bin/node-pre-gyp configure build --error_on_warnings=$(WERROR) --loglevel=error --debug
	@echo "run 'make clean' for full rebuild"

coverage:
	./scripts/coverage.sh

tidy:
	./scripts/clang-tidy.sh

format:
	./scripts/clang-format.sh

sanitize:
	./scripts/sanitize.sh

clean:
	rm -rf lib/binding
	rm -rf build
	# remove remains from running 'make coverage'
	rm -f *.profraw
	rm -f *.profdata
	@echo "run 'make distclean' to also clear node_modules, mason_packages, and .mason directories"

distclean: clean
	rm -rf node_modules
	rm -rf mason_packages
	# remove remains from running './scripts/setup.sh'
	rm -rf .mason
	rm -rf .toolchain
	rm -f local.env

xcode: ./node_modules/.bin/node-pre-gyp
	./node_modules/.bin/node-pre-gyp configure -- -f xcode

	@# If you need more targets, e.g. to run other npm scripts, duplicate the last line and change NPM_ARGUMENT
	SCHEME_NAME="$(MODULE_NAME)" SCHEME_TYPE=library BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node scripts/create_scheme.sh
	SCHEME_NAME="npm test" SCHEME_TYPE=node BLUEPRINT_NAME=$(MODULE_NAME) BUILDABLE_NAME=$(MODULE_NAME).node NODE_ARGUMENT="`npm bin tape`/tape test/*.test.js" scripts/create_scheme.sh

	open build/binding.xcodeproj

docs:
	npm run docs

test:
	npm test

.PHONY: test docs

testpack:
	rm -f ./*tgz
	npm pack

testpacked: testpack
	rm -rf /tmp/package
	tar -xf *tgz --directory=/tmp/
	du -h -d 0 /tmp/package
	cp -r test /tmp/package/
	cp -r fonts /tmp/package/
	ln -s `pwd`/mason_packages /tmp/package/mason_packages
	(cd /tmp/package && make && make test)
