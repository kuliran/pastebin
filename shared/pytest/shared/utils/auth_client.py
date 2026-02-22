import pytest
from dataclasses import dataclass
import shared.utils.auth as auth

@pytest.fixture
def auth_client(service_client, signup):
    async def _authenticated_client(username, password):
        tokens = await signup(username, password)
        return AuthClient(
            _client=service_client,
            _access_tk=tokens.access_tk,
            _refresh_tk=tokens.refresh_tk,
        )
    return _authenticated_client

@dataclass
class AuthClient:
    _client: object
    _access_tk: str
    _refresh_tk: str
    _host: str = ''

    async def get(self, path, **kwargs):
        return await self._auth_request('GET', path, **kwargs)

    async def post(self, path, **kwargs):
        return await self._auth_request('POST', path, **kwargs)

    async def delete(self, path, **kwargs):
        return await self._auth_request('DELETE', path, **kwargs)

    async def _auth_request(self, method, path, **kwargs):
        headers = kwargs.pop('headers', {})
        headers['Authorization'] = f'Bearer {self._access_tk}'
        headers.setdefault('Host', self._host)
        return await self._client.request(
            method, path, headers=headers, **kwargs
        )
    
    async def _unauth_request(self, method, path, **kwargs):
        headers = {}
        headers.setdefault('Host', self._host)
        return await self._client.request(
            method, path, headers=headers, **kwargs
        )