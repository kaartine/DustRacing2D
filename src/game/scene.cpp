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

#include "scene.hpp"

#include "ailogic.hpp"
#include "car.hpp"
#include "checkeredflag.hpp"
#include "inputhandler.hpp"
#include "layers.hpp"
#include "menu.hpp"
#include "menuitem.hpp"
#include "menuitemview.hpp"
#include "menumanager.hpp"
#include "offtrackdetector.hpp"
#include "race.hpp"
#include "startlights.hpp"
#include "startlightsoverlay.hpp"
#include "statemachine.hpp"
#include "timingoverlay.hpp"
#include "track.hpp"
#include "trackdata.hpp"
#include "trackobject.hpp"
#include "trackpreviewoverlay.hpp"
#include "tracktile.hpp"

#include "../common/config.hpp"
#include "../common/targetnodebase.hpp"

#include "MiniCore/Core/MCCamera"
#include "MiniCore/Core/MCLogger"
#include "MiniCore/Core/MCObject"
#include "MiniCore/Core/MCSurface"
#include "MiniCore/Core/MCTextureManager"
#include "MiniCore/Core/MCTypes"
#include "MiniCore/Core/MCWorld"
#include "MiniCore/Core/Physics/MCFrictionGenerator"
#include "MiniCore/Core/Physics/MCSpringForceGenerator"

#include <algorithm>
#include <cassert>

Scene::Scene(Renderer & renderer, unsigned int numCars)
: m_race(numCars)
, m_activeTrack(nullptr)
, m_world(new MCWorld)
, m_timingOverlay(new TimingOverlay)
, m_startlights(new Startlights(m_race))
, m_startlightsOverlay(new StartlightsOverlay(*m_startlights))
, m_stateMachine(new StateMachine(renderer, *m_startlights))
, m_checkeredFlag(new CheckeredFlag)
, m_cameraBaseOffset(0)
, m_trackPreviewOverlay(new TrackPreviewOverlay)
, m_pMainMenu(nullptr)
, m_pMenuManager(nullptr)
{
    const int humanPower = 7500;

    // Create and add cars.
    assert(numCars);
    for (MCUint i = 0; i < numCars; i++)
    {
        Car::Description desc;

        Car * car = nullptr;
        if (i == 0)
        {
            desc.power = humanPower;
            car = new Car(desc, MCTextureManager::instance().surface("car001"), i, true);
        }
        else
        {
            // Introduce some variance to the power of computer players so that the
            // slowest cars have less power than the human player and the fastest
            // cars have more power than the human player.
            desc.power = humanPower - humanPower / 2 + i * humanPower / numCars;
            car = new Car(desc, MCTextureManager::instance().surface("car002"), i, false);
            m_aiLogic.push_back(new AiLogic(*car));
        }

        car->setLayer(Layers::Cars);

        m_cars.push_back(car);
        m_race.addCar(*car);

        m_offTrackDetectors.push_back(new OffTrackDetector(*car));
    }

    m_startlightsOverlay->setDimensions(width(), height());
    m_checkeredFlag->setDimensions(width(), height());

    m_timingOverlay->setDimensions(width(), height());
    m_timingOverlay->setTiming(m_race.timing());
    m_timingOverlay->setRace(m_race);
    m_timingOverlay->setCarToFollow(*m_cars.at(0));

    m_trackPreviewOverlay->setDimensions(width(), height());

    MCWorld::instance().enableDepthTestOnLayer(Layers::Tree, true);

    createMenus();
}

unsigned int Scene::width()
{
    return 800;
}

unsigned int Scene::height()
{
    return 600;
}

void Scene::createMenus()
{
    const int width = Config::Game::WINDOW_WIDTH;
    const int height = Config::Game::WINDOW_HEIGHT;

    m_pMenuManager = new MenuManager;
    m_pMainMenu = new Menu(width, height);

    MenuItem * play = new MenuItem(width, height / 2, "Play");
    play->setView(new MenuItemView, true);

    MenuItem * quit = new MenuItem(width, height / 2, "Quit");
    quit->setView(new MenuItemView, true);

    m_pMainMenu->addItem(*play, true);
    m_pMainMenu->addItem(*quit, true);

    m_pMenuManager->enterMenu(*m_pMainMenu);
}

