import pytest
import pathlib

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


# ================================
# GENERAL
# ================================

@pytest.fixture(scope='session')
def userver_config_secdist():
    def _userver_config_secdist(config_yaml, config_vars):
        project_dir = pathlib.Path(__file__).parent.parent
        config_vars['secdist-path'] = str(project_dir / 'configs' / 'secdist.testing.json')
    return _userver_config_secdist

@pytest.fixture
def mongo_collection(mongodb):
    return mongodb['pastes_db_1']

@pytest.fixture
def pg_cursor(pgsql):
    return pgsql['db_1'].cursor()