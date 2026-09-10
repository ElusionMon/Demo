#include "MyEngine.h"
#include <windows.h>
#include <mmsystem.h>

namespace MyEngine {

    std::string AudioSystem::GenerateAlias() {
        return "snd_" + std::to_string(aliasCounter++);
    }

    AudioSystem::~AudioSystem() {
        for (auto const& pair : soundAliases) {
            std::string command = "close " + pair.second;
            mciSendStringA(command.c_str(), NULL, 0, NULL);
        }
    }

    bool AudioSystem::Load(std::string path, std::string soundName) {
        if (soundAliases.count(soundName)) return true;
        std::string alias = GenerateAlias();
        std::string command = "open \"" + path + "\" type mpegvideo alias " + alias;
        if (mciSendStringA(command.c_str(), NULL, 0, NULL) == 0) {
            soundAliases[soundName] = alias; return true;
        }
        return false;
    }

    void AudioSystem::Play(std::string soundName, bool loop) {
        if (!soundAliases.count(soundName)) return;
        std::string alias = soundAliases[soundName];
        std::string seekCmd = "seek " + alias + " to start";
        mciSendStringA(seekCmd.c_str(), NULL, 0, NULL);
        std::string playCmd = "play " + alias;
        if (loop) playCmd += " repeat";
        mciSendStringA(playCmd.c_str(), NULL, 0, NULL);
    }

    void AudioSystem::Stop(std::string soundName) {
        if (!soundAliases.count(soundName)) return;
        std::string stopCmd = "stop " + soundAliases[soundName];
        mciSendStringA(stopCmd.c_str(), NULL, 0, NULL);
    }

    void AudioSystem::SetVolume(std::string soundName, int volume) {
        if (!soundAliases.count(soundName)) return;
        if (volume < 0) volume = 0; if (volume > 1000) volume = 1000;
        std::string volCmd = "setaudio " + soundAliases[soundName] + " volume to " + std::to_string(volume);
        mciSendStringA(volCmd.c_str(), NULL, 0, NULL);
    }
}
