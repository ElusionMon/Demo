#include "MyEngine.h"
#include <cstdio>

namespace MyEngine {

    void Physics::Add(int id, D3DXVECTOR3 p, D3DXVECTOR3 s, bool isStatic, CollisionType cType, D3DXVECTOR3 rotation, bool isKinematic) {
        Body b;
        b.id = id;
        b.pos = p;
        b.size = s;
        b.velocity = D3DXVECTOR3(0, 0, 0);
        b.isStatic = isStatic;
        b.isKinematic = isKinematic;
        b.isGrounded = false;
        b.colType = cType;

        D3DXMATRIX rotX, rotY, rotZ;
        D3DXMatrixRotationX(&rotX, rotation.x);
        D3DXMatrixRotationY(&rotY, rotation.y);
        D3DXMatrixRotationZ(&rotZ, rotation.z);
        b.matRotation = rotX * rotY * rotZ;

        char buf[256];
        sprintf(buf, "ADD id=%d, pos=(%.2f,%.2f,%.2f), size=(%.2f,%.2f,%.2f), rot=(%.2f,%.2f,%.2f), type=%d\n",
                id, p.x, p.y, p.z, s.x, s.y, s.z, rotation.x, rotation.y, rotation.z, cType);
        OutputDebugStringA(buf);

        bodies.push_back(b);
    }

