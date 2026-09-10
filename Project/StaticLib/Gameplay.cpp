#include "MyEngine.h"

namespace MyEngine {
    
    Light::Light(int lightIndex) : index(lightIndex), enabled(false) {
        ZeroMemory(&lightData, sizeof(lightData));
    }

    void Light::SetupDirectional(D3DXVECTOR3 direction, D3DCOLORVALUE color) {
        ZeroMemory(&lightData, sizeof(lightData));
        lightData.Type = D3DLIGHT_DIRECTIONAL;
        lightData.Diffuse = color;
        lightData.Ambient.r = color.r * 0.2f; 
        lightData.Ambient.g = color.g * 0.2f; 
        lightData.Ambient.b = color.b * 0.2f; 
        lightData.Ambient.a = 1.0f;
        
        D3DXVECTOR3 normDir;
        D3DXVec3Normalize(&normDir, &direction);
        lightData.Direction = *(D3DVECTOR*)&normDir;
    }

    void Light::Enable(LPDIRECT3DDEVICE9 dev, bool state) {
        enabled = state;
        dev->SetLight(index, &lightData);
        dev->LightEnable(index, enabled ? TRUE : FALSE);
    }

    void Light::SetDirection(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 direction) {
        D3DXVECTOR3 normDir;
        D3DXVec3Normalize(&normDir, &direction);
        lightData.Direction = *(D3DVECTOR*)&normDir;
        if (enabled) {
            dev->SetLight(index, &lightData);
        }
    }
    
    void Effect::Play(Type t, float i) { active = true; intensity = i; timer = i; }
    
    void Effect::Update(Mesh& m, float dt) {
        if (!active) { shakeOffset.x = 0; shakeOffset.y = 0; shakeOffset.z = 0; return; }
        timer -= dt; if (timer <= 0) { active = false; return; }
        shakeOffset.x = ((rand() % 100) - 50) / 50.0f * intensity * timer;
        shakeOffset.y = ((rand() % 100) - 50) / 50.0f * intensity * timer;
        shakeOffset.z = 0;
    }
    
    ParticleSystem::ParticleSystem(int count) : maxParticles(count) {}
    
    ParticleSystem::~ParticleSystem() { if (vb) vb->Release(); }
    
    bool ParticleSystem::Init(LPDIRECT3DDEVICE9 dev) {
        return SUCCEEDED(dev->CreateVertexBuffer(maxParticles * 6 * sizeof(Vertex), 
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, Vertex::FVF, D3DPOOL_DEFAULT, &vb, NULL));
    }
    
    void ParticleSystem::Add(D3DXVECTOR3 p, D3DXVECTOR3 v, DWORD c, float l, float size) { 
        if (particles.size() < (size_t)maxParticles) { 
            Particle prt; 
            prt.pos = p; 
            prt.vel = v; 
            prt.life = 1.0f; 
            prt.maxLife = l; 
            prt.color = c;
            prt.size = size;
            particles.push_back(prt); 
        } 
    }
    
