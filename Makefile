.PHONY: format format-check build flash monitor clean check

ENV ?= esp32dev
PORT ?=

PIO = pio
PIO_ENV = -e $(ENV)

format:
	find src include -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 clang-format -i

format-check:
	@find src include -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | \
	xargs -0 -I{} sh -c 'clang-format "$$1" | diff -u "$$1" -' sh {}

build:
	$(PIO) run $(PIO_ENV)

flash:
ifeq ($(PORT),)
	$(PIO) run $(PIO_ENV) -t upload
else
	$(PIO) run $(PIO_ENV) -t upload --upload-port $(PORT)
endif

monitor:
ifeq ($(PORT),)
	$(PIO) device monitor
else
	$(PIO) device monitor --port $(PORT)
endif

flash-monitor: flash monitor

check:
	$(PIO) check $(PIO_ENV)

clean:
	$(PIO) run $(PIO_ENV) -t clean