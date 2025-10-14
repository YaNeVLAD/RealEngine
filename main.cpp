#include <iostream>
#include <thread>
#include <entt/entt.hpp>

#include <SFML/Graphics.hpp>

struct Position {
    float x, y;
};

entt::handle CreateHandle(entt::registry &registry) {
    return {registry, registry.create()};
}

[[maybe_unused]] bool SetActive(sf::RenderWindow *const window, bool active) {
    return window->setActive(active);
}

void RenderThread(sf::RenderWindow *const window, const entt::registry *const registry) {
    SetActive(window, true);

    while (window->isOpen()) {
        window->clear(sf::Color::Black);

        for (const auto &[entity, position]: registry->view<Position>().each()) {
            sf::RectangleShape rectangle{{50.f, 50.f}};
            rectangle.setPosition({position.x, position.y});

            window->draw(rectangle);
        }

        window->display();
    }
}

int main() {
    entt::registry registry;
    const auto entity = CreateHandle(registry);
    entity.emplace<Position>(10.f, 20.f);

    sf::RenderWindow window(sf::VideoMode{{800u, 600u}}, "Window");
    SetActive(&window, false);

    std::jthread renderThread(&RenderThread, &window, &registry);

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
        }
    }

    return EXIT_SUCCESS;
}
