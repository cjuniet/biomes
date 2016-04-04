#include "simplexnoise.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <memory>
#include <vector>

namespace details {
const float scale = 0.001f;
const float sea_level = 0.05f;
constexpr float lerp(float a, float b, float x) { return (b - a) * x + a; }
constexpr float unlerp(float a, float b, float x) { return (x - a) / (b - a); }
constexpr float clamp(float x, float min, float max) { return (x > max ? max : (x < min ? min : x)); }

sf::Color lerp(const sf::Color& a, const sf::Color& b, float x)
{
  return sf::Color((sf::Uint8)lerp(a.r, b.r, x), (sf::Uint8)lerp(a.g, b.g, x), (sf::Uint8)lerp(a.b, b.b, x));
}

sf::Color get_heatmap_color(float value)
{
  static sf::Color color[] = {
      sf::Color{0, 0, 128},     // 0 : deeps
      sf::Color{0, 0, 255},     // 1 : shallow
      sf::Color{0, 128, 255},   // 2 : shore
      sf::Color{240, 240, 64},  // 3 : sand
      sf::Color{17, 119, 45},   // 4 : grass
      sf::Color{133, 84, 57},   // 5 : dirt
      sf::Color{128, 128, 128}, // 6 : rock
      sf::Color{192, 192, 192}, // 7 : mountain
      sf::Color{255, 255, 255}  // 8 : snow
  };

  if (value < -0.25f) return lerp(color[0], color[1], unlerp(-1.00f, -0.25f, value));
  if (value < 0.000f) return lerp(color[1], color[2], unlerp(-0.25f, 0.000f, value));
  if (value < 0.050f) return lerp(color[2], color[3], unlerp(0.000f, 0.050f, value));
  if (value < 0.100f) return lerp(color[3], color[4], unlerp(0.050f, 0.100f, value));
  if (value < 0.500f) return lerp(color[4], color[5], unlerp(0.100f, 0.500f, value));
  if (value < 0.850f) return lerp(color[5], color[6], unlerp(0.500f, 0.850f, value));
  if (value < 0.950f) return lerp(color[6], color[7], unlerp(0.850f, 0.950f, value));
  return lerp(color[7], color[8], unlerp(0.950f, 1.000f, value));
}

void generate_noise(sf::Image& image, float xpos, float ypos)
{
  for (unsigned int y = 0; y < image.getSize().y; ++y) {
    for (unsigned int x = 0; x < image.getSize().x; ++x) {
      const float noise = simplex::fBm2D((float)x + xpos, (float)y + ypos, scale);
      image.setPixel(x, y, get_heatmap_color(clamp(noise, -1, 1)));
    }
  }
}
} // namespace details

////////////////////////////////////////////////////////////////

enum DIRECTIONS {
  DIR_UP = 1, DIR_RIGHT = 2, DIR_DOWN = 4, DIR_LEFT = 8,
  DIR_UP_RIGHT = DIR_UP | DIR_RIGHT,
  DIR_UP_LEFT = DIR_UP | DIR_LEFT,
  DIR_DOWN_RIGHT = DIR_DOWN | DIR_RIGHT,
  DIR_DOWN_LEFT = DIR_DOWN | DIR_LEFT
};

struct Tile {
  enum SIZE { WIDTH = 64, HEIGHT = 64 };

  int map_x;
  int map_y;
  sf::Image image;
  sf::Texture texture;
  sf::Sprite sprite;

  Tile() : map_x(0), map_y(0)
  {
    image.create(Tile::WIDTH, Tile::HEIGHT);
    texture.create(Tile::WIDTH, Tile::HEIGHT);
    sprite.setTexture(texture);
  }

  void update_image(int x, int y)
  {
    map_x = x;
    map_y = y;
    details::generate_noise(image, (float)map_x * Tile::WIDTH, (float)map_y * Tile::HEIGHT);
    texture.update(image);
  }
};

////////////////////////////////////////////////////////////////

class Boat : public sf::Drawable
{
  sf::Texture texture;
  sf::Sprite ship;
  sf::Sprite trail;
  int w, h, n, t;
  int nbtrails;
  bool afloat;
  float x, y;
  float _speed;

public:
  Boat(const std::string& asset, int w, int h, int n, float speed = 4.0f)
    : w(w), h(h), n(0), t(0), nbtrails(n - 1), afloat(false), x(0), y(0), _speed(speed)
  {
    texture.loadFromFile(asset);

    ship.setTexture(texture);
    ship.setTextureRect({ 0, 0, w, h });
    ship.setOrigin(w / 2.0f, h / 2.0f);
    ship.scale(0.5f, 0.5f);

    if (nbtrails > 0) {
      trail.setTexture(texture);
      trail.setTextureRect({ w, 0, w, h });
      trail.setOrigin(w / 2.0f, h / 2.0f);
      trail.scale(0.5f, 0.5f);
    }
  }

  float speed() const { return _speed; }
  void speed(float speed) { _speed = speed; }

