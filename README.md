# Isometric Tile Rendering

Every tile-based lab so far (Pacman, Platformer, Steering and Pathfinding) has
used `LevelSystem` to turn a text file into a flat, top-down or side-on grid.
This practical asks a different question: can the same engine render an
isometric view instead, without rewriting `LevelSystem` itself?

## Why not subclass LevelSystem

`LevelSystem` looks like an ordinary class but isn't used like one. Every
member is `static`, the constructor is deleted, and the whole codebase
accesses it through the `#define ls LevelSystem` macro, not an object
reference. Static methods aren't virtual, so there's no dispatch to
override. A subclass could define its own `getTilePosition`, but every
existing call site (`astar.cpp`, every scene in every earlier lab) calls
`ls::getTilePosition(...)`, which the macro resolves to the literal
`LevelSystem` name. Your subclass's version would never run unless you
rewrote every call site to name it explicitly, at which point you've lost
the point of subclassing in the first place.

## Composition instead

Split the problem in two. `LevelSystem` already does this by accident:
it's a tile *data* layer (`getTile`, `getWidth`, `getHeight`, `findTiles`,
loaded from the text file) bolted onto an orthogonal *positioning/rendering*
layer (`getTilePosition`, `buildSprites`). The data layer doesn't care how
you draw it.

So: keep using `LevelSystem` exactly as-is for loading and storage. Write
your own positioning and rendering as a separate Component instead, one
that reads tiles through `ls::getWidth()` / `ls::getHeight()` / `ls::getTile()`
only, and does its own screen-position math. `LevelSystem::getTilePosition`
and `buildSprites` are never touched; every other lab's orthogonal rendering
keeps working exactly as before.

## The projection

Standard 2:1 isometric: moving one tile right on the grid moves you
right-and-down on screen; moving one tile down the grid moves you
left-and-down.

```cpp
sf::Vector2f isoPosition(size_t gridX, size_t gridY, float tileWidth,
                          float tileHeight, sf::Vector2f origin) {
  float x = (float)gridX;
  float y = (float)gridY;
  return sf::Vector2f((x - y) * (tileWidth / 2.f),
                       (x + y) * (tileHeight / 2.f)) + origin;
}
```

This replaces `LevelSystem::getTilePosition`'s orthogonal
`Vector2f(x, y) * tileSize` with its isometric equivalent. Nothing else
about `LevelSystem` changes.

## IsoTileMapComponent

A Component that builds one diamond-shaped sprite per non-empty tile, once,
at load time.

```cpp
//"cmp_iso_tilemap.h"
#pragma once
#include <ecm.h>
#include <SFML/Graphics/ConvexShape.hpp>
#include <vector>

class IsoTileMapComponent : public Component {
public:
  // origin is a constructor parameter, not read from Engine::GetWindow(),
  // because this component is built inside Scene::Load(), which the engine
  // runs on a background thread via LoadAsync(). Querying SFML window state
  // off the main thread is unsafe and can silently stall.
  explicit IsoTileMapComponent(Entity* p, sf::Vector2f origin,
                                float tileWidth = 64.f,
                                float tileHeight = 32.f);
  IsoTileMapComponent() = delete;

  void update(double dt) override {}
  void render() override;

  sf::Vector2f isoPosition(size_t gridX, size_t gridY) const;

private:
  float _tileWidth;
  float _tileHeight;
  sf::Vector2f _origin;

  // Renderer::queue() takes a pointer to a Drawable that must outlive the
  // frame, so these are built once, not per-frame. Sorted back-to-front
  // at build time, so render() just queues them in order: a painter's
  // algorithm.
  std::vector<sf::ConvexShape> _tiles;

  void buildFromLevelSystem();
};
```

