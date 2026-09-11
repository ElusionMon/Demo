#include "MyEngine.h"
#include <cstdio>

using namespace MyEngine;

Renderer        g_Renderer;
Input           g_Input;
Physics         g_Physics;
ResourceManager* g_Res = nullptr;
Camera*         g_Cam = nullptr;
Text            g_UI;
Skybox          g_Sky;
Scene*          g_Scene = nullptr;

Object*         g_PlayerBody = nullptr;
Object*         g_PlayerHead = nullptr;

ParticleSystem* g_Bullets = nullptr;

AudioSystem     g_Audio;

float           g_PlayerAngle = 0.0f;
float           g_PlayerPitch = 0.0f;
float           g_MoveSpeed = 0.08f;
bool            g_Wireframe = false;
bool            g_JumpRequested = false;
float           g_ShootCooldown = 0.0f;
const float     g_ShootDelay = 0.15f;

Object* CreateSimpleCube(LPDIRECT3DDEVICE9 dev, int id, D3DXVECTOR3 pos, D3DXVECTOR3 size, D3DCOLORVALUE color) {
    std::vector<Vertex> verts;
    
    float hx = size.x / 2;
    float hy = size.y / 2;
    float hz = size.z / 2;
    
    D3DXVECTOR3 corners[8] = {
        D3DXVECTOR3(-hx, -hy, -hz), D3DXVECTOR3( hx, -hy, -hz),
        D3DXVECTOR3( hx, -hy,  hz), D3DXVECTOR3(-hx, -hy,  hz),
        D3DXVECTOR3(-hx,  hy, -hz), D3DXVECTOR3( hx,  hy, -hz),
        D3DXVECTOR3( hx,  hy,  hz), D3DXVECTOR3(-hx,  hy,  hz)
    };
    
    D3DXVECTOR3 normals[6] = {
        D3DXVECTOR3(0, -1, 0), D3DXVECTOR3(0, 1, 0),
        D3DXVECTOR3(-1, 0, 0), D3DXVECTOR3(1, 0, 0),
        D3DXVECTOR3(0, 0, -1), D3DXVECTOR3(0, 0, 1)
    };
    
    int indices[36] = {
        0,1,2, 0,2,3,
        4,6,5, 4,7,6,
        0,4,1, 1,4,5,
        2,6,3, 3,6,7,
        0,3,7, 0,7,4,
        1,5,2, 2,5,6
    };
    
    for (int i = 0; i < 36; i++) {
        Vertex v;
        v.x = corners[indices[i]].x;
        v.y = corners[indices[i]].y;
        v.z = corners[indices[i]].z;
        
        int faceIdx = i / 6;
        v.nx = normals[faceIdx].x;
        v.ny = normals[faceIdx].y;
        v.nz = normals[faceIdx].z;
        
        v.color = D3DCOLOR_COLORVALUE(color.r, color.g, color.b, color.a);
        v.tu = 0;
        v.tv = 0;
        verts.push_back(v);
    }
    
    Object* obj = new Object(id);
    if (obj->mesh.Create(dev, verts, false)) {
        obj->transform.localPosition = pos;
        obj->SetColor(color.r, color.g, color.b, color.a);
        obj->attachment.SetTransformSystem(g_Scene->GetTransformSystem());
        obj->attachment.SetOwnerId(id);
        return obj;
    }
    delete obj;
    return nullptr;
}

void CreateWall(LPDIRECT3DDEVICE9 dev, int id, D3DXVECTOR3 pos, D3DXVECTOR3 size, D3DCOLORVALUE color) {
    Object* wall = CreateSimpleCube(dev, id, pos, size, color);
    if (wall) {
        g_Scene->AddObject(wall);
        g_Physics.Add(id, pos, size, true, COL_OBB, D3DXVECTOR3(0,0,0), false);
    }
}

LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_CLOSE: DestroyWindow(hWnd); return 0;
        case WM_KEYDOWN:
            if (wParam == VK_F1) {
                g_Wireframe = !g_Wireframe;
                LPDIRECT3DDEVICE9 dev = g_Renderer.GetDevice();
                if (dev) {
                    dev->SetRenderState(D3DRS_FILLMODE, g_Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
                }
            }
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    const char* className = "EngineDemo";
    
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, hInst, NULL, 
                      LoadCursor(NULL, IDC_ARROW), NULL, NULL, className, NULL };
    if (!RegisterClassEx(&wc)) return 0;

    HWND hWnd = CreateWindow(className, "EngineDemo", 
                             WS_OVERLAPPEDWINDOW, 100, 100, 1920, 1080, NULL, NULL, hInst, NULL);
    if (!hWnd) return 0;

    if (!g_Renderer.Init(hWnd)) return 0;
    LPDIRECT3DDEVICE9 dev = g_Renderer.GetDevice();

    Light directionalLight(0);
    D3DCOLORVALUE lightColor = {1.0f, 1.0f, 1.0f, 1.0f};
    directionalLight.SetupDirectional(D3DXVECTOR3(1.0f, -1.0f, 0.5f), lightColor);
    directionalLight.Enable(dev, true);
    
    dev->SetRenderState(D3DRS_AMBIENT, 0x00444444);
    dev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    
    g_Renderer.SetupFog(false, 0, 0, 0);

    g_Input.Init(hWnd);
    g_Res = new ResourceManager(dev);
    g_Cam = new Camera(1920.0f / 1080.0f);
    g_UI.Init(dev, 20, "Consolas");
    g_Scene = new Scene(&g_Physics);
    
	if (!g_Audio.Load("jump.wav", "jump")) {
    	MessageBoxA(NULL, "jump.wav not found", "Error", MB_OK);
	}

    g_Bullets = new ParticleSystem(500);
    g_Bullets->Init(dev);
    
    D3DCOLORVALUE bodyColor = {0.2f, 0.3f, 0.8f, 1.0f};
    g_PlayerBody = CreateSimpleCube(dev, 1, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0.8f, 0.8f, 0.8f), bodyColor);
    
    D3DCOLORVALUE headColor = {1.0f, 0.5f, 0.7f, 1.0f};
    g_PlayerHead = CreateSimpleCube(dev, 2, D3DXVECTOR3(0, 0.6f, 0), D3DXVECTOR3(0.5f, 0.5f, 0.5f), headColor);
    
    if (g_PlayerBody) {
        g_Scene->AddObject(g_PlayerBody);
        g_Physics.Add(1, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0.8f, 0.8f, 0.8f), false, COL_OBB, D3DXVECTOR3(0,0,0), false);
    }
    
    if (g_PlayerHead) {
        g_Scene->AddObject(g_PlayerHead);
        g_Scene->GetTransformSystem()->SetParent(2, 1);
    }
    
    D3DCOLORVALUE groundColor = {0.3f, 0.3f, 0.3f, 1.0f};
    Object* ground = CreateSimpleCube(dev, 100, D3DXVECTOR3(0, -1.0f, 0), D3DXVECTOR3(25.0f, 0.5f, 25.0f), groundColor);
    if (ground) {
        g_Scene->AddObject(ground);
        g_Physics.Add(100, D3DXVECTOR3(0, -1.0f, 0), D3DXVECTOR3(25.0f, 0.5f, 25.0f), true, COL_OBB, D3DXVECTOR3(0,0,0), false);
    }
    
    D3DCOLORVALUE wallColor = {0.7f, 0.2f, 0.2f, 1.0f};
    CreateWall(dev, 101, D3DXVECTOR3(-11, 0.0f, 0), D3DXVECTOR3(0.5f, 3.0f, 22.5f), wallColor);
    CreateWall(dev, 102, D3DXVECTOR3(11, 0.0f, 0), D3DXVECTOR3(0.5f, 3.0f, 22.5f), wallColor);
    CreateWall(dev, 103, D3DXVECTOR3(0, 0.0f, -11), D3DXVECTOR3(22.5f, 3.0f, 0.5f), wallColor);
    CreateWall(dev, 104, D3DXVECTOR3(0, 0.0f, 11), D3DXVECTOR3(22.5f, 3.0f, 0.5f), wallColor);
    
    D3DCOLORVALUE rampColor = {0.2f, 0.8f, 0.3f, 1.0f};
    Object* ramp = CreateSimpleCube(dev, 105, D3DXVECTOR3(5, -1.5f, 8), D3DXVECTOR3(3.0f, 3.0f, 3.0f), rampColor);
    if (ramp) {
        ramp->transform.localRotation.x = 0.4f;
        g_Scene->AddObject(ramp);
        g_Physics.Add(105, D3DXVECTOR3(5, -1.5f, 8), D3DXVECTOR3(3.0f, 3.0f, 3.0f), true, COL_OBB, 
                      D3DXVECTOR3(0.4f, 0, 0), false);
    }
    
    D3DCOLORVALUE bonusColor = {1.0f, 0.9f, 0.2f, 1.0f};
    D3DXVECTOR3 bonusPositions[] = {
        D3DXVECTOR3(-4, -0.6f, 4), D3DXVECTOR3(0, -0.6f, 5), 
        D3DXVECTOR3(-5, -0.6f, -3), D3DXVECTOR3(3, -0.6f, -4)
    };
    
    for (int i = 0; i < 4; i++) {
        Object* bonus = CreateSimpleCube(dev, 200 + i, bonusPositions[i], D3DXVECTOR3(0.4f, 0.4f, 0.4f), bonusColor);
        if (bonus) {
            g_Scene->AddObject(bonus);
            g_Physics.Add(200 + i, bonusPositions[i], D3DXVECTOR3(0.4f, 0.4f, 0.4f), false, COL_OBB, D3DXVECTOR3(0,0,0), false);
        }
    }

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    MSG msg = {0};
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);

    const float FIXED_DELTA = 0.016f;
    float accumulator = 0.0f;
    int frameCount = 0;
    float fpsTimer = 0.0f;
    char fpsText[64];

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            g_Renderer.HandleDeviceLost(hWnd);

            QueryPerformanceCounter(&currentTime);
            float frameTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
            lastTime = currentTime;
            if (frameTime > 0.25f) frameTime = 0.25f;
            accumulator += frameTime;
            
            fpsTimer += frameTime;
            frameCount++;
            if (fpsTimer >= 1.0f) {
                sprintf(fpsText, "FPS: %d", frameCount);
                frameCount = 0;
                fpsTimer = 0.0f;
            }

            g_Input.Update();
            
            float vx = 0.0f, vz = 0.0f;
            bool isMoving = false;
            
            if (g_Input.IsKeyDown('A')) g_PlayerAngle -= 0.05f;
            if (g_Input.IsKeyDown('D')) g_PlayerAngle += 0.05f;
            
            if (g_Input.IsKeyDown(VK_UP))    g_PlayerPitch += 0.03f;
            if (g_Input.IsKeyDown(VK_DOWN))  g_PlayerPitch -= 0.03f;
            if (g_PlayerPitch > 0.9f)  g_PlayerPitch = 0.9f;
            if (g_PlayerPitch < -0.9f) g_PlayerPitch = -0.9f;
            
            if (g_Input.IsKeyDown('W')) {
                vx += sinf(g_PlayerAngle) * g_MoveSpeed;
                vz += cosf(g_PlayerAngle) * g_MoveSpeed;
                isMoving = true;
            }
            if (g_Input.IsKeyDown('S')) {
                vx -= sinf(g_PlayerAngle) * g_MoveSpeed;
                vz -= cosf(g_PlayerAngle) * g_MoveSpeed;
                isMoving = true;
            }
            
            if (g_Input.IsKeyDown(VK_SPACE)) {
                g_JumpRequested = true;
            }
            
            bool wantShoot = g_Input.IsKeyDown(VK_LBUTTON) || g_Input.IsKeyDown(VK_LCONTROL);
            
            while (accumulator >= FIXED_DELTA) {
                g_Physics.SetVelocity(g_PlayerBody->id, vx, vz);
                
                if (g_JumpRequested) {
    				if (g_Physics.GetPosition(g_PlayerBody->id).y == g_PlayerBody->transform.globalPosition.y) {
    				}
    				g_Physics.Jump(g_PlayerBody->id, 0.2f);
    				g_Audio.Play("jump");
    				g_JumpRequested = false;
				}
                
                if (wantShoot && g_ShootCooldown <= 0) {
                    D3DXVECTOR3 spawnPos = g_PlayerHead->transform.globalPosition;
                    
                    spawnPos.x += sinf(g_PlayerAngle) * 0.35f;
                    spawnPos.z += cosf(g_PlayerAngle) * 0.35f;
                    
                    D3DXVECTOR3 direction;
                    direction.x = sinf(g_PlayerAngle) * cosf(g_PlayerPitch);
                    direction.y = sinf(g_PlayerPitch);
                    direction.z = cosf(g_PlayerAngle) * cosf(g_PlayerPitch);
                    D3DXVec3Normalize(&direction, &direction);
                    
                    g_Bullets->Add(spawnPos, direction * 0.5f, 0xFFFFCC00, 1.5f, 0.08f);
                    g_ShootCooldown = g_ShootDelay;
                }
                if (g_ShootCooldown > 0) g_ShootCooldown -= FIXED_DELTA;
                
                g_Physics.Update(FIXED_DELTA);
                
                if (g_PlayerBody) {
                    D3DXVECTOR3 bodyRot = g_PlayerBody->transform.localRotation;
                    bodyRot.y = g_PlayerAngle;
                    g_PlayerBody->transform.localRotation = bodyRot;
                }
                if (g_PlayerHead) {
                    D3DXVECTOR3 headRot = g_PlayerHead->transform.localRotation;
                    headRot.y = g_PlayerAngle;
                    headRot.x = g_PlayerPitch * 0.5f;
                    g_PlayerHead->transform.localRotation = headRot;
                }
                
                g_Scene->GetTransformSystem()->Update();
                
                g_Scene->Update(FIXED_DELTA, dev);
                
                g_Bullets->Update(FIXED_DELTA);
                
                accumulator -= FIXED_DELTA;
            }
            D3DXVECTOR3 targetPos = g_PlayerBody ? g_PlayerBody->transform.globalPosition : D3DXVECTOR3(0,0,0);
            float camDist = 10.0f;
            float camHeight = 5.0f;
            
            D3DXVECTOR3 camOffset;
            camOffset.x = -sinf(g_PlayerAngle) * camDist;
            camOffset.z = -cosf(g_PlayerAngle) * camDist;
            camOffset.y = camHeight + sinf(g_PlayerPitch) * 2.0f;
            
            D3DXVECTOR3 camPos = targetPos + camOffset;
            
            D3DXMATRIX view, proj;
            D3DXVECTOR3 upVec(0, 1, 0);
            D3DXMatrixLookAtLH(&view, &camPos, &targetPos, &upVec);
            D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 4.0f, 1920.0f / 1080.0f, 0.5f, 500.0f);
            
            dev->SetTransform(D3DTS_VIEW, &view);
            dev->SetTransform(D3DTS_PROJECTION, &proj);
            
            Frustum frustum;
            frustum.Construct(view, proj);
            
            g_Renderer.Begin();
            
            g_Scene->DrawAll(dev, frustum);
            g_Bullets->Draw(dev, camPos);
            
            g_Renderer.SetupFog(false, 0, 0, 0);
            
            if (g_PlayerBody) {
                D3DXVECTOR3 pos = g_PlayerBody->transform.globalPosition;
                char buf[128];
                sprintf(buf, "%.2f, %.2f, %.2f  Angle: %.1f° | %s", 
                        pos.x, pos.y, pos.z, g_PlayerAngle * 57.3f, fpsText);
                g_UI.Draw(buf, 25, 25, 0xFFAAFFAA);
                g_UI.Draw("LMB/Ctrl - Shoot | F1 - Wireframe", 25, 55, 0xFFAAAAAA);
            }
            g_Renderer.End();
        }
    }

    delete g_Bullets;
    delete g_Scene;
    delete g_Cam;
    delete g_Res;
    UnregisterClass(className, hInst);
    return 0;
}
