import pytest
import yaml
import pathlib
import sys
import aiohttp

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql',
    'pytest_userver.plugins.grpc',
    'fixtures.signup',
    'fixtures.login',
    'fixtures.auth_refresh',
    'shared.fixtures.auth',
    'shared.fixtures.api_friends',
    'shared.fixtures.endpoints',
    'shared.fixtures.make_pgsql',
]

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))

# ================================
# POSTGRESQL
# ================================
@pytest.fixture(scope='session')
def make_pgsql():
    return {
        'db_name': 'pg',
        'schemas_path': REPO_ROOT / 'db' / 'postgresql' / 'schemas',
    }

@pytest.fixture
def pg_cursor(pgsql):
    return pgsql['db_1'].cursor()