    void Physics::SetRotation(int id, D3DXVECTOR3 rotation) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) {
                D3DXMATRIX rotX, rotY, rotZ;
                D3DXMatrixRotationX(&rotX, rotation.x);
                D3DXMatrixRotationY(&rotY, rotation.y);
                D3DXMatrixRotationZ(&rotZ, rotation.z);
                bodies[i].matRotation = rotX * rotY * rotZ;

                char buf[256];
                sprintf(buf, "SETROT id=%d, rot=(%.2f,%.2f,%.2f)\n", id, rotation.x, rotation.y, rotation.z);
                OutputDebugStringA(buf);
                break;
            }
        }
    }

    D3DXVECTOR3 Physics::GetPosition(int id) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) return bodies[i].pos;
        }
        return D3DXVECTOR3(0, 0, 0);
    }

    void Physics::SetPosition(int id, D3DXVECTOR3 pos) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) {
                bodies[i].pos = pos;
                break;
            }
        }
    }

    bool Physics::GetBounds(int id, D3DXVECTOR3& minPt, D3DXVECTOR3& maxPt) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) {
                D3DXVECTOR3 half = bodies[i].size * 0.5f;
                minPt = bodies[i].pos - half;
                maxPt = bodies[i].pos + half;
                return true;
            }
        }
        return false;
    }

    void Physics::SetVelocity(int id, float vx, float vz) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) {
                bodies[i].velocity.x = vx;
                bodies[i].velocity.z = vz;
            }
        }
    }

    void Physics::ApplyImpulse(int id, D3DXVECTOR3 f) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id) bodies[i].velocity += f;
        }
    }

    void Physics::Jump(int id, float force) {
        for (size_t i = 0; i < bodies.size(); i++) {
            if (bodies[i].id == id && !bodies[i].isStatic && bodies[i].isGrounded) {
                bodies[i].velocity.y = force;
                bodies[i].isGrounded = false;
            }
        }
    }

    void Physics::Update(float dt) {
        for (size_t i = 0; i < bodies.size(); i++) {
            Body& b = bodies[i];
            if (!b.isStatic && !b.isKinematic) {
                b.velocity.y += GRAVITY;
                b.pos += b.velocity;
                b.velocity.x *= 0.85f;
                b.velocity.z *= 0.85f;
                b.isGrounded = false;

                for (size_t j = 0; j < bodies.size(); j++) {
                    if (i != j) {
                        ResolveCollision(b, bodies[j]);
                    }
                }
            }
        }
    }

    bool Physics::CheckOBBvsOBB(const Body& a, const Body& b, D3DXVECTOR3& outOverlap, D3DXVECTOR3& outAxis) {
        D3DXVECTOR3 axesA[3] = {
            D3DXVECTOR3(a.matRotation._11, a.matRotation._12, a.matRotation._13),
            D3DXVECTOR3(a.matRotation._21, a.matRotation._22, a.matRotation._23),
            D3DXVECTOR3(a.matRotation._31, a.matRotation._32, a.matRotation._33)
        };
        D3DXVECTOR3 axesB[3] = {
            D3DXVECTOR3(b.matRotation._11, b.matRotation._12, b.matRotation._13),
            D3DXVECTOR3(b.matRotation._21, b.matRotation._22, b.matRotation._23),
            D3DXVECTOR3(b.matRotation._31, b.matRotation._32, b.matRotation._33)
        };

        D3DXVECTOR3 allAxes[15];
        int axisCount = 0;
        for (int i = 0; i < 3; i++) allAxes[axisCount++] = axesA[i];
        for (int i = 0; i < 3; i++) allAxes[axisCount++] = axesB[i];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                D3DXVECTOR3 cross;
                D3DXVec3Cross(&cross, &axesA[i], &axesB[j]);
                if (D3DXVec3LengthSq(&cross) > 0.001f) {
                    D3DXVec3Normalize(&cross, &cross);
                    allAxes[axisCount++] = cross;
                }
            }
        }

        D3DXVECTOR3 delta = b.pos - a.pos;
        float minOverlap = 999999.0f;
        D3DXVECTOR3 bestAxis(0, 0, 0);

        for (int i = 0; i < axisCount; i++) {
            D3DXVECTOR3 axis = allAxes[i];
            if (D3DXVec3LengthSq(&axis) < 0.001f) continue;
            D3DXVec3Normalize(&axis, &axis);

            float rA = fabsf(D3DXVec3Dot(&axesA[0], &axis)) * a.size.x * 0.5f +
                       fabsf(D3DXVec3Dot(&axesA[1], &axis)) * a.size.y * 0.5f +
                       fabsf(D3DXVec3Dot(&axesA[2], &axis)) * a.size.z * 0.5f;

            float rB = fabsf(D3DXVec3Dot(&axesB[0], &axis)) * b.size.x * 0.5f +
                       fabsf(D3DXVec3Dot(&axesB[1], &axis)) * b.size.y * 0.5f +
                       fabsf(D3DXVec3Dot(&axesB[2], &axis)) * b.size.z * 0.5f;

            float dist = fabsf(D3DXVec3Dot(&delta, &axis));

            if (dist >= rA + rB) return false;

            float overlap = (rA + rB) - dist;
            if (overlap < minOverlap) {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        if (D3DXVec3Dot(&delta, &bestAxis) < 0) bestAxis = -bestAxis;
        outOverlap = bestAxis * minOverlap;
        outAxis = bestAxis;
        return true;
    }

    void Physics::ResolveCollision(Body& dyn, const Body& st) {
        if (dyn.id == st.id) return;

        if (dyn.colType == COL_OBB && st.colType == COL_OBB) {
            D3DXVECTOR3 overlap, axis;
            if (CheckOBBvsOBB(st, dyn, overlap, axis)) {
                dyn.pos += overlap;
                if (axis.y > 0.7f) {
                    dyn.velocity.y = 0.0f;
                    dyn.isGrounded = true;
                } else if (axis.y < -0.7f) {
                    if (dyn.velocity.y > 0) dyn.velocity.y = 0.0f;
                } else {
                    dyn.velocity.x = 0.0f;
                    dyn.velocity.z = 0.0f;
                }
            }
            return;
        }

        if (dyn.colType == COL_AABB && st.colType == COL_AABB) {
            float dx = dyn.pos.x - st.pos.x;
            float dy = dyn.pos.y - st.pos.y;
            float dz = dyn.pos.z - st.pos.z;
            float px = (dyn.size.x + st.size.x) * 0.5f - fabsf(dx);
            float py = (dyn.size.y + st.size.y) * 0.5f - fabsf(dy);
            float pz = (dyn.size.z + st.size.z) * 0.5f - fabsf(dz);
            
            if (px > 0 && py > 0 && pz > 0) {
                float stTopY = st.pos.y + st.size.y * 0.5f;
                float dynBottomY = dyn.pos.y - dyn.size.y * 0.5f;

                if (dy > 0 && (dynBottomY >= stTopY - fabsf(dyn.velocity.y) - 0.05f)) {
                    dyn.pos.y = stTopY + dyn.size.y * 0.5f;
                    dyn.velocity.y = 0.0f;
                    dyn.isGrounded = true;
                    return;
                }

                const float MAX_STEP_HEIGHT = 0.3f;
                if (dy > 0 && stTopY > dynBottomY && (stTopY - dynBottomY) <= MAX_STEP_HEIGHT) {
                    dyn.pos.y = stTopY + dyn.size.y * 0.5f;
                    dyn.velocity.y = 0.0f;
                    dyn.isGrounded = true;
                    return;
                }

                if (py < px && py < pz) {
                    if (dy > 0) {
                        dyn.pos.y += py;
                        if (dyn.velocity.y < 0) dyn.velocity.y = 0.0f;
                        dyn.isGrounded = true;
                    } else {
                        dyn.pos.y -= py;
                        if (dyn.velocity.y > 0) dyn.velocity.y = 0.0f;
                    }
                } else if (px < py && px < pz) {
                    dyn.pos.x += (dx > 0) ? px : -px;
                    dyn.velocity.x = 0.0f;
                } else {
                    dyn.pos.z += (dz > 0) ? pz : -pz;
                    dyn.velocity.z = 0.0f;
                }
            }
            return;
        }

        if (dyn.colType == COL_AABB && st.colType == COL_OBB) {
            Body temp = dyn;
            temp.colType = COL_OBB;
            D3DXMatrixIdentity(&temp.matRotation);
            D3DXVECTOR3 overlap, axis;
            if (CheckOBBvsOBB(st, temp, overlap, axis)) {
                dyn.pos += overlap;
                if (axis.y > 0.7f) dyn.isGrounded = true;
            }
            return;
        }

        if (dyn.colType == COL_OBB && st.colType == COL_AABB) {
            Body temp = st;
            temp.colType = COL_OBB;
            D3DXMatrixIdentity(&temp.matRotation);
            D3DXVECTOR3 overlap, axis;
            if (CheckOBBvsOBB(temp, dyn, overlap, axis)) {
                dyn.pos += overlap;
                if (axis.y > 0.7f) dyn.isGrounded = true;
            }
            return;
        }
    }
}
