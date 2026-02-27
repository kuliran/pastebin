import requests

class ApiResponse:
    def __init__(self, response: requests.Response):
        self._response = response

    @property
    def status(self) -> int:
        return self._response.status_code

    @property
    def headers(self):
        return self._response.headers

    @property
    def text(self) -> str:
        return self._response.text

    def json(self):
        return self._response.json()

    @property
    def content(self) -> bytes:
        return self._response.content
    
    @property
    def cookies(self) -> object:
        return self._response.cookies


class ApiSession:
    def __init__(self, base_url: str, host: str, timeout):
        self._base_url = base_url
        self._client = requests.Session()
        self._client.headers.update({'Host': host})
        self._timeout = timeout

    async def get(self, path, **kwargs):
        return await self.request("GET", path, **kwargs)

    async def post(self, path, **kwargs):
        return await self.request("POST", path, **kwargs)

    async def delete(self, path, **kwargs):
        return await self.request("DELETE", path, **kwargs)

    async def request(self, method, path, **kwargs):
        r = self._client.request(method, f"{self._base_url}/{path}", timeout=self._timeout, **kwargs)
        return ApiResponse(r)

    def set_cookie(self, key, value):
        self._client.cookies.set(key, value)

    def clear_cookies(self):
        self._client.cookies.clear()

    async def __aenter__(self):
        return self

    async def __aexit__(self, exc_type, exc, tb):
        await self._client.aclose()