  void position(float posx, float posy)
  {
    ship.setPosition(posx, posy);
    trail.setPosition(posx, posy);
    x = ship.getPosition().x;
    y = ship.getPosition().y;
  }

  bool move(int direction)
  {
    if (!direction) return false;

    auto oldx = x, oldy = y;
    //FIXME: normalise diagonal movement
    if (direction & DIR_UP)  y -= _speed;
    if (direction & DIR_DOWN) y += _speed;
    if (direction & DIR_RIGHT) x += _speed;
    if (direction & DIR_LEFT) x -= _speed;

    auto c = simplex::fBm2D(x, y, details::scale);
    if (afloat && c >= details::sea_level) {
      x = oldx; y = oldy;
      return false;
    }
    if (!afloat && c <= details::sea_level) {
      afloat = true;
    }

    float angle = 0;
    switch (direction) {
      case DIR_UP: angle = 0; break;
      case DIR_UP_RIGHT: angle = 45; break;
      case DIR_RIGHT: angle = 90; break;
      case DIR_DOWN_RIGHT: angle = 135; break;
      case DIR_DOWN: angle = 180; break;
      case DIR_DOWN_LEFT: angle = -135; break;
      case DIR_LEFT: angle = -90; break;
      case DIR_UP_LEFT: angle = -45; break;
    }
    ship.setRotation(angle);
    trail.setRotation(angle);
    return true;
  }

  void update()
  {
    ++t;
    if (t >= 10) {
      n = (n + 1) % nbtrails;
      trail.setTextureRect({ (1 + n) * w , 0, w, h });
      t = 0;
    }
  }

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override
  {
    if (afloat && nbtrails > 0) target.draw(trail, states);
    target.draw(ship, states);
  }
};

////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
  sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
  sf::RenderWindow window(desktop, "Biomes", sf::Style::Fullscreen);

  window.setVerticalSyncEnabled(true);
  window.setMouseCursorVisible(false);

  const int nb_tiles_x = (int)std::ceil((float)desktop.width / Tile::WIDTH) + 2;
  const int nb_tiles_y = (int)std::ceil((float)desktop.height / Tile::HEIGHT) + 2;

  std::vector<std::unique_ptr<Tile>> tiles;
  for (int y = 0; y < nb_tiles_y; ++y) {
    for (int x = 0; x < nb_tiles_x; ++x) {
      auto tile = std::make_unique<Tile>();
      tile->sprite.setPosition((float)(x - 1) * Tile::WIDTH, (float)(y - 1) * Tile::HEIGHT);
      tile->update_image(x - 1, y - 1);
      tiles.push_back(std::move(tile));
    }
  }

  // Original asset made by Csaba Felvegi under CC-BY 3.0
  // http://opengameart.org/content/ships-with-ripple-effect
  Boat boat("assets/ship.png", 84, 226, 6);
  boat.position(desktop.width / 2.0f, desktop.height / 2.0f);

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Q || event.key.code == sf::Keyboard::Escape) {
          window.close();
        }
      }
    }

    int direction = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      direction |= DIR_UP;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      direction |= DIR_DOWN;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      direction |= DIR_RIGHT;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      direction |= DIR_LEFT;
    }

    if (boat.move(direction)) {
      for (auto& tile : tiles) {
        //FIXME: normalise diagonal movement
        if (direction & DIR_UP) tile->sprite.move(0, boat.speed());
        if (direction & DIR_DOWN) tile->sprite.move(0, -boat.speed());
        if (direction & DIR_RIGHT) tile->sprite.move(-boat.speed(), 0);
        if (direction & DIR_LEFT) tile->sprite.move(boat.speed(), 0);

        const auto pos = tile->sprite.getPosition();
        if (pos.x + Tile::WIDTH < -Tile::WIDTH) {
          tile->sprite.move((float)nb_tiles_x * Tile::WIDTH, 0);
          tile->update_image(tile->map_x + nb_tiles_x, tile->map_y);
        } else if (pos.x > (nb_tiles_x - 1) * Tile::WIDTH) {
          tile->sprite.move(-(float)nb_tiles_x * Tile::WIDTH, 0);
          tile->update_image(tile->map_x - nb_tiles_x, tile->map_y);
        }
        if (pos.y + Tile::HEIGHT < -Tile::HEIGHT) {
          tile->sprite.move(0, (float)nb_tiles_y * Tile::HEIGHT);
          tile->update_image(tile->map_x, tile->map_y + nb_tiles_y);
        } else if (pos.y > (nb_tiles_y - 1) * Tile::HEIGHT) {
          tile->sprite.move(0, -(float)nb_tiles_y * Tile::HEIGHT);
          tile->update_image(tile->map_x, tile->map_y - nb_tiles_y);
        }
      }
    }

    window.clear();
    for (auto& tile : tiles) {
      window.draw(tile->sprite);
    }

    boat.update();
    window.draw(boat);

    window.display();
  }

  return 0;
}
