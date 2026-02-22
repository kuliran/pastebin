import pytest
import yaml
import pathlib
import sys
import aiohttp

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql', 
    'pytest_userver.plugins.mongo',
    'shared.fixtures.signup',
    'shared.fixtures.auth_refresh'
]

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))
from shared.fixtures.make_pgsql import make_pgsql

# ================================
# POSTGRESQL
# ================================
pgsql_local = make_pgsql(
    'pg',
    REPO_ROOT / 'db' / 'postgresql' / 'schemas'
)

@pytest.fixture
def pg_cursor(pgsql):
    return pgsql['db_1'].cursor()

# ================================
# GENERAL
# ================================
