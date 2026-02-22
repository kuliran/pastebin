DROP SCHEMA IF EXISTS pastes CASCADE;

-- V001__init
CREATE SCHEMA IF NOT EXISTS pastes;

CREATE TABLE IF NOT EXISTS pastes.metadata (
    id VARCHAR(64) PRIMARY KEY,
    created_at TIMESTAMPTZ NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    delete_key TEXT NOT NULL,
    size_bytes INTEGER NOT NULL
);

-- V002__users
CREATE SCHEMA IF NOT EXISTS users;

CREATE TABLE IF NOT EXISTS users.accounts (
    id TEXT PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    pwd_hash TEXT NOT NULL
);

-- V003__jwt
CREATE TABLE IF NOT EXISTS users.jwt_sessions (
    user_id TEXT REFERENCES users.accounts(id) ON DELETE CASCADE,
    refresh_tk UUID DEFAULT gen_random_uuid(),
    created_at TIMESTAMPTZ NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    PRIMARY KEY (user_id, refresh_tk)
);
CREATE UNIQUE INDEX uniq_jwt_sessions_refresh_tk ON users.jwt_sessions(refresh_tk);