#include <iostream>
#include <entt/entt.hpp>

#include <SFML/Graphics.hpp>

struct Position {
    float x, y;
};

entt::handle create_handle(entt::registry &registry) {
    return {registry, registry.create()};
}

int main() {
    entt::registry registry;
    const auto entity = create_handle(registry);
    entity.emplace<Position>(10.f, 20.f);

    sf::RenderWindow window(sf::VideoMode{{800u, 600u}}, "Window");

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }

        window.clear();

        for (const auto &[entity, position]: registry.view<Position>().each()) {
            sf::RectangleShape rectangle{{50.f, 50.f}};
            rectangle.setPosition({position.x, position.y});
            window.draw(rectangle);
        }

        window.display();
    }

    return 0;
}
