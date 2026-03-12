from dataclasses import dataclass

@dataclass
class Client:
    _client: object
    _access_tk: str
    _user_id: str
    _refresh_tk: str = None
    _password: str = None
    _username: str = None

    async def get(self, path, **kwargs):
        return await self._auth_request('GET', path, **kwargs)

    async def post(self, path, **kwargs):
        return await self._auth_request('POST', path, **kwargs)

    async def delete(self, path, **kwargs):
        return await self._auth_request('DELETE', path, **kwargs)

    async def patch(self, path, **kwargs):
        return await self._auth_request('PATCH', path, **kwargs)

    async def _auth_request(self, method, path, **kwargs):
        headers = kwargs.pop('headers', {})
        if self._access_tk is not None:
            headers['Authorization'] = f'Bearer {self._access_tk}'
        return await self._client.request(
            method, path, headers=headers, **kwargs
        )
    
    async def _unauth_request(self, method, path, **kwargs):
        return await self._client.request(
            method, path, **kwargs
        )