void Scene::updateFrame(InputHandler & handler,
    MCCamera & camera, float timeStep)
{
    if (m_race.started())
    {
        processUserInput(handler);

        updateAiLogic();
    }

    updateWorld(timeStep);

    updateRace();

    for (OffTrackDetector * otd : m_offTrackDetectors)
    {
        otd->update();
    }

    updateCameraLocation(camera);
}

void Scene::updateAnimations()
{
    m_stateMachine->update();
    m_timingOverlay->update();
}

void Scene::updateWorld(float timeStep)
{
    // Step time
    m_world->stepTime(timeStep);
}

void Scene::updateRace()
{
    // Update race situation
    m_race.update();
}

void Scene::updateCameraLocation(MCCamera & camera)
{
    // Update camera location with respect to the car speed.
    // Make changes a bit smoother so that an abrupt decrease
    // in the speed won't look bad.
    MCVector2d<MCFloat> p(m_cars.at(0)->location());

    const float offsetAmplification = 10;
    const float smooth              = 0.2;

    m_cameraBaseOffset +=
        (m_cars.at(0)->velocity().lengthFast() - m_cameraBaseOffset) * smooth;
    p += m_cars.at(0)->direction() * m_cameraBaseOffset * offsetAmplification;

    camera.setPos(p.i(), p.j());
}

void Scene::processUserInput(InputHandler & handler)
{
    bool steering = false;

    m_cars.at(0)->clearStatuses();

    // Handle accelerating / braking
    if (handler.getActionState(0, InputHandler::IA_UP))
    {
        m_cars.at(0)->accelerate();
    }
    else if (handler.getActionState(0, InputHandler::IA_DOWN))
    {
        m_cars.at(0)->brake();
    }

    // Handle turning
    if (handler.getActionState(0, InputHandler::IA_LEFT))
    {
        m_cars.at(0)->turnLeft();
        steering = true;
    }
    else if (handler.getActionState(0, InputHandler::IA_RIGHT))
    {
        m_cars.at(0)->turnRight();
        steering = true;
    }

    if (!steering)
    {
        m_cars.at(0)->noSteering();
    }
}

void Scene::updateAiLogic()
{
    for (AiLogic * ai : m_aiLogic)
    {
        const bool isRaceCompleted = m_race.timing().raceCompleted(ai->car().index());
        ai->update(isRaceCompleted);
    }
}

void Scene::setActiveTrack(Track & activeTrack)
{
    m_activeTrack = &activeTrack;
    m_stateMachine->setTrack(*m_activeTrack);

    // TODO: Remove objects
    // TODO: Removing not inserted objects results in a
    // crash because ObjectTree can't handle it.
    //m_car.removeFromWorldNow();

    setWorldDimensions();

    addCarsToWorld();

    translateCarsToStartPositions();

    addTrackObjectsToWorld();

    initRace();

    for (AiLogic * ai : m_aiLogic)
    {
        ai->setTrack(activeTrack);
    }

    for (OffTrackDetector * otd : m_offTrackDetectors)
    {
        otd->setTrack(activeTrack);
    }

    m_trackPreviewOverlay->setTrack(&activeTrack);
}

void Scene::setWorldDimensions()
{
    assert(m_world);
    assert(m_activeTrack);

    // Update world dimensions according to the
    // active track.
    const MCUint minX = 0;
    const MCUint maxX = m_activeTrack->width();
    const MCUint minY = 0;
    const MCUint maxY = m_activeTrack->height();
    const MCUint minZ = 0;
    const MCUint maxZ = 1000;

    const MCFloat metersPerPixel = 0.05f;
    m_world->setDimensions(minX, maxX, minY, maxY, minZ, maxZ, metersPerPixel);
}

void Scene::addCarsToWorld()
{
    // Add objects to the world
    for (Car * car : m_cars)
    {
        car->addToWorld();
    }
}

