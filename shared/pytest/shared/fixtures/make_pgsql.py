import pytest
import pathlib
from testsuite.databases.pgsql import discover

def make_pgsql(db_name: str, schemas_path: pathlib.Path):
    @pytest.fixture(scope='session')
    def pgsql_local(pgsql_local_create):
        databases = discover.find_schemas(
            db_name,
            [schemas_path],
        )
        return pgsql_local_create(list(databases.values()))
    return pgsql_local