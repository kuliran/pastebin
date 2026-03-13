import pytest
from dataclasses import dataclass
import utils.auth as auth

@dataclass
class LoginResult:
    user_id: str
    access_tk: str
    refresh_tk: str

@pytest.fixture
def login(endpoints, service_client):
    async def _login(username: str, password: str) -> LoginResult:
        body = {"username": username, "password": password}
        response = await service_client.post(endpoints['auth_login'](), json=body)
        assert response.status == 200
        tokens = auth.validate_response_tokens(response)

        json = response.json()
        return LoginResult(
            user_id=json['user_id'],
            access_tk=tokens.access_tk,
            refresh_tk=tokens.refresh_tk
        )
    return _login