void Scene::translateCarsToStartPositions()
{
    assert(m_activeTrack);

    if (TrackTile * finishLine = m_activeTrack->finishLine())
    {
        const MCFloat startTileX = finishLine->location().x();
        const MCFloat startTileY = finishLine->location().y();
        const MCFloat tileWidth  = TrackTile::TILE_W;
        const MCFloat tileHeight = TrackTile::TILE_H;

        // Reverse order
        std::vector<Car *> order = m_cars;
        std::reverse(order.begin(), order.end());

        // Position the cars into two queues.
        const int routeDirection = finishLine->rotation() % 360;
        switch (routeDirection)
        {
        case 90:
        case -270:
            for (MCUint i = 0; i < order.size(); i++)
            {
                const MCFloat rowPos = (i / 2) * tileWidth;
                const MCFloat colPos = (i % 2) * tileHeight / 3 - tileHeight / 6;

                order.at(i)->translate(
                    MCVector2d<MCFloat>(
                        startTileX + rowPos,
                        startTileY + colPos));

                order.at(i)->rotate(180);
            }
            break;

        default:
        case 270:
        case -90:
            for (MCUint i = 0; i < order.size(); i++)
            {
                const MCFloat rowPos = (i / 2) * tileWidth;
                const MCFloat colPos = (i % 2) * tileHeight / 3 - tileHeight / 6;

                order.at(i)->translate(
                    MCVector2d<MCFloat>(
                        startTileX - rowPos,
                        startTileY + colPos));

                order.at(i)->rotate(0);
            }
            break;

        case 0:
            for (MCUint i = 0; i < order.size(); i++)
            {
                const MCFloat rowPos = (i % 2) * tileWidth / 3 - tileWidth / 6;
                const MCFloat colPos = (i / 2) * tileHeight;

                order.at(i)->translate(
                    MCVector2d<MCFloat>(
                        startTileX + rowPos,
                        startTileY - colPos));

                order.at(i)->rotate(90);
            }
            break;

        case 180:
        case -180:
            for (MCUint i = 0; i < order.size(); i++)
            {
                const MCFloat rowPos = (i % 2) * tileWidth / 3 - tileWidth / 6;
                const MCFloat colPos = (i / 2) * tileHeight;

                order.at(i)->translate(
                    MCVector2d<MCFloat>(
                        startTileX + rowPos,
                        startTileY + colPos));

                order.at(i)->rotate(270);
            }
            break;
        }
    }
    else
    {
        MCLogger().error() << "Finish line tile not found in track '" <<
            m_activeTrack->trackData().name().toStdString() << "'";
    }
}

void Scene::addTrackObjectsToWorld()
{
    assert(m_activeTrack);

    const unsigned int trackObjectCount =
        m_activeTrack->trackData().objects().count();

    for (unsigned int i = 0; i < trackObjectCount; i++)
    {
        TrackObject & trackObject = static_cast<TrackObject &>(
            m_activeTrack->trackData().objects().object(i));
        MCObject & mcObject = trackObject.object();
        mcObject.addToWorld();
        mcObject.translate(mcObject.initialLocation());
        mcObject.rotate(mcObject.initialAngle());
    }
}

void Scene::initRace()
{
    assert(m_activeTrack);
    m_race.setTrack(*m_activeTrack);
    m_race.init();
}

Track & Scene::activeTrack() const
{
    assert(m_activeTrack);
    return *m_activeTrack;
}

MCWorld & Scene::world() const
{
    assert(m_world);
    return *m_world;
}

void Scene::render(MCCamera & camera)
{
    if (m_stateMachine->state() == StateMachine::Intro)
    {
        const int w2 = width() / 2;
        const int h2 = height() / 2;
        static MCSurface & surface = MCTextureManager::instance().surface("dustRacing");
        surface.renderScaled(nullptr, MCVector3dF(w2, h2, 0), w2, h2, 0);
    }
    else if (
        m_stateMachine->state() == StateMachine::GameTransitionIn ||
        m_stateMachine->state() == StateMachine::DoStartlights ||
        m_stateMachine->state() == StateMachine::Play)
    {
        m_activeTrack->render(&camera);
        m_world->renderShadows(&camera);
        m_world->render(&camera);

        if (m_race.checkeredFlagEnabled())
        {
            m_checkeredFlag->render();
        }

        m_timingOverlay->render();
        m_startlightsOverlay->render();
    }

    m_trackPreviewOverlay->render();
}

Scene::~Scene()
{
    for (Car * car : m_cars)
    {
        delete car;
    }

    for (AiLogic * ai : m_aiLogic)
    {
        delete ai;
    }

    for (OffTrackDetector * otd : m_offTrackDetectors)
    {
        delete otd;
    }

    delete m_startlights;
    delete m_startlightsOverlay;
    delete m_stateMachine;
    delete m_timingOverlay;
    delete m_pMainMenu;
    delete m_pMenuManager;
}
