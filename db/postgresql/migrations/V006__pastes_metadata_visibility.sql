CREATE TYPE pastes.visibility AS ENUM ('public', 'private');

ALTER TABLE pastes.metadata
    ADD COLUMN visibility pastes.visibility NOT NULL DEFAULT 'public';

CREATE TABLE IF NOT EXISTS pastes.private_permissions (
    paste_id TEXT,
    user_id TEXT,
    PRIMARY KEY (paste_id, user_id)
);