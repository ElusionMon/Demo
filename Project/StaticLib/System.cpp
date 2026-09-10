#include "MyEngine.h"

namespace MyEngine {
    
    void Camera::Rotate(float deltaYaw, float deltaPitch) {
        yaw += deltaYaw;
        pitch += deltaPitch;
        
        if (pitch > MAX_PITCH) pitch = MAX_PITCH;
        if (pitch < -MAX_PITCH) pitch = -MAX_PITCH;
    }
    
    void Camera::Update(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 fp, float angle, float dist, float height) {
        if (angle != 0.0f) {
            yaw = angle;
        }
        
        pos.x = fp.x - sinf(yaw) * dist; 
        pos.y = fp.y + height + sinf(pitch) * dist * 0.3f;
        pos.z = fp.z - cosf(yaw) * dist; 
        target = fp;
        
        D3DXVECTOR3 upVec(0.0f, 1.0f, 0.0f); 
        D3DXMatrixLookAtLH(&matView, &pos, &target, &upVec);
        D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4.0f, aspect, 1.0f, 500.0f);
        
        dev->SetTransform(D3DTS_VIEW, &matView); 
        dev->SetTransform(D3DTS_PROJECTION, &matProj);
    }

    void Input::Init(HWND h) { 
        attachedHWnd = h; 
        memset(keys, 0, 256); 
        memset(prevKeys, 0, 256); 
    }
    
    void Input::Update() {
        memcpy(prevKeys, keys, 256);
        for (int i = 0; i < 256; i++) 
            keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        GetCursorPos(&mousePos); 
        ScreenToClient(attachedHWnd, &mousePos);
    }

    bool Text::Init(LPDIRECT3DDEVICE9 dev, int s, std::string f) {
        if (font) { 
            font->Release(); 
            font = nullptr; 
        }
        return SUCCEEDED(D3DXCreateFontA(dev, s, 0, FW_BOLD, 1, FALSE, 
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, 
            DEFAULT_PITCH, f.c_str(), &font));
    }
    
    void Text::Draw(std::string s, int x, int y, DWORD c) {
        if (!font) return;
        RECT r = { x, y, 32000, 32000 };
        font->DrawTextA(NULL, s.c_str(), -1, &r, DT_LEFT | DT_NOCLIP, c);
    }
}
