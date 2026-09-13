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
        	}
    	}

    	for (size_t i = 0; i < bodies.size(); i++) {
        	Body& b = bodies[i];
        	if (b.isStatic || b.isKinematic) continue;
        
        	for (size_t j = 0; j < bodies.size(); j++) {
            	if (i != j) {
                	ResolveCollision(b, bodies[j]);
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

    void Physics::ResolveCollision(Body& a, Body& b) {
    	if (a.id == b.id) return;
    	bool aIsPlayer = (a.id == 1);
    	bool bIsPlayer = (b.id == 1);
    	
    	if (a.colType == COL_OBB && b.colType == COL_OBB) {
        	D3DXVECTOR3 overlap, axis;
        	if (CheckOBBvsOBB(b, a, overlap, axis)) {
            	if (aIsPlayer || bIsPlayer) {
                	Body& player = aIsPlayer ? a : b;
                	Body& other  = aIsPlayer ? b : a;
                	float sign = aIsPlayer ? 1.0f : -1.0f;
                
                	player.pos += overlap * sign;
                
                	if (!other.isStatic && !other.isKinematic) {
                    	other.velocity.x += player.velocity.x * 0.5f;
                    	other.velocity.z += player.velocity.z * 0.5f;
                    	other.pos -= overlap * sign * 0.3f;
                	}
                
                	if (axis.y > 0.7f) {
                    	player.velocity.y = 0.0f;
                    	player.isGrounded = true;
                	} else if (axis.y < -0.7f) {
                    	if (player.velocity.y > 0) player.velocity.y = 0.0f;
                	}
                
                	return;
            	}
            
            	float aPush = a.isStatic ? 0.0f : 0.5f;
            	float bPush = b.isStatic ? 0.0f : 0.5f;
            
            	if (!a.isStatic && !a.isKinematic) a.pos += overlap * aPush;
            	if (!b.isStatic && !b.isKinematic) b.pos -= overlap * bPush;
            
            	if (axis.y > 0.7f) {
                	if (!a.isStatic) { a.velocity.y = 0.0f; a.isGrounded = true; }
                	if (!b.isStatic) { b.velocity.y = 0.0f; b.isGrounded = true; }
            	} else if (axis.y < -0.7f) {
                	if (!a.isStatic && a.velocity.y > 0) a.velocity.y = 0.0f;
                	if (!b.isStatic && b.velocity.y > 0) b.velocity.y = 0.0f;
            	} else {
                	if (!a.isStatic) { a.velocity.x = 0.0f; a.velocity.z = 0.0f; }
                	if (!b.isStatic) { b.velocity.x = 0.0f; b.velocity.z = 0.0f; }
            	}
        	}
        	return;
    	}
    
    	if (a.colType == COL_AABB && b.colType == COL_AABB) {
        	float dx = a.pos.x - b.pos.x;
        	float dy = a.pos.y - b.pos.y;
        	float dz = a.pos.z - b.pos.z;
        	float px = (a.size.x + b.size.x) * 0.5f - fabsf(dx);
        	float py = (a.size.y + b.size.y) * 0.5f - fabsf(dy);
        	float pz = (a.size.z + b.size.z) * 0.5f - fabsf(dz);
        
        	if (px > 0 && py > 0 && pz > 0) {

            	if (aIsPlayer || bIsPlayer) {
                	Body& player = aIsPlayer ? a : b;
                	Body& other  = aIsPlayer ? b : a;
                
                	if (py < px && py < pz) {
                    	if (dy > 0) {
                        	player.pos.y = other.pos.y + other.size.y * 0.5f + player.size.y * 0.5f;
                        	player.velocity.y = 0.0f;
                        	player.isGrounded = true;
                    	} else {
                        	player.pos.y = other.pos.y - other.size.y * 0.5f - player.size.y * 0.5f;
                        	if (player.velocity.y > 0) player.velocity.y = 0.0f;
                    	}
                	} else if (px < pz) {
                    	player.pos.x += (dx > 0) ? px : -px;
                    	player.velocity.x = 0.0f;
                    	if (!other.isStatic && !other.isKinematic) {
                        	other.velocity.x += player.velocity.x * 0.5f;
                    	}
                	} else {
                    	player.pos.z += (dz > 0) ? pz : -pz;
                    	player.velocity.z = 0.0f;
                    	if (!other.isStatic && !other.isKinematic) {
                        	other.velocity.z += player.velocity.z * 0.5f;
                    	}
                	}
                	return;
            	}
            	if (py < px && py < pz) {
                	if (dy > 0) {
                    	if (!a.isStatic) { a.pos.y += py; a.velocity.y = 0.0f; a.isGrounded = true; }
                    	if (!b.isStatic) { b.pos.y -= py; b.velocity.y = 0.0f; b.isGrounded = true; }
                	} else {
                    	if (!a.isStatic) { a.pos.y -= py; if (a.velocity.y > 0) a.velocity.y = 0.0f; }
                    	if (!b.isStatic) { b.pos.y += py; if (b.velocity.y > 0) b.velocity.y = 0.0f; }
                	}
            	} else if (px < pz) {
                	if (!a.isStatic) { a.pos.x += (dx > 0) ? px : -px; a.velocity.x = 0.0f; }
                	if (!b.isStatic) { b.pos.x -= (dx > 0) ? px : -px; b.velocity.x = 0.0f; }
            	} else {
                	if (!a.isStatic) { a.pos.z += (dz > 0) ? pz : -pz; a.velocity.z = 0.0f; }
                	if (!b.isStatic) { b.pos.z -= (dz > 0) ? pz : -pz; b.velocity.z = 0.0f; }
            	}
        	}
        	return;
    	}
    	
    	if (a.colType == COL_OBB && b.colType == COL_AABB) {
        	Body temp = b;
        	temp.colType = COL_OBB;
        	D3DXMatrixIdentity(&temp.matRotation);
        	D3DXVECTOR3 overlap, axis;
        	if (CheckOBBvsOBB(temp, a, overlap, axis)) {
            	if (aIsPlayer || bIsPlayer) {
                	Body& player = aIsPlayer ? a : b;
                	Body& other  = aIsPlayer ? b : a;
                	float sign = aIsPlayer ? 1.0f : -1.0f;
                	player.pos += overlap * sign;
                	if (!other.isStatic && !other.isKinematic) {
                    	other.velocity.x += player.velocity.x * 0.5f;
                    	other.velocity.z += player.velocity.z * 0.5f;
                	}
                	if (axis.y > 0.7f) { player.velocity.y = 0.0f; player.isGrounded = true; }
            	} else {
                	if (!a.isStatic) a.pos += overlap * 0.5f;
                	if (!b.isStatic) b.pos -= overlap * 0.5f;
                	if (axis.y > 0.7f) a.isGrounded = true;
            	}
        	}
        	return;
    	}
    
    	if (a.colType == COL_AABB && b.colType == COL_OBB) {
        	Body temp = a;
        	temp.colType = COL_OBB;
        	D3DXMatrixIdentity(&temp.matRotation);
        	D3DXVECTOR3 overlap, axis;
        	if (CheckOBBvsOBB(b, temp, overlap, axis)) {
            	if (aIsPlayer || bIsPlayer) {
                	Body& player = aIsPlayer ? a : b;
                	Body& other  = aIsPlayer ? b : a;
                	float sign = aIsPlayer ? 1.0f : -1.0f;
                	player.pos += overlap * sign;
                	if (!other.isStatic && !other.isKinematic) {
                    	other.velocity.x += player.velocity.x * 0.5f;
                    	other.velocity.z += player.velocity.z * 0.5f;
                	}
                	if (axis.y > 0.7f) { player.velocity.y = 0.0f; player.isGrounded = true; }
            	} else {
                	if (!a.isStatic) a.pos += overlap * 0.5f;
                	if (!b.isStatic) b.pos -= overlap * 0.5f;
                	if (axis.y > 0.7f) b.isGrounded = true;
            	}
        	}
        	return;
    	}
	}
}
