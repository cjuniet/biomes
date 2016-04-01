#include "simplexnoise.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

namespace details {
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
  const float scale = 0.002f;
  for (unsigned int y = 0; y < image.getSize().y; ++y) {
    for (unsigned int x = 0; x < image.getSize().x; ++x) {
      const float noise = simplex::fBm2D((float)x + xpos, (float)y + ypos, scale);
      image.setPixel(x, y, get_heatmap_color(clamp(noise, -1, 1)));
    }
  }
}
} // namespace details

struct Tile {
  enum { WIDTH = 64, HEIGHT = 64 };

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

int main(int argc, char* argv[])
{
  sf::RenderWindow window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Biomes", sf::Style::Fullscreen);
  window.setVerticalSyncEnabled(true);
  window.setMouseCursorVisible(false);

  const int nb_tiles_x = (int)std::ceil((float)SCREEN_WIDTH / Tile::WIDTH) + 2;
  const int nb_tiles_y = (int)std::ceil((float)SCREEN_HEIGHT / Tile::HEIGHT) + 2;

  std::vector<std::unique_ptr<Tile>> tiles;
  for (int y = 0; y < nb_tiles_y; ++y) {
    for (int x = 0; x < nb_tiles_x; ++x) {
      auto tile = std::make_unique<Tile>();
      tile->sprite.setPosition((float)(x - 1) * Tile::WIDTH, (float)(y - 1) * Tile::HEIGHT);
      tile->update_image(x - 1, y - 1);
      tiles.push_back(std::move(tile));
    }
  }

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      if (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::Q || event.key.code == sf::Keyboard::Escape)) {
        window.close();
      }
    }

    float speed = 8;
    float offset_x = 0;
    float offset_y = 0;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
      offset_y += speed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
      offset_y -= speed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
      offset_x += speed;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
      offset_x -= speed;
    }

    if (offset_x || offset_y) {
      for (auto& tile : tiles) {
        tile->sprite.move(offset_x, offset_y);

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
    window.display();
  }

  return 0;
}
