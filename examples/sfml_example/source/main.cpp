#include <iostream>
#include <SFML/Graphics.hpp>
#include <Engine.h>
#include <opengl/RenderingSystemOpenGL.h>
#include <Camera.h>
#include <Entity.h>
#include <Light.h>
#include <Transform.h>
#include <AssimpImporter.h>
#include <Renderables.h>
#include <Model.h>
#include <GameTimer.h>

int main()
{
  sf::RenderWindow window(sf::VideoMode(1024, 768), "My window");
  auto engine = std::make_unique<fly::Engine>();
  auto rs = std::make_shared<fly::RenderingSystemOpenGL>();
  engine->addSystem(rs);
  rs->init(fly::Vec2i({ static_cast<int>(window.getSize().x), static_cast<int>( window.getSize().y)}));
  auto cam_entity = engine->getEntityManager()->createEntity();
  cam_entity->addComponent(std::make_shared<fly::Camera>(glm::vec3(0.f, 3.f, 0.f), glm::vec3(glm::radians(270.f), 0.f, 0.f)));
  auto light = engine->getEntityManager()->createEntity();
  std::vector<float> csm_distances = { 5.f, 10.f, 20.f, 50.f };
  light->addComponent(std::make_shared<fly::DirectionalLight>(glm::vec3(1.f), csm_distances));
  light->addComponent(std::make_shared <fly::Transform>(glm::vec3(10.f, 10.f, 0.f)));
  auto importer = std::make_unique<fly::AssimpImporter>();
  auto sponza_model = importer->loadModel("assets/sponza/sponza.obj");
  auto sponza_entity = engine->getEntityManager()->createEntity();
  sponza_entity->addComponent(sponza_model);
  sponza_entity->addComponent(std::make_shared<fly::StaticModelRenderable>());
  sponza_entity->addComponent(std::make_shared<fly::Transform>(glm::vec3(0.f), glm::vec3(0.01f)));
  fly::GameTimer timer;
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      else if (event.type == sf::Event::Resized) {
         auto size = window.getSize();
         rs->onResize(glm::ivec2(size.x, size.y));
      }
    }
    timer.tick();
    engine->update(timer.getTimeSeconds(), timer.getDeltaTimeSeconds());
    window.display();
  }
  return 0;
}