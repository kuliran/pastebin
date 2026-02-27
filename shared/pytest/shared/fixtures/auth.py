import pytest
from shared.utils.client import Client
from shared.utils.make_jwt_tk import make_jwt_tk 

@pytest.fixture
def auth_client(new_auth_client):
    return new_auth_client('test-user-id')

@pytest.fixture
def new_auth_client(service_client):
    def _new_auth_client(user_id):
        return Client(
            _client=service_client,
            _access_tk=make_jwt_tk(user_id),
            _user_id=user_id,
        )
    return _new_auth_client
