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

-- V004__paste_metadata_with_users
CREATE TYPE pastes.status AS ENUM ('pending', 'submitted', 'deleted');

ALTER TABLE pastes.metadata
    ADD COLUMN owner_user_id TEXT,
    ADD COLUMN status pastes.status NOT NULL DEFAULT 'pending',
    ADD COLUMN s3_version_id TEXT,
    ALTER COLUMN delete_key DROP NOT NULL;
-- no FK constraint

-- V005__pastes_rate_limit
CREATE TABLE IF NOT EXISTS pastes.rate_limit (
    user_id TEXT,
    upload_create_url_cnt INT NOT NULL DEFAULT 0,
    upload_submit_cnt INT NOT NULL DEFAULT 0,
    window_started_at TIMESTAMPTZ NOT NULL,
    PRIMARY KEY (user_id)
);
-- no FK constraint

-- V006__pastes_metadata_visibility
CREATE TYPE pastes.visibility AS ENUM ('public', 'private');

ALTER TABLE pastes.metadata
    ADD COLUMN visibility pastes.visibility NOT NULL DEFAULT 'public';

CREATE TABLE IF NOT EXISTS pastes.private_permissions (
    paste_id TEXT,
    user_id TEXT,
    PRIMARY KEY (paste_id, user_id)
);

-- V007__pastes_metadata_user_id_idx
CREATE INDEX idx_pastes_metadata_owner_user_id ON pastes.metadata (owner_user_id);