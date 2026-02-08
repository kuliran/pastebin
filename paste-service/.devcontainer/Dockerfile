FROM ghcr.io/userver-framework/ubuntu-22.04-userver-pg-dev:v2.15

# Install mongodb
RUN wget -qO- https://pgp.mongodb.com/server-7.0.asc | sudo gpg --dearmor | sudo tee /usr/share/keyrings/mongodb-server-7.0.gpg >/dev/null \
    && echo "deb [ arch=amd64,arm64 signed-by=/usr/share/keyrings/mongodb-server-7.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" \
        | tee -a /etc/apt/sources.list.d/mongodb-org-7.0.list \
    && apt-get update && apt-get install -y \
        mongodb-org \
        mongodb-mongosh