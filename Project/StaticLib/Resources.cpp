#include "MyEngine.h"

namespace MyEngine {
    std::shared_ptr<IDirect3DTexture9> ResourceManager::GetTexture(std::string p) {
        if (cache.count(p)) return cache[p];
        
        if (failedAttempts[p] >= MAX_FAILED_ATTEMPTS) {
            return nullptr;
        }
        
        LPDIRECT3DTEXTURE9 t;
        if (SUCCEEDED(D3DXCreateTextureFromFileA(dev, p.c_str(), &t))) {
            failedAttempts[p] = 0;
            return cache[p] = std::shared_ptr<IDirect3DTexture9>(t, 
                [](IDirect3DTexture9* ptr){ if(ptr) ptr->Release(); });
        }
        
        failedAttempts[p]++;
        return nullptr;
    }

    void ResourceManager::ClearUnused() {
        auto it = cache.begin();
        while (it != cache.end()) {
            if (it->second.use_count() == 1) it = cache.erase(it);
            else ++it;
        }
    }
}
