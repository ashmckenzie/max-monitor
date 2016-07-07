SOURCEDIR="."
SOURCES := $(shell find $(SOURCEDIR) -name '*.go')

BINARY=max_monitor
BINARY_RELEASE=bin/${BINARY}_${VERSION}

VERSION=$(shell cat VERSION)

HTTP_LISTEN ?= 0.0.0.0:3000
REDIS_HOST ?= 127.0.0.1:6379
DEBUG ?= false

.DEFAULT_GOAL: $(BINARY)

$(BINARY): $(SOURCES) deps bin_dir
	CGO_ENABLED=0 GOOS=linux GOARCH=arm GOARM=6 go build -a -installsuffix cgo -o ${BINARY_RELEASE}_linux_arm

.PHONY: deps
deps:
	go get ./...

.PHONY: update_deps
update_deps:
	go get -u ./...

.PHONY: bin_dir
bin_dir:
	mkdir -p bin

.PHONY: run
run:
	DEBUG="$(DEBUG)" HTTP_LISTEN="$(HTTP_LISTEN)" REDIS_HOST="$(REDIS_HOST)" go run cmd/max_monitor/main.go $(filter-out $@, $(MAKECMDGOALS))	go run main.go

.PHONY: clean
clean:
	rm -f ${BINARY} ${BINARY}_*
