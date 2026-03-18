import pytest
from shared.utils.client import Client

@pytest.fixture
async def api_get_friends(api_get_friends_raw, auth_client):
    async def _get(*, client: Client = auth_client) -> list[str]:
        response = await api_get_friends_raw(client=client)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']
        
        json = response.json()
        assert type(json) is list
        return json
    return _get

@pytest.fixture
async def api_get_friends_raw(auth_client, endpoints):
    async def _get(*, client: Client = auth_client):
        return await client.get(endpoints['get_friends']())
    return _get



@pytest.fixture
async def api_add_friend(api_add_friend_raw, auth_client):
    async def _add(friend_id: str, *, client: Client = auth_client):
        response = await api_add_friend_raw(friend_id, client=client)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']
    return _add

@pytest.fixture
async def api_add_friend_raw(auth_client, endpoints):
    async def _add(friend_id: str, *, client: Client = auth_client):
        return await client.post(endpoints['add_friend'](friend_id))
    return _add



@pytest.fixture
async def api_rm_friend(api_rm_friend_raw, auth_client):
    async def _rm(friend_id: str, *, client: Client = auth_client):
        response = await api_rm_friend_raw(friend_id, client=client)
        assert response.status == 204
        assert 'application/json' in response.headers['Content-Type']
    return _rm

@pytest.fixture
async def api_rm_friend_raw(auth_client, endpoints):
    async def _rm(friend_id: str, *, client: Client = auth_client):
        return await client.delete(endpoints['rm_friend'](friend_id))
    return _rm