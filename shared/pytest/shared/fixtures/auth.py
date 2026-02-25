import pytest
import shared.utils.auth as auth

@pytest.fixture
def auth_client(new_auth_client):
    return new_auth_client('test-user-id')

@pytest.fixture
def new_auth_client(service_client):
    def _new_auth_client(user_id):
        return auth.Client(
            _client=service_client,
            _access_tk=auth.make_test_token(user_id),
            _user_id=user_id,
        )
    return _new_auth_client