    void ParticleSystem::Update(float dt) {
        for (auto& p : particles) {
            p.pos.x += p.vel.x * dt; 
            p.pos.y += p.vel.y * dt; 
            p.pos.z += p.vel.z * dt; 
            p.life -= dt / p.maxLife;
        }
        
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.life <= 0; }),
            particles.end()
        );
    }
    
    void ParticleSystem::Draw(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 camPos) {
        if (particles.empty() || !vb) return;
        
        cameraPos = camPos;
        std::sort(particles.begin(), particles.end(),
            [this](const Particle& a, const Particle& b) {
                float distA = (a.pos.x - cameraPos.x)*(a.pos.x - cameraPos.x) +
                             (a.pos.y - cameraPos.y)*(a.pos.y - cameraPos.y) + 
                             (a.pos.z - cameraPos.z)*(a.pos.z - cameraPos.z);
                float distB = (b.pos.x - cameraPos.x)*(b.pos.x - cameraPos.x) +
                              (b.pos.y - cameraPos.y)*(b.pos.y - cameraPos.y) +
                              (b.pos.z - cameraPos.z)*(b.pos.z - cameraPos.z);
                return distA > distB;
            });
        
        DWORD alphaEnable, srcBlend, destBlend, zEnable, zWrite, lighting, cullMode;
        dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaEnable);
        dev->GetRenderState(D3DRS_SRCBLEND, &srcBlend);
        dev->GetRenderState(D3DRS_DESTBLEND, &destBlend);
        dev->GetRenderState(D3DRS_ZENABLE, &zEnable);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zWrite);
        dev->GetRenderState(D3DRS_LIGHTING, &lighting);
        dev->GetRenderState(D3DRS_CULLMODE, &cullMode);
        
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_LIGHTING, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        
        Vertex* v; 
        if (SUCCEEDED(vb->Lock(0, 0, (void**)&v, D3DLOCK_DISCARD))) {
            int idx = 0; 
            for (size_t i = 0; i < particles.size(); i++) {
                Particle& p = particles[i];
                float s = p.size;
                
                DWORD finalColor = p.color;
                float alpha = p.life * 255.0f;
                finalColor = (finalColor & 0x00FFFFFF) | ((int)alpha << 24);
                
                v[idx].x = p.pos.x - s; v[idx].y = p.pos.y - s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 0; v[idx].tv = 1; idx++;
                
                v[idx].x = p.pos.x - s; v[idx].y = p.pos.y + s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 0; v[idx].tv = 0; idx++;
                
                v[idx].x = p.pos.x + s; v[idx].y = p.pos.y + s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 1; v[idx].tv = 0; idx++;
                
                v[idx].x = p.pos.x - s; v[idx].y = p.pos.y - s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 0; v[idx].tv = 1; idx++;
                
                v[idx].x = p.pos.x + s; v[idx].y = p.pos.y + s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 1; v[idx].tv = 0; idx++;
                
                v[idx].x = p.pos.x + s; v[idx].y = p.pos.y - s; v[idx].z = p.pos.z; 
                v[idx].nx = 0; v[idx].ny = 1; v[idx].nz = 0; v[idx].color = finalColor; 
                v[idx].tu = 1; v[idx].tv = 1; idx++;
            }
            vb->Unlock(); 
            dev->SetStreamSource(0, vb, 0, sizeof(Vertex)); 
            dev->SetTexture(0, NULL); 
            dev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, particles.size() * 2);
        }
        
        dev->SetRenderState(D3DRS_ZENABLE, zEnable);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, zWrite);
        dev->SetRenderState(D3DRS_LIGHTING, lighting);
        dev->SetRenderState(D3DRS_CULLMODE, cullMode);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaEnable);
        dev->SetRenderState(D3DRS_SRCBLEND, srcBlend);
        dev->SetRenderState(D3DRS_DESTBLEND, destBlend);
    }
    
    void ParticleSystem::Clear() {
        particles.clear();
    }
    
    D3DXVECTOR3 AttachPoint::GetWorldPosition(const D3DXVECTOR3& parentPos, const D3DXMATRIX& parentRot) {
        D3DXVECTOR3 worldOffset;
        D3DXVec3TransformCoord(&worldOffset, &offset, &parentRot);
        return parentPos + worldOffset;
    }
    
    AttachComponent::AttachComponent() : transformSystem(nullptr), ownerId(-1) {}
    
    AttachComponent::~AttachComponent() {
        DetachAll();
    }
    
    void AttachComponent::AddSlot(const std::string& name, const D3DXVECTOR3& offset, const D3DXVECTOR3& rotation) {
        AttachPoint point;
        point.name = name;
        point.offset = offset;
        point.rotation = rotation;
        slots[name] = point;
    }
    
    void AttachComponent::AddSlot(AttachSlotType type, const D3DXVECTOR3& offset, const D3DXVECTOR3& rotation) {
        std::string name = SlotTypeToString(type);
        AddSlot(name, offset, rotation);
    }
    
    bool AttachComponent::HasSlot(const std::string& name) const {
        return slots.find(name) != slots.end();
    }
    
    const AttachPoint* AttachComponent::GetSlot(const std::string& name) const {
        auto it = slots.find(name);
        return (it != slots.end()) ? &it->second : nullptr;
    }
    
    bool AttachComponent::Attach(int childId, const std::string& slotName, const D3DXVECTOR3& offset, float scale) {
    if (!HasSlot(slotName)) return false;
    
    const AttachPoint* slot = GetSlot(slotName);
    if (!slot) return false;
    
    for (auto& att : attachedChildren) {
        if (att.childId == childId && att.isAttached) {
            return false;
        }
    }
    
    AttachmentInfo info;
    info.childId = childId;
    info.parentId = ownerId;
    info.slotName = slotName;
    info.slotType = StringToSlotType(slotName);
    info.localOffset = offset;
    info.localRotation = slot->rotation;
    info.isAttached = true;
    info.scale = scale;
    
    attachedChildren.push_back(info);
    
    if (transformSystem && ownerId != -1) {
        transformSystem->SetParent(childId, ownerId);
        
        TransformComponent* childTransform = transformSystem->GetTransform(childId);
        if (childTransform) {
            childTransform->SetLocalPosition(slot->offset + offset);
            childTransform->SetLocalRotation(slot->rotation);
        }
    }
    
    return true;
}
    
    bool AttachComponent::Attach(int childId, AttachSlotType slotType, const D3DXVECTOR3& offset, float scale) {
        return Attach(childId, SlotTypeToString(slotType), offset, scale);
    }
    
    void AttachComponent::Detach(int childId) {
        attachedChildren.erase(
            std::remove_if(attachedChildren.begin(), attachedChildren.end(),
                [childId](const AttachmentInfo& a) { return a.childId == childId; }),
            attachedChildren.end()
        );
        
        if (transformSystem) {
            transformSystem->SetParent(childId, -1);
        }
    }
    
    void AttachComponent::DetachFromSlot(const std::string& slotName) {
        for (auto& att : attachedChildren) {
            if (att.slotName == slotName && att.isAttached) {
                if (transformSystem) {
                    transformSystem->SetParent(att.childId, -1);
                }
            }
        }
        
        attachedChildren.erase(
            std::remove_if(attachedChildren.begin(), attachedChildren.end(),
                [slotName](const AttachmentInfo& a) { return a.slotName == slotName; }),
            attachedChildren.end()
        );
    }
    
    void AttachComponent::DetachAll() {
        for (auto& att : attachedChildren) {
            if (transformSystem && att.isAttached) {
                transformSystem->SetParent(att.childId, -1);
            }
        }
        attachedChildren.clear();
    }
    
    bool AttachComponent::IsAttached(int childId) const {
        for (const auto& att : attachedChildren) {
            if (att.childId == childId && att.isAttached) {
                return true;
            }
        }
        return false;
    }
    
    bool AttachComponent::IsAttached(Object* child) const {
        if (!child) return false;
        return IsAttached(child->id);
    }
    
    D3DXVECTOR3 AttachComponent::GetSlotWorldPosition(Object* owner, const std::string& slotName) {
        const AttachPoint* slot = GetSlot(slotName);
        if (!slot || !owner) return owner ? owner->transform.globalPosition : D3DXVECTOR3(0,0,0);
        
        D3DXVECTOR3 worldOffset;
        D3DXVec3TransformCoord(&worldOffset, &slot->offset, &owner->transform.globalMatrix);
        
        return owner->transform.globalPosition + worldOffset;
    }
    
    void AttachComponent::Update(Object* owner, float dt) {
        if (!transformSystem || !owner) return;
        
        for (auto& att : attachedChildren) {
            if (!att.isAttached) continue;
            
            TransformComponent* childTransform = transformSystem->GetTransform(att.childId);
            if (childTransform) {
                att.parentId = childTransform->parentId;
            }
        }
    }
    
    std::string AttachComponent::SlotTypeToString(AttachSlotType type) const {
        switch (type) {
            case AttachSlotType::HAND_RIGHT: return "hand_right";
            case AttachSlotType::HAND_LEFT: return "hand_left";
            case AttachSlotType::HEAD: return "head";
            case AttachSlotType::BACK: return "back";
            case AttachSlotType::WEAPON_PRIMARY: return "weapon_primary";
            case AttachSlotType::WEAPON_SECONDARY: return "weapon_secondary";
            case AttachSlotType::CHEST: return "chest";
            case AttachSlotType::MUZZLE: return "muzzle";
            default: return "custom";
        }
    }
    
    AttachSlotType AttachComponent::StringToSlotType(const std::string& name) const {
        if (name == "hand_right") return AttachSlotType::HAND_RIGHT;
        if (name == "hand_left") return AttachSlotType::HAND_LEFT;
        if (name == "head") return AttachSlotType::HEAD;
        if (name == "back") return AttachSlotType::BACK;
        if (name == "weapon_primary") return AttachSlotType::WEAPON_PRIMARY;
        if (name == "weapon_secondary") return AttachSlotType::WEAPON_SECONDARY;
        if (name == "chest") return AttachSlotType::CHEST;
        if (name == "muzzle") return AttachSlotType::MUZZLE;
        return AttachSlotType::CUSTOM;
    }
    
    Object::Object(int _id) : id(_id) {
        originalSize = D3DXVECTOR3(2, 2, 2);
        D3DXMatrixIdentity(&transform.globalMatrix);
        transform.localScale = D3DXVECTOR3(1, 1, 1);
        attachment.SetTransformSystem(nullptr);
        attachment.SetOwnerId(id);
    }

    bool Object::Load(LPDIRECT3DDEVICE9 dev, ResourceManager* res, std::string mod, std::string tex) {
        if (mod.substr(mod.find_last_of(".") + 1) == "anim") {
            if (!mesh.LoadAnimated(dev, mod)) return false;
            isAnimated = true;
            SetAnimation(ANIM_IDLE, 0, 0, 0.0f, true);
        } else {
            std::vector<Vertex> v = LoadVertices(mod);
            if (v.empty()) return false;
            if (!mesh.Create(dev, v, true)) return false;
            isAnimated = false;
        }
        if (!tex.empty()) mesh.SetTexture(res->GetTexture(tex));
        
        D3DXVECTOR3 min, max;
        mesh.GetBounds(min, max);
        originalSize = D3DXVECTOR3(max.x - min.x, max.y - min.y, max.z - min.z);
        
        return true;
    }

    void Object::SetAnimation(AnimState state, int start, int end, float speed, bool loop) {
        if (currentAnimState == state && isAnimated) return;
        currentAnimState = state;
        animStartFrame = start;
        animEndFrame = end;
        animSpeed = speed;
        animLoop = loop;
    }

    void Object::Update(float dt, LPDIRECT3DDEVICE9 dev) {
        if (isAnimated) {
            mesh.UpdateAnimation(dev, dt, animSpeed, animStartFrame, animEndFrame, animLoop);
        }
        effect.Update(mesh, dt);
    }

    void Object::Draw(LPDIRECT3DDEVICE9 dev) {
        mesh.Draw(dev, &transform.globalMatrix);
    }

    void Object::SetColor(float r, float g, float b, float alpha) {
        D3DCOLORVALUE col;
        col.r = r; col.g = g; col.b = b; col.a = alpha;
        mesh.SetMaterial(col, col, 30.0f);
    }

    void Object::GetBounds(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax) {
        mesh.GetBounds(outMin, outMax);
        
        outMin.x *= transform.globalScale.x;
        outMin.y *= transform.globalScale.y;
        outMin.z *= transform.globalScale.z;
        
        outMax.x *= transform.globalScale.x;
        outMax.y *= transform.globalScale.y;
        outMax.z *= transform.globalScale.z;
        
        outMin += transform.globalPosition;
        outMax += transform.globalPosition;
    }

    void Object::AddAttachPoint(std::string name, D3DXVECTOR3 offset, D3DXVECTOR3 rotation) {
        AttachPoint ap;
        ap.name = name;
        ap.offset = offset;
        ap.rotation = rotation;
        attachPoints.push_back(ap);
    }

    D3DXVECTOR3 Object::GetAttachPointWorld(std::string name) {
    	for (size_t i = 0; i < attachPoints.size(); ++i) {
        	if (attachPoints[i].name == name) {
            	D3DXVECTOR3 worldPos;
            	D3DXVec3TransformCoord(&worldPos, &attachPoints[i].offset, &transform.globalMatrix);
            	return worldPos;
        	}
    	}
    	return transform.globalPosition;
	}

    
    ComplexObject::ComplexObject(TransformSystem* ts) 
        : rootPartIndex(-1), nextId(1000), transformSystem(ts), turret(nullptr), base(nullptr) {}

    ComplexObject::ComplexObject(int id, int nextId) 
        : rootPartIndex(-1), nextId(nextId), transformSystem(nullptr), turret(nullptr), base(nullptr) {}

    ComplexObject::~ComplexObject() {
        for (size_t i = 0; i < parts.size(); ++i) {
            delete parts[i].obj;
        }
    }
    
    bool ComplexObject::Load(LPDIRECT3DDEVICE9 dev, ResourceManager* res, 
                              std::string modelAnim, std::string texAnim,
                              std::string modelBase, std::string texBase,
                              D3DXVECTOR3 offset) {
        base = new Object(nextId++);
        if (!base->Load(dev, res, modelBase, texBase)) {
            delete base;
            base = nullptr;
            return false;
        }
        
        turret = new Object(nextId++);
        if (!turret->Load(dev, res, modelAnim, texAnim)) {
            delete turret;
            delete base;
            turret = nullptr;
            base = nullptr;
            return false;
        }
        
        AddPart(base, D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), true, true);
        AddPart(turret, offset, D3DXVECTOR3(0, 0, 0), false, false);
        
        return true;
    }
    
    Object* ComplexObject::AddPart(Object* obj, D3DXVECTOR3 offset, D3DXVECTOR3 rotation, bool hasPhysics, bool isRoot) {
        PartInfo info;
        info.obj = obj;
        info.offset = offset;
        info.rotation = rotation;
        info.physicsId = hasPhysics ? obj->id : -1;
        info.isRoot = isRoot;
        
        obj->transform.SetLocalPosition(offset);
        obj->transform.SetLocalRotation(rotation);
        
        if (transformSystem) {
            transformSystem->Register(obj->id, &obj->transform);
        }
        
        if (rootPartIndex != -1 && !isRoot && transformSystem) {
            transformSystem->SetParent(obj->id, parts[rootPartIndex].obj->id);
        }
        
        parts.push_back(info);
        
        if (isRoot || rootPartIndex == -1) {
            rootPartIndex = (int)parts.size() - 1;
        }
        
        return obj;
    }

    Object* ComplexObject::AddPartFromFile(LPDIRECT3DDEVICE9 dev, ResourceManager* res,
                                            std::string model, std::string tex,
                                            D3DXVECTOR3 offset, D3DXVECTOR3 rotation,
                                            D3DXVECTOR3 desiredSize, bool hasPhysics, bool isRoot) {
        Object* newObj = new Object(nextId++);
        if (newObj->Load(dev, res, model, tex)) {
            D3DXVECTOR3 orig = newObj->GetOriginalSize();
            if (orig.x > 0 && orig.y > 0 && orig.z > 0) {
                newObj->transform.localScale.x = desiredSize.x / orig.x;
                newObj->transform.localScale.y = desiredSize.y / orig.y;
                newObj->transform.localScale.z = desiredSize.z / orig.z;
            }
            AddPart(newObj, offset, rotation, hasPhysics, isRoot);
            return newObj;
        }
        delete newObj;
        return nullptr;
    }

    void ComplexObject::RegisterPhysics(Physics* phys, D3DXVECTOR3 posOffset, D3DXVECTOR3 size, bool isKinematic) {
        if (!phys) return;
        
        for (size_t i = 0; i < parts.size(); ++i) {
            PartInfo& p = parts[i];
            if (p.physicsId != -1) {
                D3DXVECTOR3 worldPos = GetRootPosition() + p.offset + posOffset;
                phys->Add(p.obj->id, worldPos, size, false, COL_OBB, D3DXVECTOR3(0, 0, 0), isKinematic);
                p.physicsId = p.obj->id;
            }
        }
    }

    void ComplexObject::SetPhysicsPosition(Physics* phys, D3DXVECTOR3 pos) {
        SetRootPosition(pos);
        if (!phys) return;
        for (size_t i = 0; i < parts.size(); ++i) {
            PartInfo& p = parts[i];
            if (p.physicsId != -1) {
                phys->SetPosition(p.physicsId, pos + p.offset);
            }
        }
    }

    void ComplexObject::Update(float dt, LPDIRECT3DDEVICE9 dev, Physics* phys, float angle, float pitch, bool isMoving) {
        if (turret) {
            if (isMoving) {
                turret->SetAnimation(Object::ANIM_WALK, 0, 30, 15.0f, true);
            } else {
                turret->SetAnimation(Object::ANIM_IDLE, 0, 0, 0.0f, true);
            }
        }
        
        if (base) {
            D3DXVECTOR3 rot = base->transform.localRotation;
            rot.y = angle;
            base->transform.SetLocalRotation(rot);
        }
        
        if (turret) {
            D3DXVECTOR3 rot = turret->transform.localRotation;
            rot.x = pitch;
            turret->transform.SetLocalRotation(rot);
        }
        
        if (rootPartIndex != -1 && phys) {
            PartInfo& root = parts[rootPartIndex];
            if (root.physicsId != -1) {
                D3DXVECTOR3 physPos = phys->GetPosition(root.physicsId);
                root.obj->transform.SetLocalPosition(physPos);
            }
        }
        
        if (transformSystem) {
            transformSystem->Update();
        }
        
        for (size_t i = 0; i < parts.size(); ++i) {
            parts[i].obj->Update(dt, dev);
        }
    }

    void ComplexObject::SetRootPosition(D3DXVECTOR3 pos) {
        if (rootPartIndex != -1) {
            parts[rootPartIndex].obj->transform.SetLocalPosition(pos);
            if (transformSystem) transformSystem->Update();
        }
    }

    void ComplexObject::SetRootRotation(D3DXVECTOR3 rot) {
        if (rootPartIndex != -1) {
            parts[rootPartIndex].obj->transform.SetLocalRotation(rot);
            if (transformSystem) transformSystem->Update();
        }
    }
    
    D3DXVECTOR3 ComplexObject::GetRootPosition() {
        if (rootPartIndex != -1) return parts[rootPartIndex].obj->transform.globalPosition;
        return D3DXVECTOR3(0, 0, 0);
    }

    Object* ComplexObject::GetPart(int index) {
        if (index >= 0 && index < (int)parts.size()) return parts[index].obj;
        return nullptr;
    }

    D3DXVECTOR3 ComplexObject::GetAttachPointWorld(std::string partName, std::string attachName) {
        for (size_t i = 0; i < parts.size(); ++i) {
            if (parts[i].obj->name == partName || std::to_string(parts[i].obj->id) == partName) {
                return parts[i].obj->GetAttachPointWorld(attachName);
            }
        }
        return GetRootPosition();
    }

    void ComplexObject::Fire(ParticleSystem& partSys, float angle, float pitch, DWORD bulletColor) {
        if (!turret) return;
        
        D3DXVECTOR3 spawnPos = turret->GetAttachPointWorld("muzzle");
        
        if (spawnPos == turret->transform.globalPosition) {
            spawnPos = turret->transform.globalPosition + D3DXVECTOR3(0.5f, 0.3f, 0.8f);
        }
        
        D3DXVECTOR3 direction;
        direction.x = sinf(angle) * cosf(pitch);
        direction.y = sinf(pitch);
        direction.z = cosf(angle) * cosf(pitch);
        D3DXVec3Normalize(&direction, &direction);
        
        D3DXVECTOR3 vel = direction * 0.4f;
        partSys.Add(spawnPos, vel, bulletColor, 2.0f, 0.08f);
    }
    
    Scene::Scene(Physics* physicsModule) : physRef(physicsModule) {}
    
    Scene::~Scene() { Clear(); }

    void Scene::AddObject(Object* obj) { 
        objects.push_back(obj);
        transformSystem.Register(obj->id, &obj->transform);
        obj->attachment.SetTransformSystem(&transformSystem);
        obj->attachment.SetOwnerId(obj->id);
    }

    void Scene::AddComplexObject(ComplexObject* complexObj) {
        complexObjects.push_back(complexObj);
        for (int i = 0; i < complexObj->GetPartCount(); ++i) {
            Object* obj = complexObj->GetPart(i);
            objects.push_back(obj);
            transformSystem.Register(obj->id, &obj->transform);
            obj->attachment.SetTransformSystem(&transformSystem);
            obj->attachment.SetOwnerId(obj->id);
        }
    }

    Object* Scene::CreateObject(int objId, LPDIRECT3DDEVICE9 dev, ResourceManager* res,
                                std::string model, std::string tex,
                                D3DXVECTOR3 position, D3DXVECTOR3 desiredSize,
                                bool isStatic, CollisionType cType, D3DXVECTOR3 rotation) {
        Object* obj = new Object(objId);
        if (obj->Load(dev, res, model, tex)) {
            D3DXVECTOR3 orig = obj->GetOriginalSize();
            if (orig.x > 0 && orig.y > 0 && orig.z > 0) {
                obj->transform.localScale.x = desiredSize.x / orig.x;
                obj->transform.localScale.y = desiredSize.y / orig.y;
                obj->transform.localScale.z = desiredSize.z / orig.z;
            }
            obj->transform.localPosition = position;
            obj->transform.localRotation = rotation;
            
            if (physRef && desiredSize.x > 0) {
                physRef->Add(objId, position, desiredSize, isStatic, cType, rotation);
            }
            objects.push_back(obj);
            transformSystem.Register(objId, &obj->transform);
            obj->attachment.SetTransformSystem(&transformSystem);
            obj->attachment.SetOwnerId(objId);
            return obj;
        }
        delete obj;
        return nullptr;
    }

    void Scene::Update(float dt, LPDIRECT3DDEVICE9 dev) {
    transformSystem.Update();
    
    for (size_t i = 0; i < objects.size(); ++i) {
        Object* obj = objects[i];
        if (!obj) continue;
        
        obj->attachment.Update(obj, dt);
        
        if (physRef && obj->transform.parentId == -1) {
            const std::vector<Body>& bodies = physRef->GetBodies();
            for (size_t j = 0; j < bodies.size(); ++j) {
                if (bodies[j].id == obj->id) {
                    D3DXVECTOR3 p = physRef->GetPosition(obj->id);
                    obj->transform.SetLocalPosition(p);
                    break;
                }
            }
        }
        obj->Update(dt, dev);
    }
    
    for (size_t i = 0; i < complexObjects.size(); ++i) {
        complexObjects[i]->Update(dt, dev, physRef, 0, 0, false);
    }
}

    void Scene::DrawAll(LPDIRECT3DDEVICE9 dev, Frustum& frustum) {
        for (size_t i = 0; i < objects.size(); ++i) {
            Object* obj = objects[i];
            if (!obj) continue;
            D3DXVECTOR3 min, max;
            obj->GetBounds(min, max);
            if (!frustum.CheckBox(min, max)) continue;
            obj->Draw(dev);
        }
    }
    
    Object* Scene::GetObjectByID(int id) {
        for (size_t i = 0; i < objects.size(); ++i) {
            if (objects[i]->id == id) return objects[i];
        }
        return nullptr;
    }

    void Scene::Clear() {
        for (size_t i = 0; i < objects.size(); ++i) delete objects[i];
        objects.clear();
        for (size_t i = 0; i < complexObjects.size(); ++i) delete complexObjects[i];
        complexObjects.clear();
    }
}
