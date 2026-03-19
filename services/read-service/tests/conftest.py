import pytest
import pathlib
import sys
import os

pytest_plugins = [
    'pytest_userver.plugins.core',
    'pytest_userver.plugins.postgresql',
    'pytest_userver.plugins.grpc', 
    'pytest_userver.plugins.s3api',
    'shared.fixtures.endpoints',
    'shared.fixtures.make_minio',
    'shared.fixtures.make_pgsql',
    'shared.fixtures.auth',
    'shared.fixtures.raw_insert_paste',
    'shared.fixtures.raw_get_paste',
    'shared.fixtures.raw_patch_paste',
    'shared.fixtures.raw_delete_paste',
    'shared.fixtures.api_get_paste',
    'shared.fixtures.api_get_paste_details',
    'shared.fixtures.api_get_my_pastes',
]

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))

@pytest.fixture(scope='session')
def make_pgsql():
    return {
        'db_name': 'pg',
        'schemas_path': REPO_ROOT / 'db' / 'postgresql' / 'schemas',
    }

@pytest.fixture(scope="session")
def make_minio():
    return {
        'bucket': 'pastes',
        'access_key': 'minioadmin',
        'secret_key': 'minioadmin',
        'port': 19002,
        'policy_file_path': REPO_ROOT / 'db' / 'minio' / 'pastes_policy.json'
    }

@pytest.fixture
def pg_cursor(pgsql):
    return pgsql['db_1'].cursor()

@pytest.fixture(scope='session')
def service_env(minio_server):
    return {
        'S3_PUBLIC_ENDPOINT': minio_server["endpoint"],
        'S3_ACCESS_KEY': minio_server["access_key"],
        'S3_SECRET_KEY': minio_server["secret_key"],
        'UBSAN_OPTIONS': 'suppressions=' + str(pathlib.Path(__file__).parent / 'ubsan.supp') + ':print_stacktrace=1:print_suppressions=1'
    }

# =============
# grpc
# =============

@pytest.fixture(scope='session')
def grpc_mockserver_endpoint():
    return '[::1]:8091'

sys.path.insert(0, 'build-debug/proto')
import friends_pb2
import friends_pb2_grpc

@pytest.fixture
def mock_is_friend(grpc_mockserver):
    def _setup(*, result: bool = False, handler=None):
        if handler is None:
            async def handler(request, context):
                return friends_pb2.IsFriendResponse(is_friend=result)

        grpc_mockserver.mock_factory(
            friends_pb2_grpc.FriendsServiceServicer
        )('IsFriend')(handler)
    return _setup