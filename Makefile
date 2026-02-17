# Install python testing dependencies
.PHONE: e2e-install
e2e-install:
	python3 -m venv tests/venv
	tests/venv/bin/pip install -r tests/requirements.txt

# Run E2E tests
.PHONY: e2e
e2e:
	HOST_UID=$(id -u) HOST_GID=$(id -g) docker compose up -d --wait
	cd tests && venv/bin/pytest e2e/ -v

# Stop and remove all containers
.PHONY: down
down:
	docker compose down