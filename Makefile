.PHONY: all clean
.DEFAULT_GOAL := all

HTTP_LISTEN ?= 0.0.0.0:3000
REDIS_HOST ?= 127.0.0.1:6379
DEBUG ?= false

deps:
	go get ./...

test:
	go test

all: deps test build

install: all _install

_install:
	go install

build:
	go build -o max-monitor

clean:
	rm -f max-monitor

run:
	DEBUG="$(DEBUG)" HTTP_LISTEN="$(HTTP_LISTEN)" REDIS_HOST="$(REDIS_HOST)" go run cmd/max_monitor/main.go $(filter-out $@, $(MAKECMDGOALS))
