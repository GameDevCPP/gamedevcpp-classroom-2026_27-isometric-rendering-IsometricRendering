#pragma once

#include <SFML/Graphics.hpp>
#include <maths.h>
#include <memory>
#include <string>
#include <vector>
#include <map>

#define ls LevelSystem

class LevelSystem {
public:
  static void loadLevelFile(const std::string&, float tileSize = 100.0f);
  static void saveLevelFile(const std::string&);
  static void newLevel(size_t width, size_t height, unsigned char fill = ' ',
                        float tileSize = 100.0f);
  static void unload();
  static void render(sf::RenderWindow& window);

  typedef unsigned char Tile;

  enum TILES {
    EMPTY = ' ',
    START = 's',
    END = 'e',
    WALL = 'w',
    ENEMY = 'n',
    WAYPOINT = '+'
  };

  static Tile getTile(sf::Vector2ul);

  // Live-edit a single tile and rebuild sprites. For an editor; not used by
  // loadLevelFile (which builds the whole grid at once for speed).
  static void setTile(sf::Vector2ul pos, Tile t);

  static Tile getTileAt(sf::Vector2f);

  static bool isOnGrid(sf::Vector2f);

  static size_t getWidth();

  static size_t getHeight();

  static sf::Vector2f getTilePosition(sf::Vector2ul);

  static std::vector<sf::Vector2ul> findTiles(Tile);

  static sf::Color getColor(Tile t);

  static void setColor(Tile t, sf::Color c);

  static void setOffset(const sf::Vector2f& _offset);

  static const sf::Vector2f& getOffset();

  static float getTileSize();

  // Loads a tileset texture once; non-empty tiles render textured instead of
  // flat-coloured from then on. Optional. Without calling this, rendering
  // is unchanged.
  static void setTextureMap(const std::string& path);

  // Maps a specific tile character to a specific rect within the texture
  // set by setTextureMap, e.g. for a tile atlas with one sub-image per tile
  // type. Tiles with no mapping fall back to the default (repeat the whole
  // texture) behaviour setTextureMap already had.
  static void setTileRect(Tile t, sf::IntRect rect);

protected:
  static std::unique_ptr<Tile[]> _tiles;
  static size_t _width;
  static size_t _height;
  static sf::Vector2f _offset;

  static std::vector<std::unique_ptr<sf::RectangleShape>> _sprites;

  static sf::Texture _tileTexture;
  static bool _hasTileTexture;
  static std::map<Tile, sf::IntRect> _tileRects;

  static void buildSprites(bool optimise = true);

  static float _tileSize; // for rendering
  static std::map<Tile, sf::Color> _colours;

private:
  LevelSystem() = delete;

  ~LevelSystem() = delete;
};
