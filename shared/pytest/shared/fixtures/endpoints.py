import pytest

@pytest.fixture(scope='session')
def endpoints():
    return {
        'auth_signup': lambda: '/api/v2/auth/signup',
        'auth_login': lambda: '/api/v2/auth/login',
        'auth_refresh': lambda: '/api/v2/auth/refresh',
        'get_paste_presigned_url': lambda paste_id: f'/api/v2/paste/{paste_id}',
        'get_paste_details': lambda paste_id: f'/api/v2/paste/{paste_id}/details',
        'get_my_pastes': lambda: '/api/v2/my-pastes',
        'upload_paste_create_url': lambda: '/api/v2/paste/create-url',
        'upload_paste_submit': lambda: f'/api/v2/paste/submit/',
        'delete_paste': lambda paste_id: f'/api/v2/paste/delete/{paste_id}',
        'patch_paste': lambda paste_id: f'/api/v2/paste/patch/{paste_id}'
    }