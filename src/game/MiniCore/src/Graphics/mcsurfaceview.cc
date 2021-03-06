// This file belongs to the "MiniCore" game engine.
// Copyright (C) 2015 Jussi Lind <jussi.lind@iki.fi>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//

#include "mcglshaderprogram.hh"
#include "mcsurfaceview.hh"
#include "mccamera.hh"
#include "mcsurface.hh"

MCSurfaceView::MCSurfaceView(const std::string & viewId, MCSurface * surface)
    : MCShapeView(viewId)
    , m_surface(surface)
{
    if (surface)
    {
        m_surface->setShaderProgram(shaderProgram());
        m_surface->setShadowShaderProgram(shadowShaderProgram());

        updateBBox();
    }
}

MCSurfaceView::~MCSurfaceView()
{}

void MCSurfaceView::updateBBox()
{
    // TODO: Fix this! The view should know the angle of the
    // shape somehow. Now we just return a naive bbox.

    const float w = m_surface->width() / 2;
    const float h = m_surface->height() / 2;
    const float r = std::max(w, h);

    m_bbox = MCBBoxF(-r * scale().i(), -r * scale().j(), r * scale().i(), r * scale().j());
}

void MCSurfaceView::setSurface(MCSurface & surface)
{
    m_surface = &surface;
    m_surface->setShaderProgram(shaderProgram());
    m_surface->setShadowShaderProgram(shadowShaderProgram());

    setHandle(surface.handle());

    updateBBox();
}

MCSurface * MCSurfaceView::surface() const
{
    return m_surface;
}

void MCSurfaceView::setShaderProgram(MCGLShaderProgramPtr program)
{
    MCShapeView::setShaderProgram(program);
    m_surface->setShaderProgram(program);
}

void MCSurfaceView::setShadowShaderProgram(MCGLShaderProgramPtr program)
{
    MCShapeView::setShadowShaderProgram(program);
    m_surface->setShadowShaderProgram(program);
}

void MCSurfaceView::render(const MCVector3dF & l, float angle, MCCamera * p)
{
    m_surface->setScale(scale());
    m_surface->render(p, l, angle);
}

void MCSurfaceView::renderShadow(const MCVector3dF & l, float angle, MCCamera * p)
{
    m_surface->setScale(scale());
    m_surface->renderShadow(p, l, angle);
}

void MCSurfaceView::bind()
{
    m_surface->bind();
}

void MCSurfaceView::bindShadow()
{
    m_surface->bindShadow();
}

void MCSurfaceView::release()
{
    m_surface->release();
}

void MCSurfaceView::releaseShadow()
{
    m_surface->releaseShadow();
}

const MCBBoxF & MCSurfaceView::bbox() const
{
    return m_bbox;
}

void MCSurfaceView::setColor(const MCGLColor & color)
{
    MCShapeView::setColor(color);
    m_surface->setColor(color);
}

void MCSurfaceView::setScale(const MCVector3dF &scale)
{
    MCShapeView::setScale(scale);
    updateBBox();
}

MCGLObjectBase * MCSurfaceView::object() const
{
    return m_surface;
}
