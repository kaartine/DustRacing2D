// This file is part of Dust Racing 2D.
// Copyright (C) 2013 Jussi Lind <jussi.lind@iki.fi>
//
// Dust Racing 2D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Dust Racing 2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dust Racing 2D. If not, see <http://www.gnu.org/licenses/>.

#ifndef AUDIOTHREAD_HPP
#define AUDIOTHREAD_HPP

#include <QThread>
#include <QString>

#include <map>

#include "openaldevice.hpp"
#include "openalsource.hpp"

class AudioSource;

class AudioThread : public QThread
{
    Q_OBJECT

public:

    explicit AudioThread(int numCars, QObject * parent = nullptr);

    virtual ~AudioThread();

    virtual void run();

    void connectAudioSource(AudioSource & source);

    void disconnectAudioSource(AudioSource & source);

public slots:

    void playSound(const QString & handle, bool loop = false);

    void stopSound(const QString & handle);

    void setPitch(const QString & handle, float pitch);

    void setVolume(const QString & handle, float pitch);

    void setMasterVolume(float volume);

    void setLocation(const QString & handle, float x, float y);

    void setListenerLocation(float x, float y);

private:

    void init();

    void loadSounds();

    void loadCommonSound(QString handle, QString path);

    void loadSceneSound(QString handle, QString path);

    void loadMultiSound(QString baseName, QString path);

    STFH::DevicePtr m_openALDevice;

    typedef std::map<QString, STFH::SourcePtr> SoundMap;
    SoundMap m_soundMap;

    bool m_inited;

    float m_masterVolume;

    int m_numCars;
};

#endif // AUDIOTHREAD_HPP
