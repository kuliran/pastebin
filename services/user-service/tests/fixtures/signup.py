import pytest
from dataclasses import dataclass
import utils.auth as auth

def refresh_path():
    return '/api/v2/auth/refresh'

@dataclass
class SignupResult:
    access_tk: str
    refresh_tk: str

@pytest.fixture
def signup(service_client):
    async def _signup(username: str, password: str) -> SignupResult:
        body = {"username": username, "password": password}
        response = await service_client.post('/api/v2/auth/signup', json=body)
        assert response.status == 201
        tokens = auth.validate_response_tokens(response)

        return SignupResult(
            access_tk=tokens.access_tk,
            refresh_tk=tokens.refresh_tk
        )
    return _signup
