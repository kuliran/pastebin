CREATE TABLE IF NOT EXISTS pastes.rate_limit (
    user_id TEXT,
    upload_create_url_cnt INT NOT NULL DEFAULT 0,
    upload_submit_cnt INT NOT NULL DEFAULT 0,
    window_started_at TIMESTAMPTZ NOT NULL,
    PRIMARY KEY (user_id)
);

ALTER TABLE pastes.rate_limit
    ADD CONSTRAINT fk_user_id
    FOREIGN KEY (user_id) REFERENCES users.accounts(id) ON DELETE CASCADE;
