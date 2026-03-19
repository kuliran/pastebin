ALTER TYPE pastes.visibility ADD VALUE 'friends';

CREATE TABLE IF NOT EXISTS users.friend_relations (
    user_id TEXT,
    friend_id TEXT,
    PRIMARY KEY (user_id, friend_id)
);

ALTER TABLE users.friend_relations
    ADD CONSTRAINT fk_user_id
    FOREIGN KEY (user_id) REFERENCES users.accounts(id) ON DELETE CASCADE;
ALTER TABLE users.friend_relations
    ADD CONSTRAINT fk_friend_id
    FOREIGN KEY (friend_id) REFERENCES users.accounts(id) ON DELETE CASCADE;