TARGET ?= COCO
MISTER_HOST ?= mister.local
MISTER_DEST ?= /media/fat/fujinet

.PHONY: all build clean install

all: build

build:
	./build.sh $(TARGET)

clean:
	rm -rf build_arm dist

install: build
	@echo "Installing to $(MISTER_HOST):$(MISTER_DEST)..."
	ssh root@$(MISTER_HOST) "mkdir -p $(MISTER_DEST)"
	scp -r dist/fujinet/* root@$(MISTER_HOST):$(MISTER_DEST)/
	@echo "Installation complete."
