import pytest
import pathlib
import boto3
import shutil
import subprocess
import os
import time

@pytest.fixture(scope="session")
def minio_server(tmp_path_factory, make_minio):
    minio_bin = shutil.which("minio")
    if not minio_bin:
        raise RuntimeError("minio binary not found in PATH")

    data_dir = tmp_path_factory.mktemp("minio-data")

    proc = subprocess.Popen(
        [minio_bin, "server", str(data_dir), "--address", f":{make_minio['port']}"],
        env={
            **os.environ,
            "MINIO_ROOT_USER": make_minio['access_key'],
            "MINIO_ROOT_PASSWORD": make_minio['secret_key'],
        },
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    endpoint = f"http://127.0.0.1:{make_minio['port']}"

    import urllib.request
    import urllib.error
    for _ in range(40):
        try:
            urllib.request.urlopen(f"{endpoint}/minio/health/live")
            break
        except urllib.error.URLError:
            time.sleep(0.3)
    else:
        proc.terminate()
        raise RuntimeError("MinIO could not be launched")

    s3 = boto3.client(
        "s3",
        endpoint_url=endpoint,
        aws_access_key_id=make_minio['access_key'],
        aws_secret_access_key=make_minio['secret_key'],
        region_name="us-east-1",
    )
    s3.create_bucket(Bucket=make_minio['bucket'])
    s3.put_bucket_versioning(
        Bucket=make_minio['bucket'],
        VersioningConfiguration={'Status': 'Enabled'}
    )
    with open(make_minio['policy_file_path']) as f:
        s3.put_bucket_policy(Bucket=make_minio['bucket'], Policy=f.read()) 

    yield {
        "endpoint": endpoint,
        "access_key": make_minio['access_key'],
        "secret_key": make_minio['secret_key'],
        "bucket": make_minio['bucket'],
        "client": s3,
    }

    proc.terminate()
    proc.wait()

@pytest.fixture(autouse=True)
def cleanup_bucket(minio_server, make_minio):
    yield
    s3 = minio_server["client"]
    response = s3.list_objects_v2(Bucket=make_minio['bucket'])
    objects = response.get("Contents", [])
    if objects:
        s3.delete_objects(
            Bucket=make_minio['bucket'],
            Delete={"Objects": [{"Key": o["Key"]} for o in objects]},
        )
