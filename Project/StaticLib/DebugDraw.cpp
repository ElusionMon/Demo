#include "MyEngine.h"

namespace MyEngine {

    DebugDraw::DebugDraw() : dev(nullptr), initialized(false) {}

    DebugDraw::~DebugDraw() {
        Shutdown();
    }

    void DebugDraw::Init(LPDIRECT3DDEVICE9 device) {
        dev = device;
        initialized = true;
    }

    void DebugDraw::Shutdown() {
        dev = nullptr;
        initialized = false;
    }

    void DebugDraw::DrawOBB(const D3DXVECTOR3& center, const D3DXVECTOR3& halfSize, const D3DXMATRIX& rot, DWORD color) {
        if (!initialized || !dev) return;

        D3DXVECTOR3 local[8] = {
            D3DXVECTOR3(-halfSize.x, -halfSize.y, -halfSize.z),
            D3DXVECTOR3( halfSize.x, -halfSize.y, -halfSize.z),
            D3DXVECTOR3( halfSize.x, -halfSize.y,  halfSize.z),
            D3DXVECTOR3(-halfSize.x, -halfSize.y,  halfSize.z),
            D3DXVECTOR3(-halfSize.x,  halfSize.y, -halfSize.z),
            D3DXVECTOR3( halfSize.x,  halfSize.y, -halfSize.z),
            D3DXVECTOR3( halfSize.x,  halfSize.y,  halfSize.z),
            D3DXVECTOR3(-halfSize.x,  halfSize.y,  halfSize.z)
        };

        D3DXVECTOR3 world[8];
        for (int i = 0; i < 8; i++) {
            D3DXVec3TransformCoord(&world[i], &local[i], &rot);
            world[i] += center;
        }

        int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };

        D3DXMATRIX oldWorld, oldView, oldProj;
        dev->GetTransform(D3DTS_WORLD, &oldWorld);
        dev->GetTransform(D3DTS_VIEW, &oldView);
        dev->GetTransform(D3DTS_PROJECTION, &oldProj);
        
        D3DXMATRIX identity;
        D3DXMatrixIdentity(&identity);
        dev->SetTransform(D3DTS_WORLD, &identity);
        dev->SetTransform(D3DTS_VIEW, &oldView);
        dev->SetTransform(D3DTS_PROJECTION, &oldProj);

