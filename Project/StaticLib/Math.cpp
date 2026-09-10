#include "MyEngine.h"

namespace MyEngine {

    void Frustum::Construct(const D3DXMATRIX& view, const D3DXMATRIX& proj) {
        D3DXMATRIX combo;
        D3DXMatrixMultiply(&combo, &view, &proj);

        planes[0].a = combo._14 + combo._11; planes[0].b = combo._24 + combo._21; planes[0].c = combo._34 + combo._31; planes[0].d = combo._44 + combo._41;
        planes[1].a = combo._14 - combo._11; planes[1].b = combo._24 - combo._21; planes[1].c = combo._34 - combo._31; planes[1].d = combo._44 - combo._41;
        planes[2].a = combo._14 + combo._12; planes[2].b = combo._24 + combo._22; planes[2].c = combo._34 + combo._32; planes[2].d = combo._44 + combo._42;
        planes[3].a = combo._14 - combo._12; planes[3].b = combo._24 - combo._22; planes[3].c = combo._34 - combo._32; planes[3].d = combo._44 - combo._42;
        planes[4].a = combo._14 + combo._13; planes[4].b = combo._24 + combo._23; planes[4].c = combo._34 + combo._33; planes[4].d = combo._44 + combo._43;
        planes[5].a = combo._14 - combo._13; planes[5].b = combo._24 - combo._23; planes[5].c = combo._34 - combo._33; planes[5].d = combo._44 - combo._43;

        for (int i = 0; i < 6; i++) {
            D3DXPlaneNormalize(&planes[i], &planes[i]);
        }
    }

    bool Frustum::CheckBox(const D3DXVECTOR3& minPt, const D3DXVECTOR3& maxPt) {
        for (int i = 0; i < 6; i++) {
            D3DXVECTOR3 v;
            v.x = (planes[i].a >= 0.0f) ? maxPt.x : minPt.x;
            v.y = (planes[i].b >= 0.0f) ? maxPt.y : minPt.y;
            v.z = (planes[i].c >= 0.0f) ? maxPt.z : minPt.z;
            
            float distance = planes[i].a * v.x + planes[i].b * v.y + planes[i].c * v.z + planes[i].d;
            if (distance < 0.0f) {
                return false;
            }
        }
        return true;
    }
    
    bool Frustum::CheckSphere(const D3DXVECTOR3& center, float radius) {
        for (int i = 0; i < 6; i++) {
            float distance = planes[i].a * center.x + planes[i].b * center.y + planes[i].c * center.z + planes[i].d;
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }
}
