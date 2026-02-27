import pytest
from dataclasses import dataclass
import utils.auth as auth

@dataclass
class SignupResult:
    user_id: str
    access_tk: str
    refresh_tk: str

@pytest.fixture
def signup(endpoints, service_client):
    async def _signup(username: str, password: str) -> SignupResult:
        body = {"username": username, "password": password}
        response = await service_client.post(endpoints['auth_signup'], json=body)
        assert response.status == 201

        json = response.json()
        assert type(json['user_id']) is str
        tokens = auth.validate_response_tokens(response)

        return SignupResult(
            user_id=json['user_id'],
            access_tk=tokens.access_tk,
            refresh_tk=tokens.refresh_tk
        )
    return _signup
