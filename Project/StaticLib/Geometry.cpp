#include "MyEngine.h"
#include <algorithm>

namespace MyEngine {

    Mesh::~Mesh() { if (vb) vb->Release(); }

    bool Mesh::Create(LPDIRECT3DDEVICE9 dev, const std::vector<Vertex>& verts, bool dyn) {
        vCount = verts.size(); 
        if (vCount <= 0) return false;
        
        baseVertices = verts;
        
        DWORD usage = dyn ? D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY : 0;
        D3DPOOL pool = dyn ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
        
        if (FAILED(dev->CreateVertexBuffer(vCount * sizeof(Vertex), usage, Vertex::FVF, pool, &vb, NULL))) 
            return false;
        
        UpdateVertices(verts);
        
        D3DCOLORVALUE defColor = {1.0f, 1.0f, 1.0f, 1.0f};
        SetMaterial(defColor, defColor, 30.0f);
        return true;
    }

    void Mesh::UpdateVertices(const std::vector<Vertex>& verts) {
        if (!vb || verts.empty()) return; 
        void* pData; 
        if (SUCCEEDED(vb->Lock(0, 0, &pData, 0))) {
            memcpy(pData, verts.data(), verts.size() * sizeof(Vertex)); 
            vb->Unlock();
        }
        if (!verts.empty()) {
            baseVertices = verts;
        }
        UpdateBounds();
    }

