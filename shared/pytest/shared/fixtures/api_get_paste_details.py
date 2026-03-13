import pytest
from dataclasses import dataclass
from dateutil.parser import isoparse
from datetime import datetime
from shared.utils.client import Client

@dataclass
class GetPasteDetailsResult:
    size_bytes: int
    visibility: str
    created_at_utc: datetime
    expires_at_utc: datetime
    private_perms_user_ids: list[str]

@pytest.fixture
async def api_get_paste_details(api_get_paste_details_raw, auth_client):
    async def _get(paste_id: str, *, client: Client = auth_client) -> GetPasteDetailsResult:
        response = await api_get_paste_details_raw(paste_id, client=client)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        assert type(json['size_bytes']) is int
        assert type(json['visibility']) is str
        assert type(json['created_at']) is str
        assert type(json['expires_at']) is str
        assert type(json['private_perms']) is list

        return GetPasteDetailsResult(
            size_bytes=json['size_bytes'],
            visibility=json['visibility'],
            created_at_utc=isoparse(json['created_at']),
            expires_at_utc=isoparse(json['expires_at']),
            private_perms_user_ids=json['private_perms'],
        )
    return _get

@pytest.fixture
async def api_get_paste_details_raw(auth_client, endpoints):
    async def _get(paste_id: str, *, client: Client = auth_client):
        r = await client.get(endpoints['get_paste_details'] + f'/{paste_id}')
        return r
    return _get