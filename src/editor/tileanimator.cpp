// This file is part of Dust Racing (DustRAC).
// Copyright (C) 2011 Jussi Lind <jussi.lind@iki.fi>
//
// DustRAC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// DustRAC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DustRAC. If not, see <http://www.gnu.org/licenses/>.

#include "tracktile.h"
#include "tileanimator.h"

#include <QTimeLine>

const int FRAMES = 30;

TileAnimator::TileAnimator(TrackTile * tile) :
    QTimeLine(250),
    m_tile(tile)
{
    setFrameRange(0, FRAMES);

    connect(this, SIGNAL(frameChanged(int)), this, SLOT(setTileRotation(int)));
}

void TileAnimator::rotate90CW()
{
    if (state() == QTimeLine::NotRunning)
    {
        m_a0 = m_tile->rotation();
        m_a1 = m_tile->rotation() + 90;

        start();
    }
}

void TileAnimator::rotate90CCW()
{
    if (state() == QTimeLine::NotRunning)
    {
        m_a0 = m_tile->rotation();
        m_a1 = m_tile->rotation() - 90;

        start();
    }
}

void TileAnimator::setTileRotation(int frame)
{
    m_tile->setRotation(m_a0 + frame * (m_a1 - m_a0) / FRAMES);
}
