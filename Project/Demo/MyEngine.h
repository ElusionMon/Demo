#ifndef MYENGINE_H
#define MYENGINE_H

#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <map>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <queue>

namespace MyEngine {

    class Object;
    class TransformSystem;
    class Physics;
    class ParticleSystem;
    class Mesh;
    class ResourceManager;
    struct Vertex;

    struct Vertex {
        float x, y, z;
        float nx, ny, nz;
        DWORD color;
        float tu, tv;
        static const DWORD FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
    };

    class Renderer {
        LPDIRECT3D9 d3d = nullptr;
        LPDIRECT3DDEVICE9 device = nullptr;
        D3DPRESENT_PARAMETERS d3dpp;
    public:
        ~Renderer();
        bool Init(HWND hWnd);
        void Begin();
        void End();
        void HandleDeviceLost(HWND hWnd);
        LPDIRECT3DDEVICE9 GetDevice() { return device; }
        void SetupFog(bool enable, DWORD color, float start, float end);
    };

    struct AnimKeyframe {
        std::vector<D3DXVECTOR3> positions;
        std::vector<D3DXVECTOR3> normals;
    };

    class Mesh {
        LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
        int vCount = 0;
        D3DMATERIAL9 material;
        std::shared_ptr<IDirect3DTexture9> texture;
        std::vector<Vertex> baseVertices;
        std::vector<AnimKeyframe> keyframes;
        float currentFrame = 0.0f;
        D3DXVECTOR3 boundsMin, boundsMax;
        bool hasBounds = false;
    public:
        ~Mesh();
        bool Create(LPDIRECT3DDEVICE9 dev, const std::vector<Vertex>& verts, bool dyn = false);
        bool LoadAnimated(LPDIRECT3DDEVICE9 dev, std::string path);
        void UpdateVertices(const std::vector<Vertex>& verts);
        void AnimateWave(float time, float speed, float amplitude);
        void UpdateAnimation(LPDIRECT3DDEVICE9 dev, float dt, float speed, int startFrame, int endFrame, bool loop = true);
        void SetTexture(std::shared_ptr<IDirect3DTexture9> tex) { texture = tex; }
        void SetMaterial(D3DCOLORVALUE d, D3DCOLORVALUE s, float p);
        void Draw(LPDIRECT3DDEVICE9 dev, D3DXMATRIX* world);
        void GetBounds(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax);
        int GetVertexCount() { return vCount; }
    private:
        void UpdateBounds();
    };

    class TransformComponent {
    public:
        D3DXVECTOR3 localPosition = D3DXVECTOR3(0, 0, 0);
        D3DXVECTOR3 localRotation = D3DXVECTOR3(0, 0, 0);
        D3DXVECTOR3 localScale = D3DXVECTOR3(1, 1, 1);
        
        D3DXVECTOR3 globalPosition = D3DXVECTOR3(0, 0, 0);
        D3DXVECTOR3 globalRotation = D3DXVECTOR3(0, 0, 0);
        D3DXVECTOR3 globalScale = D3DXVECTOR3(1, 1, 1);
        D3DXMATRIX globalMatrix;
        
        int parentId = -1;
        std::vector<int> childIds;
        bool isDirty = true;
        
        TransformComponent();
        void SetLocalPosition(const D3DXVECTOR3& pos);
        void SetLocalRotation(const D3DXVECTOR3& rot);
        void SetLocalScale(const D3DXVECTOR3& scl);
        D3DXVECTOR3 GetGlobalPosition() const { return globalPosition; }
        void MarkDirty();
    };

    class TransformSystem {
        std::map<int, TransformComponent*> transforms;
    public:
        void Register(int id, TransformComponent* t);
        void Unregister(int id);
        void SetParent(int childId, int parentId);
        void Update();
        TransformComponent* GetTransform(int id);
    private:
        void UpdateTransform(TransformComponent& current);
        void MarkChildrenDirty(TransformComponent& current);
    };

