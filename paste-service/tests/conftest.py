import pytest
import yaml

from testsuite.databases.pgsql import discover 

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql', 
    'pytest_userver.plugins.mongo', 
]


# ================================
# MONGODB
# ================================
@pytest.fixture(scope='session')
def mongodb_settings():
    return {
        'hello_users': {
            'settings': {
                'collection': 'hello_users',
                'connection': 'admin',
                'database': 'admin',
            },
            'indexes': [],
        },
    }

@pytest.fixture(scope='session')
def userver_mongo_config(service_static_config):
    components = service_static_config['components_manager']['components']
    component = components.get('mongo-db-1', {})
    dbconnection = component.get('dbconnection', 'mongodb://localhost:27217/')
    
    def _userver_mongo_config(config_vars, config_vars_path):
        return {
            'mongo-db-1': {
                'dbconnection': dbconnection,
            }
        }
    return _userver_mongo_config

# ================================
# POSTGRESQL
# ================================
@pytest.fixture(scope='session')
def initial_data_path(service_source_dir):
    """Path for find files with data"""
    return [
        service_source_dir / 'postgresql/data',
    ]

@pytest.fixture(scope='session')
def pgsql_local(service_source_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        'paste_service',
        [service_source_dir.joinpath('postgresql/schemas')],
    )
    return pgsql_local_create(list(databases.values()))

@pytest.fixture(scope='session')
def userver_pg_config(service_static_config):
    components = service_static_config['components_manager']['components']
    component = components.get('postgres-db-1', {})
    dbconnection = component.get('dbconnection', 'postgresql://testsuite@localhost/postgres')
    
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
