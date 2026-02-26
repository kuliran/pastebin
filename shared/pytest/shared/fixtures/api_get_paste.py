import pytest
from dataclasses import dataclass
from dateutil.parser import isoparse
from shared.fixtures.raw_get_paste import GetPasteResult
from datetime import datetime
from shared.utils.client import Client

@dataclass
class GetPasteUrlResult:
    presigned_url: str
    size_bytes: int
    created_at_utc: datetime
    expires_at_utc: datetime

@pytest.fixture
async def api_get_paste_url(auth_client, endpoints) -> GetPasteUrlResult:
    async def _get(paste_id: str, *, client: Client = auth_client):
        response = await client.get(endpoints['get_paste_presigned_url'] + f'/{paste_id}')
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        assert type(json['presigned_url']) is str
        assert type(json['created_at']) is str
        assert type(json['expires_at']) is str
        assert type(json['size_bytes']) is int

        return GetPasteUrlResult(
            presigned_url=json['presigned_url'],
            size_bytes=json['size_bytes'],
            created_at_utc=isoparse(json['created_at']),
            expires_at_utc=isoparse(json['expires_at']),
        )
    return _get

@pytest.fixture(scope='session')
async def s3_get(minio_server) -> bytes:
    async def _get(paste_id: str):
        response = minio_server['client'].get_object(
            Bucket=minio_server['bucket'],
            Key=f"submitted/{paste_id}",
        )
        data = response["Body"].read()
        assert data
        return data
    return _get

@pytest.fixture
async def api_get_paste(api_get_paste_url, s3_get, auth_client):
    async def _get(paste_id: str, *, client: Client = auth_client) -> GetPasteResult:
        response = await api_get_paste_url(paste_id, client=client)
        data = await s3_get(paste_id)
        return GetPasteResult(
            data=data,
            size_bytes=response.size_bytes,
            created_at_utc=response.created_at_utc,
            expires_at_utc=response.expires_at_utc,
        )
    return _get

@pytest.fixture
async def api_get_paste_expect_none(auth_client, endpoints):
    async def _get(paste_id: str, *, client: Client = auth_client):
        response = await client.get(endpoints['get_paste_presigned_url'] + f'/{paste_id}')
        assert response.status == 404
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == 'null'
    return _get
