# Install python testing dependencies
.PHONE: e2e-install
e2e-install:
	python3 -m venv tests/venv
	tests/venv/bin/pip install -r tests/requirements.txt

.PHONY: build
build:
	cd services/paste-service && make docker-build-release

# Run E2E tests
.PHONY: e2e
e2e: down
	HOST_UID=$(shell id -u) HOST_GID=$(shell id -g) docker compose up --build -d --wait
	cd tests && venv/bin/pytest e2e/ -v
	docker compose down

# Stop and remove all containers
.PHONY: down
down:
	docker compose down