    class ResourceManager {
        LPDIRECT3DDEVICE9 dev;
        std::map<std::string, std::shared_ptr<IDirect3DTexture9>> cache;
        std::map<std::string, int> failedAttempts;
        const int MAX_FAILED_ATTEMPTS = 3;
    public:
        ResourceManager(LPDIRECT3DDEVICE9 d) : dev(d) {}
        std::shared_ptr<IDirect3DTexture9> GetTexture(std::string path);
        void ClearUnused();
        int GetCacheSize() { return cache.size(); }
    };

    struct AttachPoint {
        std::string name;
        D3DXVECTOR3 offset;
        D3DXVECTOR3 rotation;
        D3DXVECTOR3 GetWorldPosition(const D3DXVECTOR3& parentPos, const D3DXMATRIX& parentRot);
    };

    enum class AttachSlotType {
        CUSTOM = 0,
        HAND_RIGHT,
        HAND_LEFT,
        HEAD,
        BACK,
        WEAPON_PRIMARY,
        WEAPON_SECONDARY,
        CHEST,
        MUZZLE
    };

    struct AttachmentInfo {
        int childId;
        int parentId;
        AttachSlotType slotType;
        std::string slotName;
        D3DXVECTOR3 localOffset;
        D3DXVECTOR3 localRotation;
        bool isAttached;
        float scale;
        
        AttachmentInfo() : childId(-1), parentId(-1), slotType(AttachSlotType::CUSTOM),
                           localOffset(0,0,0), localRotation(0,0,0), isAttached(false), scale(1.0f) {}
    };

    class AttachComponent {
    private:
        std::map<std::string, AttachPoint> slots;
        std::vector<AttachmentInfo> attachedChildren;
        TransformSystem* transformSystem;
        int ownerId;
        
    public:
        AttachComponent();
        ~AttachComponent();
        void SetTransformSystem(TransformSystem* ts) { transformSystem = ts; }
        void SetOwnerId(int id) { ownerId = id; }
        int GetOwnerId() const { return ownerId; }
        
        void AddSlot(const std::string& name, const D3DXVECTOR3& offset, const D3DXVECTOR3& rotation = D3DXVECTOR3(0,0,0));
        void AddSlot(AttachSlotType type, const D3DXVECTOR3& offset, const D3DXVECTOR3& rotation = D3DXVECTOR3(0,0,0));
        bool HasSlot(const std::string& name) const;
        const AttachPoint* GetSlot(const std::string& name) const;
        
        bool Attach(int childId, const std::string& slotName, const D3DXVECTOR3& offset = D3DXVECTOR3(0,0,0), float scale = 1.0f);
        bool Attach(int childId, AttachSlotType slotType, const D3DXVECTOR3& offset = D3DXVECTOR3(0,0,0), float scale = 1.0f);
        void Detach(int childId);
        void DetachFromSlot(const std::string& slotName);
        void DetachAll();
        
        bool IsAttached(int childId) const;
        bool IsAttached(Object* child) const;
        
        D3DXVECTOR3 GetSlotWorldPosition(Object* owner, const std::string& slotName);
        void Update(Object* owner, float dt);
        
    private:
        std::string SlotTypeToString(AttachSlotType type) const;
        AttachSlotType StringToSlotType(const std::string& name) const;
    };

    class Camera {
        D3DXVECTOR3 pos, target;
        D3DXMATRIX matView, matProj;
        float aspect;
        float yaw = 0.0f, pitch = 0.0f;
        const float MAX_PITCH = D3DX_PI / 2.0f - 0.01f;
    public:
        Camera(float asp) : aspect(asp), pos(0,5,-10), target(0,0,0) {}
        void Update(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 followPos, float angle, float dist, float height);
        void Rotate(float deltaYaw, float deltaPitch);
        D3DXVECTOR3 GetPos() { return pos; }
        D3DXMATRIX GetViewMatrix() { return matView; }
        D3DXMATRIX GetProjMatrix() { return matProj; }
    };

