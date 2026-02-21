import pytest
from dataclasses import dataclass
from datetime import datetime
from dateutil.parser import isoparse
from fixtures.raw_get_paste import GetPasteResult

@pytest.fixture
async def api_get_paste(service_client) -> GetPasteResult:
    async def _get(paste_id: str):
        response = await service_client.get(f'/api/v1/{paste_id}')
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        assert type(json['text']) is str
        assert type(json['created_at']) is str
        assert type(json['expires_at']) is str
        assert type(json['size_bytes']) is int

        return GetPasteResult(
            text=json['text'],
            size_bytes=json['size_bytes'],
            created_at_utc=isoparse(json['created_at']),
            expires_at_utc=isoparse(json['expires_at']),
        )
    return _get

@pytest.fixture
async def api_get_paste_expect_none(service_client):
    async def _get(paste_id: str):
        response = await service_client.get(f'/api/v1/{paste_id}')
        assert response.status == 404
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == 'null'
    return _get
