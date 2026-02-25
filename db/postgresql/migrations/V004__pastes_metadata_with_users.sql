CREATE TYPE pastes.status AS ENUM ('pending', 'submitted', 'deleted');

ALTER TABLE pastes.metadata
    ADD COLUMN owner_user_id TEXT,
    ADD COLUMN status pastes.status NOT NULL DEFAULT 'pending',
    ADD COLUMN s3_version_id TEXT,
    ALTER COLUMN delete_key DROP NOT NULL;

-- adding a constraint without validation (non-blocking)
ALTER TABLE pastes.metadata
    ADD CONSTRAINT fk_owner_user_id
    FOREIGN KEY (owner_user_id) REFERENCES users.accounts(id) ON DELETE CASCADE
    NOT VALID;

-- validating separately - doesn't block writes
ALTER TABLE pastes.metadata
    VALIDATE CONSTRAINT fk_owner_user_id;