    class Input {
        bool keys[256];
        bool prevKeys[256];
        POINT mousePos;
        HWND attachedHWnd;
    public:
        void Init(HWND hWnd);
        void Update();
        bool IsKeyDown(int v) { return keys[v]; }
        bool IsKeyPressed(int v) { return keys[v] && !prevKeys[v]; }
        POINT GetMousePos() { return mousePos; }
    };

    class Text {
        LPD3DXFONT font = nullptr;
    public:
        ~Text() { if(font) font->Release(); }
        bool Init(LPDIRECT3DDEVICE9 dev, int size, std::string face);
        void Draw(std::string str, int x, int y, DWORD color);
    };

    class SpriteRenderer {
        LPD3DXSPRITE spriteAPI = nullptr;
    public:
        ~SpriteRenderer() { if(spriteAPI) spriteAPI->Release(); }
        bool Init(LPDIRECT3DDEVICE9 dev);
        void Begin();
        void End();
        void Draw(std::shared_ptr<IDirect3DTexture9> tex, int x, int y, int w, int h, DWORD color = 0xFFFFFFFF);
    };

    class Button {
        int x, y, width, height;
        bool isHovered = false;
        std::function<void()> onClickCallback;
    public:
        Button(int x, int y, int w, int h, std::function<void()> callback);
        void Update(Input& input);
        void Draw(SpriteRenderer& sprite, Text& text, std::shared_ptr<IDirect3DTexture9> texNormal, std::shared_ptr<IDirect3DTexture9> texHover, std::string label, DWORD textColor);
    };

    class Light {
        D3DLIGHT9 lightData;
        int index;
        bool enabled;
    public:
        Light(int lightIndex = 0);
        void SetupDirectional(D3DXVECTOR3 direction, D3DCOLORVALUE color);
        void Enable(LPDIRECT3DDEVICE9 dev, bool state);
        void SetDirection(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 direction);
    };

    class Effect {
        float timer = 0, intensity = 0;
        bool active = false;
        D3DXVECTOR3 shakeOffset;
    public:
        enum Type { NONE, BLINK, SHAKE };
        void Play(Type t, float i);
        void Update(Mesh& m, float dt);
        D3DXVECTOR3 GetShakeOffset() { return shakeOffset; }
    };

    struct Particle {
        D3DXVECTOR3 pos = {0,0,0};
        D3DXVECTOR3 vel = {0,0,0};
        float life = 0, maxLife = 0;
        DWORD color = 0xFFFFFFFF;
        float size = 0.05f;
    };
    
    class ParticleSystem {
        std::vector<Particle> particles;
        LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
        int maxParticles;
        D3DXVECTOR3 cameraPos;
    public:
        ParticleSystem(int count);
        ~ParticleSystem();
        bool Init(LPDIRECT3DDEVICE9 dev);
        void Add(D3DXVECTOR3 p, D3DXVECTOR3 v, DWORD c, float l, float size = 0.05f);
        void Update(float dt);
        void Draw(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 camPos);
        void Clear();
        int GetCount() { return particles.size(); }
    };

    enum CollisionType { COL_AABB, COL_OBB, COL_SPHERE, COL_CAPSULE };

    struct Body {
        int id;
        D3DXVECTOR3 pos;
        D3DXVECTOR3 size;
        D3DXVECTOR3 velocity;
        bool isStatic;
        bool isKinematic;
        bool isGrounded;
        CollisionType colType;
        D3DXMATRIX matRotation;
        float radius;
        float height;
    };