    void Mesh::UpdateBounds() {
        if (!vb || vCount <= 0) return;
        if (baseVertices.empty()) return;
        
        boundsMin = D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX);
        boundsMax = D3DXVECTOR3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        
        for (int i = 0; i < vCount; i++) {
            boundsMin.x = (std::min)(boundsMin.x, baseVertices[i].x);
            boundsMin.y = (std::min)(boundsMin.y, baseVertices[i].y);
            boundsMin.z = (std::min)(boundsMin.z, baseVertices[i].z);
            boundsMax.x = (std::max)(boundsMax.x, baseVertices[i].x);
            boundsMax.y = (std::max)(boundsMax.y, baseVertices[i].y);
            boundsMax.z = (std::max)(boundsMax.z, baseVertices[i].z);
        }
        hasBounds = true;
    }
    
    void Mesh::GetBounds(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax) {
        outMin = boundsMin;
        outMax = boundsMax;
    }

    void Mesh::AnimateWave(float time, float speed, float amplitude) {
        if (!vb || vCount <= 0 || baseVertices.empty()) return; 
        Vertex* pV;
        if (SUCCEEDED(vb->Lock(0, 0, (void**)&pV, 0))) {
            for (int i = 0; i < vCount; i++) {
                float baseY = baseVertices[i].y;
                pV[i].x = baseVertices[i].x;
                pV[i].z = baseVertices[i].z;
                pV[i].y = baseY + sinf(baseVertices[i].x * speed + time) * amplitude;
                pV[i].nx = baseVertices[i].nx;
                pV[i].ny = baseVertices[i].ny;
                pV[i].nz = baseVertices[i].nz;
                pV[i].color = baseVertices[i].color;
                pV[i].tu = baseVertices[i].tu;
                pV[i].tv = baseVertices[i].tv;
            }
            vb->Unlock();
        }
    }

    void Mesh::SetMaterial(D3DCOLORVALUE d, D3DCOLORVALUE s, float p) {
        ZeroMemory(&material, sizeof(material));
        material.Diffuse = d; 
        material.Ambient = d; 
        material.Specular = s; 
        material.Power = p;
    }

    void Mesh::Draw(LPDIRECT3DDEVICE9 dev, D3DXMATRIX* world) {
        if (vCount <= 0 || !vb) return;
        dev->SetTransform(D3DTS_WORLD, world); 
        dev->SetMaterial(&material);
        dev->SetTexture(0, texture ? texture.get() : NULL);
        dev->SetStreamSource(0, vb, 0, sizeof(Vertex)); 
        dev->SetFVF(Vertex::FVF);
        dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, vCount / 3);
    }

    bool Mesh::LoadAnimated(LPDIRECT3DDEVICE9 dev, std::string path) {
        std::ifstream file(path.c_str()); 
        if (!file.is_open()) return false;
        
        std::string line; 
        int currentFrameIndex = -1;
        baseVertices.clear(); 
        keyframes.clear();

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line[0] == '#') {
                if (line.find("FRAME") != std::string::npos) {
                    AnimKeyframe newFrame; 
                    keyframes.push_back(newFrame);
                    currentFrameIndex = keyframes.size() - 1;
                }
                continue;
            }
            std::stringstream ss(line);
            if (currentFrameIndex == -1) {
                Vertex v; 
                std::string colorStr;
                if (ss >> v.x >> v.y >> v.z >> v.nx >> v.ny >> v.nz >> colorStr >> v.tu >> v.tv) {
                    v.color = strtoul(colorStr.c_str(), NULL, 0); 
                    baseVertices.push_back(v);
                }
            } else {
                D3DXVECTOR3 pos, norm;
                if (ss >> pos.x >> pos.y >> pos.z >> norm.x >> norm.y >> norm.z) {
                    keyframes[currentFrameIndex].positions.push_back(pos);
                    keyframes[currentFrameIndex].normals.push_back(norm);
                }
            }
        }
        file.close();
        
        if (baseVertices.empty()) return false;
        
        for (size_t i = 0; i < keyframes.size(); i++) {
            if (keyframes[i].positions.size() != baseVertices.size() ||
                keyframes[i].normals.size() != baseVertices.size()) {
                keyframes.clear();
                return false;
            }
        }
        
        vCount = baseVertices.size();
        return Create(dev, baseVertices, true);
    }

    void Mesh::UpdateAnimation(LPDIRECT3DDEVICE9 dev, float dt, float speed, int startFrame, int endFrame, bool loop) {
        if (keyframes.empty() || !vb) return;
        if (startFrame < 0) startFrame = 0;
        if (endFrame >= (int)keyframes.size()) endFrame = keyframes.size() - 1;
        if (startFrame > endFrame) return;

        currentFrame += speed * dt;
        if (currentFrame > (float)endFrame) currentFrame = loop ? (float)startFrame : (float)endFrame;

        int frameA = (int)currentFrame; 
        int frameB = frameA + 1;
        if (frameB > endFrame) frameB = loop ? startFrame : endFrame;
        float t = currentFrame - (float)frameA;
        
        Vertex* pVertices = nullptr;
        if (SUCCEEDED(vb->Lock(0, 0, (void**)&pVertices, D3DLOCK_DISCARD))) {
            for (int i = 0; i < vCount; i++) {
                pVertices[i].color = baseVertices[i].color;
                pVertices[i].tu    = baseVertices[i].tu;
                pVertices[i].tv    = baseVertices[i].tv;

                D3DXVECTOR3 posA = keyframes[frameA].positions[i];
                D3DXVECTOR3 posB = keyframes[frameB].positions[i];
                D3DXVECTOR3 finalPos;
                D3DXVec3Lerp(&finalPos, &posA, &posB, t);
                pVertices[i].x = finalPos.x;
                pVertices[i].y = finalPos.y;
                pVertices[i].z = finalPos.z;

                D3DXVECTOR3 normA = keyframes[frameA].normals[i];
                D3DXVECTOR3 normB = keyframes[frameB].normals[i];
                D3DXVECTOR3 finalNorm;
                D3DXVec3Lerp(&finalNorm, &normA, &normB, t);
                pVertices[i].nx = finalNorm.x;
                pVertices[i].ny = finalNorm.y;
                pVertices[i].nz = finalNorm.z;
                
                baseVertices[i].x = finalPos.x;
                baseVertices[i].y = finalPos.y;
                baseVertices[i].z = finalPos.z;
                baseVertices[i].nx = finalNorm.x;
                baseVertices[i].ny = finalNorm.y;
                baseVertices[i].nz = finalNorm.z;
            }
            vb->Unlock();
            UpdateBounds();
        }
    }

    
    Skybox::~Skybox() { if (vb) vb->Release(); if (ib) ib->Release(); }
    
    bool Skybox::Create(LPDIRECT3DDEVICE9 dev, std::shared_ptr<IDirect3DTexture9> tex) {
        texture = tex; 
        const int slices = 10, stacks = 10;
        numVertices = (slices + 1) * (stacks + 1); 
        numTriangles = slices * stacks * 2;
        
        if (FAILED(dev->CreateVertexBuffer(numVertices * sizeof(Vertex), 0, Vertex::FVF, D3DPOOL_MANAGED, &vb, NULL))) 
            return false;
        if (FAILED(dev->CreateIndexBuffer(numTriangles * 3 * sizeof(WORD), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ib, NULL))) 
            return false;
        Vertex* v; 
        vb->Lock(0, 0, (void**)&v, 0); 
        int cV = 0;
        for (int i = 0; i <= stacks; ++i) {
            float phi = D3DX_PI * (float)i / (float)stacks;
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * D3DX_PI * (float)j / (float)slices;
                float x = sinf(phi) * cosf(theta), y = cosf(phi), z = sinf(phi) * sinf(theta);
                v[cV].x = x; v[cV].y = y; v[cV].z = z; 
                v[cV].nx = -x; v[cV].ny = -y; v[cV].nz = -z;
                v[cV].color = 0xFFFFFFFF; 
                v[cV].tu = (float)j / slices; 
                v[cV].tv = (float)i / stacks; 
                cV++;
            }
        }
        vb->Unlock();
        
        WORD* idx; 
        ib->Lock(0, 0, (void**)&idx, 0); 
        int cI = 0;
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int f = (i * (slices + 1)) + j, s = f + slices + 1;
                idx[cI++] = f; idx[cI++] = s; idx[cI++] = f + 1; 
                idx[cI++] = s; idx[cI++] = s + 1; idx[cI++] = f + 1;
            }
        }
        ib->Unlock(); 
        return true;
    }

    void Skybox::Update(float dt, float speed) { 
        rotation += speed * dt; 
        if (rotation > D3DX_PI * 2) rotation -= D3DX_PI * 2; 
    }

    void Skybox::Draw(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 camPos) {
        if (!vb || !ib) return;
        
        DWORD lightingEnabled, zEnable, zWriteEnabled, cullMode, alphaBlendEnable;
        dev->GetRenderState(D3DRS_LIGHTING, &lightingEnabled);
        dev->GetRenderState(D3DRS_ZENABLE, &zEnable);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zWriteEnabled);
        dev->GetRenderState(D3DRS_CULLMODE, &cullMode);
        dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlendEnable);
        
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        
        D3DXMATRIX mS, mR, mT, mW; 
        D3DXMatrixScaling(&mS, 15.0f, 15.0f, 15.0f); 
        D3DXMatrixRotationY(&mR, rotation);
        D3DXMatrixTranslation(&mT, camPos.x, camPos.y, camPos.z); 
        mW = mS * mR * mT;
        
        dev->SetTransform(D3DTS_WORLD, &mW); 
        dev->SetTexture(0, texture ? texture.get() : NULL);
        dev->SetStreamSource(0, vb, 0, sizeof(Vertex)); 
        dev->SetIndices(ib); 
        dev->SetFVF(Vertex::FVF);
        dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, numVertices, 0, numTriangles);
        
        dev->SetRenderState(D3DRS_CULLMODE, cullMode);
        dev->SetRenderState(D3DRS_ZENABLE, zEnable);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, zWriteEnabled);
        dev->SetRenderState(D3DRS_LIGHTING, lightingEnabled);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlendEnable);
    }
    
    std::vector<Vertex> LoadVertices(std::string path) {
        std::vector<Vertex> v; 
        std::ifstream f(path.c_str()); 
        if (!f.is_open()) return v; 
        std::string l;
        while (std::getline(f, l)) {
            if (l.empty() || l[0] == '#') continue; 
            std::stringstream ss(l); 
            Vertex t; 
            std::string c;
            if (ss >> t.x >> t.y >> t.z >> t.nx >> t.ny >> t.nz >> c >> t.tu >> t.tv) {
                t.color = strtoul(c.c_str(), NULL, 0); 
                v.push_back(t);
            }
        }
        f.close(); 
        return v;
    }
}
