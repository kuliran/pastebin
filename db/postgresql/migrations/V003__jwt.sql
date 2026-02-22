CREATE TABLE IF NOT EXISTS users.jwt_sessions (
    user_id TEXT REFERENCES users.accounts(id) ON DELETE CASCADE,
    refresh_tk UUID DEFAULT gen_random_uuid(),
    created_at TIMESTAMPTZ NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    PRIMARY KEY (user_id, refresh_tk)
);
CREATE UNIQUE INDEX uniq_jwt_sessions_refresh_tk ON users.jwt_sessions(refresh_tk);