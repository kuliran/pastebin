include .env

# Install python testing dependencies
.PHONE: e2e-install
e2e-install:
	python3 -m venv tests/venv
	tests/venv/bin/pip install -r tests/requirements.txt

SERVICES := read write user

.PHONY: build $(addprefix build-, $(SERVICES))
build: $(addprefix build-, $(SERVICES))

$(addprefix build-, $(SERVICES)): build-%:
	cd services/$*-service && make docker-cmake-release DOCKER_ENV="ENABLE_TESTSUITE=OFF" && make docker-build-release

.PHONY: build-all
build-all: build-read build-write build-user

# Run all containers for e2e tests
.PHONY: e2e-up
e2e-up:
	HOST_UID=$(shell id -u) HOST_GID=$(shell id -g) docker compose up --build -d --wait

# Run e2e tests
.PHONY: e2e
e2e:
	cd tests && POSTGRES_CONNECTION=postgresql://${POSTGRES_USER}:${POSTGRES_PASSWORD}@localhost:${POSTGRES_PORT}/${POSTGRES_DB} venv/bin/pytest e2e/ -v

# Reboot containers and run e2e
.PHONY: e2e-fresh
e2e-fresh: e2e-down e2e-up e2e

# Stop and remove all containers
.PHONY: e2e-down
e2e-down:
	docker compose down