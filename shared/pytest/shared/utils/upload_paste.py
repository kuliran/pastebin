from dataclasses import dataclass
from datetime import datetime
from urllib.parse import urlparse

@dataclass
class UploadCreateUrlResult:
    presigned_url: str
    paste_id: str
    created_at_utc: datetime
    expires_at_utc: datetime

@dataclass
class UploadS3Result:
    data: bytes
    version_id: str
    size_bytes: int

@dataclass
class UploadSubmitResult:
    created_at_utc: datetime
    expires_at_utc: datetime
    size_bytes: int

@dataclass
class UploadPasteResult:
    paste_id: str
    presigned_url: str
    version_id: str
    data: str
    created_at_utc: datetime
    expires_at_utc: datetime
    size_bytes: int


def get_paste_id(presigned_url: str) -> str:
    return urlparse(presigned_url).path.lstrip("/").split("/")[-1]