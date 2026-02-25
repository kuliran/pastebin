from dataclasses import dataclass
import pathlib
from datetime import datetime, timezone, timedelta
import jwt

SHARED_DIR = pathlib.Path(__file__).parent.parent.parent.parent
PRIVATE_KEY = pathlib.Path(str(SHARED_DIR / 'keys' / 'dev' / 'private.pem')).read_text()

def make_test_token(user_id: str) -> str:
    return jwt.encode(
        {
            'sub': user_id,
            'iss': 'user-service',
            'iat': datetime.now(timezone.utc),
            'exp': datetime.now(timezone.utc) + timedelta(hours=24),
        },
        PRIVATE_KEY,
        algorithm='RS256'
    )

@dataclass
class Client:
    _client: object
    _access_tk: str
    _user_id: str

    async def get(self, path, **kwargs):
        return await self._auth_request('GET', path, **kwargs)

    async def post(self, path, **kwargs):
        return await self._auth_request('POST', path, **kwargs)

    async def delete(self, path, **kwargs):
        return await self._auth_request('DELETE', path, **kwargs)

    async def _auth_request(self, method, path, **kwargs):
        headers = kwargs.pop('headers', {})
        headers['Authorization'] = f'Bearer {self._access_tk}'
        return await self._client.request(
            method, path, headers=headers, **kwargs
        )
    
    async def _unauth_request(self, method, path, **kwargs):
        return await self._client.request(
            method, path, **kwargs
        )