        DWORD zEnable, zWriteEnable, lighting, cullMode;
        dev->GetRenderState(D3DRS_ZENABLE, &zEnable);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zWriteEnable);
        dev->GetRenderState(D3DRS_LIGHTING, &lighting);
        dev->GetRenderState(D3DRS_CULLMODE, &cullMode);

        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        for (int i = 0; i < 12; i++) {
            Vertex verts[2];
            verts[0].x = world[edges[i][0]].x;
            verts[0].y = world[edges[i][0]].y;
            verts[0].z = world[edges[i][0]].z;
            verts[0].color = color;
            verts[0].nx = verts[0].ny = verts[0].nz = 0;
            verts[0].tu = verts[0].tv = 0;

            verts[1].x = world[edges[i][1]].x;
            verts[1].y = world[edges[i][1]].y;
            verts[1].z = world[edges[i][1]].z;
            verts[1].color = color;
            verts[1].nx = verts[1].ny = verts[1].nz = 0;
            verts[1].tu = verts[1].tv = 0;

            dev->SetFVF(Vertex::FVF);
            dev->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(Vertex));
        }

        dev->SetRenderState(D3DRS_CULLMODE, cullMode);
        dev->SetRenderState(D3DRS_ZENABLE, zEnable);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
        dev->SetRenderState(D3DRS_LIGHTING, lighting);
        
        dev->SetTransform(D3DTS_WORLD, &oldWorld);
    }

    void DebugDraw::DrawAABB(const D3DXVECTOR3& min, const D3DXVECTOR3& max, DWORD color) {
        if (!initialized || !dev) return;

        D3DXVECTOR3 corners[8] = {
            D3DXVECTOR3(min.x, min.y, min.z),
            D3DXVECTOR3(max.x, min.y, min.z),
            D3DXVECTOR3(max.x, min.y, max.z),
            D3DXVECTOR3(min.x, min.y, max.z),
            D3DXVECTOR3(min.x, max.y, min.z),
            D3DXVECTOR3(max.x, max.y, min.z),
            D3DXVECTOR3(max.x, max.y, max.z),
            D3DXVECTOR3(min.x, max.y, max.z)
        };

        int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0},
            {4,5}, {5,6}, {6,7}, {7,4},
            {0,4}, {1,5}, {2,6}, {3,7}
        };

        DWORD zEnable, zWriteEnable, lighting, cullMode;
        dev->GetRenderState(D3DRS_ZENABLE, &zEnable);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zWriteEnable);
        dev->GetRenderState(D3DRS_LIGHTING, &lighting);
        dev->GetRenderState(D3DRS_CULLMODE, &cullMode);

        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

        for (int i = 0; i < 12; i++) {
            Vertex verts[2];
            verts[0].x = corners[edges[i][0]].x;
            verts[0].y = corners[edges[i][0]].y;
            verts[0].z = corners[edges[i][0]].z;
            verts[0].color = color;
            verts[0].nx = verts[0].ny = verts[0].nz = 0;
            verts[0].tu = verts[0].tv = 0;

            verts[1].x = corners[edges[i][1]].x;
            verts[1].y = corners[edges[i][1]].y;
            verts[1].z = corners[edges[i][1]].z;
            verts[1].color = color;
            verts[1].nx = verts[1].ny = verts[1].nz = 0;
            verts[1].tu = verts[1].tv = 0;

            dev->SetFVF(Vertex::FVF);
            dev->DrawPrimitiveUP(D3DPT_LINELIST, 1, verts, sizeof(Vertex));
        }

        dev->SetRenderState(D3DRS_CULLMODE, cullMode);
        dev->SetRenderState(D3DRS_ZENABLE, zEnable);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
        dev->SetRenderState(D3DRS_LIGHTING, lighting);
    }
    
    void DebugDraw::DrawSphere(const D3DXVECTOR3& center, float radius, DWORD color) {
        const int segments = 16;
        Vertex vertices[200];
        int vertexCount = 0;
        
        for (int i = 0; i <= segments; i++) {
            float angle1 = 2.0f * D3DX_PI * i / segments;
            float angle2 = 2.0f * D3DX_PI * (i + 1) / segments;
            
            D3DXVECTOR3 p1(center.x + radius * cosf(angle1), center.y + radius * sinf(angle1), center.z);
            D3DXVECTOR3 p2(center.x + radius * cosf(angle2), center.y + radius * sinf(angle2), center.z);
            
            vertices[vertexCount].x = p1.x; vertices[vertexCount].y = p1.y; vertices[vertexCount].z = p1.z;
            vertices[vertexCount].color = color; vertexCount++;
            vertices[vertexCount].x = p2.x; vertices[vertexCount].y = p2.y; vertices[vertexCount].z = p2.z;
            vertices[vertexCount].color = color; vertexCount++;
        }
        
        for (int i = 0; i <= segments; i++) {
            float angle1 = 2.0f * D3DX_PI * i / segments;
            float angle2 = 2.0f * D3DX_PI * (i + 1) / segments;
            
            D3DXVECTOR3 p1(center.x + radius * cosf(angle1), center.y, center.z + radius * sinf(angle1));
            D3DXVECTOR3 p2(center.x + radius * cosf(angle2), center.y, center.z + radius * sinf(angle2));
            
            vertices[vertexCount].x = p1.x; vertices[vertexCount].y = p1.y; vertices[vertexCount].z = p1.z;
            vertices[vertexCount].color = color; vertexCount++;
            vertices[vertexCount].x = p2.x; vertices[vertexCount].y = p2.y; vertices[vertexCount].z = p2.z;
            vertices[vertexCount].color = color; vertexCount++;
        }
        
        for (int i = 0; i <= segments; i++) {
            float angle1 = 2.0f * D3DX_PI * i / segments;
            float angle2 = 2.0f * D3DX_PI * (i + 1) / segments;
            
            D3DXVECTOR3 p1(center.x, center.y + radius * cosf(angle1), center.z + radius * sinf(angle1));
            D3DXVECTOR3 p2(center.x, center.y + radius * cosf(angle2), center.z + radius * sinf(angle2));
            
            vertices[vertexCount].x = p1.x; vertices[vertexCount].y = p1.y; vertices[vertexCount].z = p1.z;
            vertices[vertexCount].color = color; vertexCount++;
            vertices[vertexCount].x = p2.x; vertices[vertexCount].y = p2.y; vertices[vertexCount].z = p2.z;
            vertices[vertexCount].color = color; vertexCount++;
        }
        
        DWORD zEnable, zWriteEnable, lighting, cullMode;
        dev->GetRenderState(D3DRS_ZENABLE, &zEnable);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zWriteEnable);
        dev->GetRenderState(D3DRS_LIGHTING, &lighting);
        dev->GetRenderState(D3DRS_CULLMODE, &cullMode);
        
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        
        dev->SetFVF(Vertex::FVF);
        dev->DrawPrimitiveUP(D3DPT_LINELIST, vertexCount / 2, vertices, sizeof(Vertex));
        
        dev->SetRenderState(D3DRS_CULLMODE, cullMode);
        dev->SetRenderState(D3DRS_ZENABLE, zEnable);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnable);
        dev->SetRenderState(D3DRS_LIGHTING, lighting);
    }

    void DebugDraw::DrawCapsule(const D3DXVECTOR3& center, float radius, float height, const D3DXMATRIX& rot, DWORD color) {
        D3DXVECTOR3 halfSize(radius, height * 0.5f, radius);
        DrawOBB(center, halfSize, rot, color);
    }
}
