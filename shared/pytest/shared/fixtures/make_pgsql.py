import pytest
from testsuite.databases.pgsql import discover

@pytest.fixture(scope='session')
def pgsql_local(pgsql_local_create, make_pgsql):
    databases = discover.find_schemas(
        make_pgsql['db_name'],
        [make_pgsql['schemas_path']],
    )
    return pgsql_local_create(list(databases.values()))