# Install python testing dependencies
.PHONE: e2e-install
e2e-install:
	python3 -m venv tests/venv
	tests/venv/bin/pip install -r tests/requirements.txt

.PHONY: build
build:
	cd services/read-service && make docker-build-release
	cd services/write-service && make docker-build-release
	cd services/user-service && make docker-build-release

# Run all containers for e2e tests
.PHONY: e2e-up
e2e-up:
	HOST_UID=$(shell id -u) HOST_GID=$(shell id -g) docker compose up --build -d --wait

# Run e2e tests
.PHONY: e2e-reuse
e2e-reuse:
	cd tests && venv/bin/pytest e2e/ -v

# Reboot containers and run e2e
.PHONY: e2e
e2e: e2e-down e2e-up e2e-reuse

# Stop and remove all containers
.PHONY: e2e-down
e2e-down:
	docker compose down