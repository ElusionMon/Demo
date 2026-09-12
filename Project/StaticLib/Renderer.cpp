#include "MyEngine.h"

namespace MyEngine {
    Renderer::~Renderer() { 
        if (device) device->Release(); 
        if (d3d) d3d->Release(); 
    }

    bool Renderer::Init(HWND hWnd) {
        d3d = Direct3DCreate9(D3D_SDK_VERSION);
        if (!d3d) return false;

        ZeroMemory(&d3dpp, sizeof(d3dpp));
        d3dpp.Windowed = TRUE;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        d3dpp.EnableAutoDepthStencil = TRUE;
        d3dpp.AutoDepthStencilFormat = D3DFMT_D16; //D24S8

        if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, 
           D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device))) {
            return false;
        } 
        device->SetRenderState(D3DRS_LIGHTING, TRUE);
        device->SetRenderState(D3DRS_ZENABLE, TRUE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device->SetRenderState(D3DRS_WRAP0, D3DWRAP_U | D3DWRAP_V);
        
        device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
    	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        return true;
    }

    void Renderer::Begin() {
        device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFF333333, 1.0f, 0);
        device->BeginScene();
    }

    void Renderer::End() { 
        device->EndScene(); 
        device->Present(NULL, NULL, NULL, NULL); 
    }

    void Renderer::HandleDeviceLost(HWND hWnd) {
        if (!device) return;
        HRESULT hr = device->TestCooperativeLevel();
        if (hr == D3DERR_DEVICENOTRESET) {
            device->Reset(&d3dpp);
            device->SetRenderState(D3DRS_LIGHTING, TRUE);
            device->SetRenderState(D3DRS_ZENABLE, TRUE);
            device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }
    }

    void Renderer::SetupFog(bool enable, DWORD color, float start, float end) {
        if (!device) return;
        if (enable) {
            device->SetRenderState(D3DRS_FOGENABLE, TRUE);
            device->SetRenderState(D3DRS_FOGCOLOR, color);
            device->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
            device->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            device->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&start);
            device->SetRenderState(D3DRS_FOGEND, *(DWORD*)&end);
        } else {
            device->SetRenderState(D3DRS_FOGENABLE, FALSE);
        }
    }
}
