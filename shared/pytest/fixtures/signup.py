import pytest
from dataclasses import dataclass

@dataclass
class SignupResult:
    access_tk: str
    refresh_tk: str

@pytest.fixture
def signup(service_client):
    async def _signup(username: str, password: str) -> SignupResult:
        body = {"username": username, "password": password}
        response = await service_client.post('/api/v2/signup', json=body)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        assert type(json['access_tk']) is str

        return SignupResult(
            access_tk=json['access_tk'],
            refresh_tk=""
        )
    return _signup