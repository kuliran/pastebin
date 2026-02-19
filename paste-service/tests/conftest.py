import pytest
import yaml

from testsuite.databases.pgsql import discover

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql', 
    'pytest_userver.plugins.mongo',
    'fixtures.api_get_paste',
    'fixtures.raw_insert_paste',
    'fixtures.api_upload_paste',
    'fixtures.api_delete_paste',
]


# ================================
# MONGODB
# ================================
@pytest.fixture(scope='session')
def mongodb_settings():
    return {
        'pastes_db_1': {
            'settings': {
                'collection': 'pastes',
                'connection': 'pastes_db_1',
                'database': 'pastes_db_1',
            },
            'indexes': [],
        },
    }

# ================================
# POSTGRESQL
# ================================

@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        'pg_pastes',
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))

@pytest.fixture(scope='session')
def userver_pg_config(service_static_config):
    component = service_static_config['components_manager']['components'].get('postgres-db-1')
    dbconnection = component.get('dbconnection')
    
    def _userver_pg_config(config_vars, config_vars_path):
        return {
            'postgres-db-1': {
                'dbconnection': dbconnection,
            }
        }
    return _userver_pg_config

# ================================
# GENERAL
# ================================
@pytest.fixture(scope='session')
def service_static_config(service_source_dir):
    config_path = service_source_dir / 'configs' / 'static_config.yaml'
    with open(config_path) as f:
        return yaml.safe_load(f)

@pytest.fixture
def mongo_collection(mongodb):
    return mongodb['pastes_db_1']

@pytest.fixture
def pg_cursor(pgsql):
    return pgsql['db_1'].cursor()