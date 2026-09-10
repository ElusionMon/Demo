#include "MyEngine.h"

namespace MyEngine {

    bool SpriteRenderer::Init(LPDIRECT3DDEVICE9 dev) {
        return SUCCEEDED(D3DXCreateSprite(dev, &spriteAPI));
    }

    void SpriteRenderer::Begin() {
        if (spriteAPI) spriteAPI->Begin(D3DXSPRITE_ALPHABLEND);
    }

    void SpriteRenderer::End() {
        if (spriteAPI) spriteAPI->End();
    }

    void SpriteRenderer::Draw(std::shared_ptr<IDirect3DTexture9> tex, int x, int y, int w, int h, DWORD color) {
        if (!spriteAPI || !tex) return;

        D3DSURFACE_DESC desc;
        tex->GetLevelDesc(0, &desc);

        D3DXMATRIX matScale, matTrans, matFinal;
        float scaleX = (float)w / desc.Width;
        float scaleY = (float)h / desc.Height;
        D3DXMatrixScaling(&matScale, scaleX, scaleY, 1.0f);
        D3DXMatrixTranslation(&matTrans, (float)x, (float)y, 0.0f);
        
        matFinal = matScale * matTrans;
        spriteAPI->SetTransform(&matFinal);
        spriteAPI->Draw(tex.get(), NULL, NULL, NULL, color);
        
        D3DXMATRIX matId; D3DXMatrixIdentity(&matId);
        spriteAPI->SetTransform(&matId);
    }

    Button::Button(int _x, int _y, int _w, int _h, std::function<void()> callback) 
        : x(_x), y(_y), width(_w), height(_h), onClickCallback(callback) {}

    void Button::Update(Input& input) {
        POINT mouse = input.GetMousePos();
        if (mouse.x >= x && mouse.x <= x + width && mouse.y >= y && mouse.y <= y + height) {
            isHovered = true;
            if (input.IsKeyPressed(0x01) && onClickCallback) {
                onClickCallback(); 
            }
        } else {
            isHovered = false;
        }
    }

    void Button::Draw(SpriteRenderer& sprite, Text& text, 
                      std::shared_ptr<IDirect3DTexture9> texNormal, 
                      std::shared_ptr<IDirect3DTexture9> texHover, 
                      std::string label, DWORD textColor) 
    {
        auto currentTex = isHovered ? texHover : texNormal;
        if (currentTex) {
            sprite.Draw(currentTex, x, y, width, height);
        }
        int textX = x + (width - (int)label.length() * 9) / 2;
        int textY = y + (height - 20) / 2;
        text.Draw(label, textX, textY, textColor);
    }
}