```cpp
//"cmp_iso_tilemap.cpp"
#include "cmp_iso_tilemap.h"
#include <LevelSystem.h>
#include <algorithm>
#include <engine.h>
#include <system_renderer.h>

using namespace sf;

IsoTileMapComponent::IsoTileMapComponent(Entity* p, sf::Vector2f origin,
                                          float tileWidth, float tileHeight)
    : Component(p), _tileWidth(tileWidth), _tileHeight(tileHeight),
      _origin(origin) {
  buildFromLevelSystem();
}

sf::Vector2f IsoTileMapComponent::isoPosition(size_t gridX,
                                               size_t gridY) const {
  float x = (float)gridX;
  float y = (float)gridY;
  return Vector2f((x - y) * (_tileWidth / 2.f),
                  (x + y) * (_tileHeight / 2.f)) +
         _origin;
}

void IsoTileMapComponent::buildFromLevelSystem() {
  _tiles.clear();

  struct Pending {
    size_t gridX, gridY;
  };
  std::vector<Pending> pending;

  // Pure data-layer reads, exactly what LevelSystem already offers every
  // other (orthogonal) consumer.
  for (size_t y = 0; y < ls::getHeight(); ++y) {
    for (size_t x = 0; x < ls::getWidth(); ++x) {
      if (ls::getTile({x, y}) != ls::EMPTY) {
        pending.push_back({x, y});
      }
    }
  }

  // Painter's algorithm: draw back-to-front. With no elevation, tiles
  // never actually overlap, but sorting now means this still behaves
  // correctly the moment someone adds height or sprites later.
  std::sort(pending.begin(), pending.end(),
            [](const Pending& a, const Pending& b) {
              return (a.gridX + a.gridY) < (b.gridX + b.gridY);
            });

  _tiles.reserve(pending.size());
  for (const auto& p : pending) {
    ConvexShape diamond(4);
    diamond.setPoint(0, Vector2f(_tileWidth / 2.f, 0.f));
    diamond.setPoint(1, Vector2f(_tileWidth, _tileHeight / 2.f));
    diamond.setPoint(2, Vector2f(_tileWidth / 2.f, _tileHeight));
    diamond.setPoint(3, Vector2f(0.f, _tileHeight / 2.f));
    diamond.setPosition(isoPosition(p.gridX, p.gridY) -
                         Vector2f(_tileWidth / 2.f, _tileHeight / 2.f));

    LevelSystem::Tile t = ls::getTile({p.gridX, p.gridY});
    diamond.setFillColor(t == ls::WALL ? Color(90, 90, 90)
                                        : Color(60, 160, 90));
    diamond.setOutlineColor(Color::Black);
    diamond.setOutlineThickness(1.f);

    _tiles.push_back(std::move(diamond));
  }
}

void IsoTileMapComponent::render() {
  for (auto& shape : _tiles) {
    Renderer::queue(&shape);
  }
}
```

## Scene

```cpp
//"scene_iso_demo.h"
#pragma once
#include "engine.h"

class IsoDemoScene : public Scene {
public:
  void Load() override;
  void UnLoad() override;
  void Update(const double& dt) override;
  void Render() override;
};
```

```cpp
//"scene_iso_demo.cpp"
#include "scene_iso_demo.h"
#include "../components/cmp_iso_tilemap.h"
#include <LevelSystem.h>

void IsoDemoScene::Load() {
  ls::loadLevelFile("res/pacman.txt", 20.0f);
  auto mapEntity = makeEntity();
  // Origin matches the 1000x700 window in main.cpp. Not read from
  // Engine::GetWindow() here, since Load() runs off the main thread.
  mapEntity->addComponent<IsoTileMapComponent>(sf::Vector2f(500.f, 40.f),
                                                32.f, 16.f);
  setLoaded(true);
}

void IsoDemoScene::UnLoad() {
  ls::unload();
  Scene::UnLoad();
}

void IsoDemoScene::Update(const double& dt) { Scene::Update(dt); }

void IsoDemoScene::Render() { Scene::Render(); }
```

```cpp
//"main.cpp"
#include "engine.h"
#include "scenes/scene_iso_demo.h"

IsoDemoScene isoDemo;

int main() {
  Engine::Start(1000, 700, "Isometric Tile Rendering", &isoDemo);
  return 0;
}
```

Run it. You should see the Pacman maze rendered as a diamond-tiled
isometric grid instead of a flat top-down one, built from the exact same
`res/pacman.txt` and the exact same `LevelSystem`.

## Exercise

- Add a player entity that moves between tiles (reuse `isoPosition` to
  place it).
- Add elevation: give each tile a height value and offset its screen `y`
  by `height * someScale`. The painter's-algorithm sort already in
  `buildFromLevelSystem` means tiles will still draw in the right order
  once you do.
- Swap `res/pacman.txt` for a level file of your own.
