// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include <string>
#include <memory>
#include <functional>

namespace sf {

class SoundBuffer;
class Music;

} // namespace sf

namespace hg {

class Audio
{
public:
    using SoundBufferGetter =
        std::function<sf::SoundBuffer*(const std::string&)>;

    using MusicGetter = std::function<sf::Music*(const std::string&)>;

private:
    class AudioImpl;

    std::unique_ptr<AudioImpl> _impl;

    [[nodiscard]] const AudioImpl& impl() const noexcept;
    [[nodiscard]] AudioImpl& impl() noexcept;

public:
    explicit Audio(const SoundBufferGetter& soundBufferGetter,
        const MusicGetter& musicGetter);

    ~Audio();

    void setSoundVolume(const float volume);
    void setMusicVolume(const float volume);

    void resumeMusic();
    void pauseMusic();
    void stopMusic();

    void stopSounds();

    void playSoundOverride(const std::string& id);
    void playPackSoundOverride(
        const std::string& packId, const std::string& id);

    void playSoundAbort(const std::string& id);
    void playPackSoundAbort(const std::string& packId, const std::string& id);

    void playMusic(const std::string& packId, const std::string& id,
        const float playingOffsetSeconds);

    void setCurrentMusicPitch(const float pitch);
};

} // namespace hg