-- 001_init
DROP SCHEMA IF EXISTS pastes CASCADE;

CREATE SCHEMA IF NOT EXISTS pastes;

CREATE TABLE IF NOT EXISTS pastes.metadata (
    id VARCHAR(64),
    created_at TIMESTAMPTZ NOT NULL,
    expires_at TIMESTAMPTZ NOT NULL,
    delete_key TEXT NOT NULL,
    size_bytes INTEGER NOT NULL,

    PRIMARY KEY (id, expires_at)
) PARTITION BY RANGE (expires_at);

CREATE SCHEMA IF NOT EXISTS partman;
CREATE EXTENSION IF NOT EXISTS pg_partman SCHEMA partman;

SELECT pg_get_function_arguments(oid) 
FROM pg_proc 
WHERE proname = 'create_parent' 
AND pronamespace = (SELECT oid FROM pg_namespace WHERE nspname = 'partman');

SELECT partman.create_parent(
    p_parent_table  => 'pastes.metadata',
    p_control       => 'expires_at',
    p_type         => 'native',
    p_interval      => '1 week',
    p_premake       => 1
);

SELECT partman.run_maintenance('pastes.metadata');

UPDATE partman.part_config
SET    retention = '1 week',
       retention_keep_table = false
WHERE  parent_table = 'pastes.metadata';