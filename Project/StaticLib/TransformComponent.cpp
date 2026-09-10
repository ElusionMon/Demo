#include "MyEngine.h"
#include <cmath>

namespace MyEngine {
    
    TransformComponent::TransformComponent() {
        D3DXMatrixIdentity(&globalMatrix);
        localPosition = D3DXVECTOR3(0, 0, 0);
        localRotation = D3DXVECTOR3(0, 0, 0);
        localScale = D3DXVECTOR3(1, 1, 1);
        globalPosition = D3DXVECTOR3(0, 0, 0);
        globalRotation = D3DXVECTOR3(0, 0, 0);
        globalScale = D3DXVECTOR3(1, 1, 1);
        parentId = -1;
        isDirty = true;
    }
    
    void TransformComponent::SetLocalPosition(const D3DXVECTOR3& pos) {
        localPosition = pos;
        isDirty = true;
    }
    
    void TransformComponent::SetLocalRotation(const D3DXVECTOR3& rot) {
        localRotation = rot;
        isDirty = true;
    }
    
    void TransformComponent::SetLocalScale(const D3DXVECTOR3& scl) {
        localScale = scl;
        isDirty = true;
    }
    
    void TransformComponent::MarkDirty() {
        isDirty = true;
    }
    
    void TransformSystem::Register(int id, TransformComponent* t) {
        transforms[id] = t;
    }
    
    void TransformSystem::Unregister(int id) {
        transforms.erase(id);
    }
    
    void TransformSystem::SetParent(int childId, int parentId) {
        if (transforms.count(childId) && transforms.count(parentId)) {
            TransformComponent* child = transforms[childId];
            if (child->parentId != -1 && transforms.count(child->parentId)) {
                TransformComponent* oldParent = transforms[child->parentId];
                oldParent->childIds.erase(
                    std::remove(oldParent->childIds.begin(), oldParent->childIds.end(), childId),
                    oldParent->childIds.end()
                );
            }
            
            child->parentId = parentId;
            transforms[parentId]->childIds.push_back(childId);
            child->isDirty = true;
            MarkChildrenDirty(*child);
        } else if (parentId == -1 && transforms.count(childId)) {
            TransformComponent* child = transforms[childId];
            if (child->parentId != -1 && transforms.count(child->parentId)) {
                TransformComponent* oldParent = transforms[child->parentId];
                oldParent->childIds.erase(
                    std::remove(oldParent->childIds.begin(), oldParent->childIds.end(), childId),
                    oldParent->childIds.end()
                );
            }
            child->parentId = -1;
            child->isDirty = true;
            MarkChildrenDirty(*child);
        }
    }
    
    void TransformSystem::MarkChildrenDirty(TransformComponent& current) {
        for (int childId : current.childIds) {
            if (transforms.count(childId)) {
                transforms[childId]->isDirty = true;
                MarkChildrenDirty(*transforms[childId]);
            }
        }
    }
    
    void TransformSystem::UpdateTransform(TransformComponent& current) {
    if (!current.isDirty) return;
    
    D3DXMATRIX localMat, rotMat, scaleMat, transMat;
    D3DXMATRIX rotX, rotY, rotZ;
    
    D3DXMatrixScaling(&scaleMat, current.localScale.x, current.localScale.y, current.localScale.z);
    
    D3DXMatrixRotationX(&rotX, current.localRotation.x);
    D3DXMatrixRotationY(&rotY, current.localRotation.y);
    D3DXMatrixRotationZ(&rotZ, current.localRotation.z);
    rotMat = rotX * rotY * rotZ;
    
    D3DXMatrixTranslation(&transMat, current.localPosition.x, current.localPosition.y, current.localPosition.z);
    
    localMat = scaleMat * rotMat * transMat;
    
    if (current.parentId != -1 && transforms.count(current.parentId)) {
        TransformComponent* parent = transforms[current.parentId];
        if (parent->isDirty) {
            UpdateTransform(*parent);
        }
        current.globalMatrix = localMat * parent->globalMatrix;
    } else {
        current.globalMatrix = localMat;
    }
    
    current.globalPosition.x = current.globalMatrix._41;
    current.globalPosition.y = current.globalMatrix._42;
    current.globalPosition.z = current.globalMatrix._43;
    
    current.globalScale.x = sqrtf(current.globalMatrix._11 * current.globalMatrix._11 + 
                                    current.globalMatrix._12 * current.globalMatrix._12 + 
                                    current.globalMatrix._13 * current.globalMatrix._13);
    current.globalScale.y = sqrtf(current.globalMatrix._21 * current.globalMatrix._21 + 
                                    current.globalMatrix._22 * current.globalMatrix._22 + 
                                    current.globalMatrix._23 * current.globalMatrix._23);
    current.globalScale.z = sqrtf(current.globalMatrix._31 * current.globalMatrix._31 + 
                                    current.globalMatrix._32 * current.globalMatrix._32 + 
                                    current.globalMatrix._33 * current.globalMatrix._33);
    
    current.isDirty = false;
    
    for (int childId : current.childIds) {
        if (transforms.count(childId)) {
            transforms[childId]->isDirty = true;
            UpdateTransform(*transforms[childId]);
        }
    }
}
    
    void TransformSystem::Update() {
        for (auto& pair : transforms) {
            if (pair.second->parentId == -1) {
                UpdateTransform(*pair.second);
            }
        }
    }
    
    TransformComponent* TransformSystem::GetTransform(int id) {
        if (transforms.count(id)) return transforms[id];
        return nullptr;
    }
}