    class Physics {
        std::vector<Body> bodies;
        const float GRAVITY = -0.005f;
    public:
        void Add(int id, D3DXVECTOR3 p, D3DXVECTOR3 s, bool isStatic, CollisionType cType = COL_AABB, 
                 D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0), bool isKinematic = false);
        void Update(float dt);
        void SetVelocity(int id, float vx, float vz);
        void ApplyImpulse(int id, D3DXVECTOR3 force);
        void Jump(int id, float force);
        D3DXVECTOR3 GetPosition(int id);
        void SetPosition(int id, D3DXVECTOR3 pos);
        bool GetBounds(int id, D3DXVECTOR3& minPt, D3DXVECTOR3& maxPt);
        void SetRotation(int id, D3DXVECTOR3 rotation);
        int GetBodyCount() { return bodies.size(); }
        const std::vector<Body>& GetBodies() const { return bodies; }
    private:
        void ResolveCollision(Body& a, Body& b);
        bool CheckOBBvsOBB(const Body& a, const Body& b, D3DXVECTOR3& outOverlap, D3DXVECTOR3& outAxis);
    };

    class Frustum {
        D3DXPLANE planes[6];
    public:
        void Construct(const D3DXMATRIX& view, const D3DXMATRIX& proj);
        bool CheckBox(const D3DXVECTOR3& minPt, const D3DXVECTOR3& maxPt);
        bool CheckSphere(const D3DXVECTOR3& center, float radius);
    };

    class DebugDraw {
        LPDIRECT3DDEVICE9 dev;
        bool initialized;
    public:
        DebugDraw();
        ~DebugDraw();
        void Init(LPDIRECT3DDEVICE9 device);
        void DrawAABB(const D3DXVECTOR3& min, const D3DXVECTOR3& max, DWORD color = 0xFFFF0000);
        void DrawOBB(const D3DXVECTOR3& center, const D3DXVECTOR3& halfSize, const D3DXMATRIX& rot, DWORD color = 0xFF00FF00);
        void DrawSphere(const D3DXVECTOR3& center, float radius, DWORD color = 0xFFFF00FF);
        void DrawCapsule(const D3DXVECTOR3& center, float radius, float height, const D3DXMATRIX& rot, DWORD color = 0xFFFFAA00);
        void Shutdown();
    };

    class Object {
    public:
        enum AnimState { ANIM_IDLE, ANIM_WALK, ANIM_JUMP, ANIM_ATTACK, ANIM_CUSTOM };
        
        Mesh mesh;
        Effect effect;
        TransformComponent transform;
        AttachComponent attachment;
        
        int id;
        std::string name;
        std::vector<AttachPoint> attachPoints;
        
    private:
        AnimState currentAnimState = ANIM_IDLE;
        int animStartFrame = 0;
        int animEndFrame = 0;
        float animSpeed = 0.0f;
        bool animLoop = true;
        bool isAnimated = false;
        D3DXVECTOR3 originalSize;
        
    public:
        Object(int _id);
        bool Load(LPDIRECT3DDEVICE9 dev, ResourceManager* res, std::string mod, std::string tex);
        void Update(float dt, LPDIRECT3DDEVICE9 dev);
        void Draw(LPDIRECT3DDEVICE9 dev);
        void SetColor(float r, float g, float b, float alpha = 1.0f);
        void SetAnimation(AnimState state, int start, int end, float speed, bool loop = true);
        AnimState GetAnimationState() { return currentAnimState; }
        void GetBounds(D3DXVECTOR3& outMin, D3DXVECTOR3& outMax);
        D3DXVECTOR3 GetOriginalSize() { return originalSize; }
        void AddAttachPoint(std::string name, D3DXVECTOR3 offset, D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));
        D3DXVECTOR3 GetAttachPointWorld(std::string name);
        TransformComponent* GetTransform() { return &transform; }
    };

    struct PartInfo {
        Object* obj;
        D3DXVECTOR3 offset;
        D3DXVECTOR3 rotation;
        int physicsId;
        bool isRoot;
    };

    class ComplexObject {
        std::vector<PartInfo> parts;
        int rootPartIndex;
        int nextId;
        TransformSystem* transformSystem;
        Object* turret;
        Object* base;
    public:
        ComplexObject(TransformSystem* ts);
        ComplexObject(int id, int nextId);
        ~ComplexObject();
        
        bool Load(LPDIRECT3DDEVICE9 dev, ResourceManager* res, 
                  std::string modelAnim, std::string texAnim,
                  std::string modelBase, std::string texBase,
                  D3DXVECTOR3 offset);
        
        Object* AddPart(Object* obj, D3DXVECTOR3 offset, D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0), bool hasPhysics = true, bool isRoot = false);
        Object* AddPartFromFile(LPDIRECT3DDEVICE9 dev, ResourceManager* res, 
                                 std::string model, std::string tex,
                                 D3DXVECTOR3 offset, D3DXVECTOR3 rotation,
                                 D3DXVECTOR3 desiredSize, bool hasPhysics = true, bool isRoot = false);
        
        void RegisterPhysics(Physics* phys, D3DXVECTOR3 posOffset, D3DXVECTOR3 size, bool isKinematic);
        void SetPhysicsPosition(Physics* phys, D3DXVECTOR3 pos);
        void Update(float dt, LPDIRECT3DDEVICE9 dev, Physics* phys, float angle = 0, float pitch = 0, bool isMoving = false);
        
        void SetRootPosition(D3DXVECTOR3 pos);
        void SetRootRotation(D3DXVECTOR3 rot);
        D3DXVECTOR3 GetRootPosition();
        
        Object* GetPart(int index);
        Object* GetRoot() { return parts[rootPartIndex].obj; }
        Object* GetTurret() { return turret; }
        Object* GetBase() { return base; }
        
        D3DXVECTOR3 GetAttachPointWorld(std::string partName, std::string attachName);
        void Fire(ParticleSystem& partSys, float angle, float pitch, DWORD bulletColor);
        
        int GetPartCount() { return (int)parts.size(); }
    };

    class Scene {
        std::vector<Object*> objects;
        std::vector<ComplexObject*> complexObjects;
        Physics* physRef;
        TransformSystem transformSystem;
    public:
        Scene(Physics* physicsModule);
        ~Scene();
        void AddObject(Object* obj);
        void AddComplexObject(ComplexObject* complexObj);
        Object* CreateObject(int id, LPDIRECT3DDEVICE9 dev, ResourceManager* res, 
                             std::string model, std::string tex, 
                             D3DXVECTOR3 pos, D3DXVECTOR3 desiredSize,
                             bool isStatic, CollisionType cType = COL_OBB,
                             D3DXVECTOR3 rotation = D3DXVECTOR3(0,0,0));
        void Update(float dt, LPDIRECT3DDEVICE9 dev);
        void DrawAll(LPDIRECT3DDEVICE9 dev, Frustum& frustum);
        Object* GetObjectByID(int id);
        TransformSystem* GetTransformSystem() { return &transformSystem; }
        void Clear();
    };

    class Skybox {
        LPDIRECT3DVERTEXBUFFER9 vb = nullptr;
        LPDIRECT3DINDEXBUFFER9  ib = nullptr;
        std::shared_ptr<IDirect3DTexture9> texture;
        int numVertices = 0, numTriangles = 0;
        float rotation = 0.0f;
    public:
        ~Skybox();
        bool Create(LPDIRECT3DDEVICE9 dev, std::shared_ptr<IDirect3DTexture9> tex);
        void Update(float dt, float speed = 0.01f);
        void Draw(LPDIRECT3DDEVICE9 dev, D3DXVECTOR3 camPos);
    };

    class AudioSystem {
    	std::map<std::string, std::string> soundAliases;
    	int aliasCounter = 0;
    	std::string GenerateAlias();
	public:
    	~AudioSystem();
    	bool Load(std::string path, std::string soundName);
    	void Play(std::string soundName, bool loop = false);
    	void Stop(std::string soundName);
    	void SetVolume(std::string soundName, int volume);
	};

    std::vector<Vertex> LoadVertices(std::string path);
}
#endif
