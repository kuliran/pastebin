CREATE SCHEMA IF NOT EXISTS pastes;

CREATE TABLE IF NOT EXISTS pastes.metadata (
    id VARCHAR(64) PRIMARY KEY,
    created_at TIMESTAMPTZ NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    delete_key TEXT NOT NULL,
    size_bytes INTEGER NOT NULL
) PARTITION BY RANGE (expires_at);

CREATE EXTENSION pg_partman;

SELECT partman.create_parent(
    p_parent_table  => 'pastes.metadata',
    p_control       => 'expires_at',
    p_type          => 'range',
    p_interval      => '1 week',
    p_premake       => 4
);

SELECT partman.run_maintenance('pastes.metadata');

UPDATE partman.part_config
SET    retention = '1 week',
       retention_keep_table = false
WHERE  parent_table = 'pastes.metadata';