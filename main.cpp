#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <sstream>

const int WIDTH = 1200;
const int HEIGHT = 600;
const float GRAVITY = 0.04f;
const float TRAIL_FADE = 0.94f;

struct LightFlash {
    sf::Vector2f position;
    sf::Color color;
    float radius;
    float maxRadius;
    float life;
    float maxLife;
};

struct TrailPoint {
    sf::Vector2f position;
    sf::Color color;
    float size;
};

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float size;
    float life;
    float maxLife;
    std::vector<TrailPoint> trail;
    int layer;
    float timeSinceExpand = 0.0f;
    bool isFalling = false;
    bool hasCreatedTrail = false;
    bool isFlowerPetal = false;
    float petalAngle = 0.0f;
    float petalRotationSpeed = 0.0f;
};

class Firework {
private:
    std::vector<Particle> particles;
    bool exploded = false;
    sf::Vector2f explosionPos;
    sf::Color baseColor;
    int currentLayer = -1;
    float layerTimer = 0.0f;
    bool allLayersDone = false;
    bool isTextMode = false;
    std::string textContent;
    int explosionCount = 0;
    bool rocketAlive = true;
    float timeToExplode = 0.0f;
    bool hasReachedPeak = false;
    float targetHeight = 0.0f;
    int maxLayers = 3;
    std::vector<LightFlash> lightFlashes;

    sf::Color getGradientColor(float ratio, sf::Color start, sf::Color end) {
        return sf::Color(
            start.r + (end.r - start.r) * ratio,
            start.g + (end.g - start.g) * ratio,
            start.b + (end.b - start.b) * ratio
        );
    }

    sf::Color getVariedColor(sf::Color base, int layer, float lifeRatio) {
        sf::Color c2 = layer % 2 == 0 ? sf::Color(255, 255, 200) : sf::Color(200, 255, 255);
        sf::Color grad = getGradientColor(1.0f - lifeRatio, base, c2);
        int variation = 20 + layer * 10;
        return sf::Color(
            std::min(255, std::max(0, static_cast<int>(grad.r) + (rand() % (variation * 2) - variation))),
            std::min(255, std::max(0, static_cast<int>(grad.g) + (rand() % (variation * 2) - variation))),
            std::min(255, std::max(0, static_cast<int>(grad.b) + (rand() % (variation * 2) - variation)))
        );
    }

    void createSparkles(int count, float baseSize = 1.5f) {
        for (int i = 0; i < count; ++i) {
            Particle p;
            p.position = explosionPos;
            float angle = rand() % 628 / 100.0f;
            float speed = 2.0f + rand() % 60 / 10.0f;
            p.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);
            p.color = sf::Color(255, 255, 220);
            p.size = baseSize + (rand() % 40) / 10.0f;
            p.maxLife = 20 + rand() % 10;
            p.life = p.maxLife;
            p.layer = -6;
            particles.push_back(p);
        }
    }

    void createPeachBlossomPetal(float angle, float distance, float sizeMultiplier = 1.0f) {
        Particle petal;
        petal.position = explosionPos;

        float speed = 0.5f + (rand() % 20) / 100.0f;
        petal.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);

        int pinkVariation = rand() % 40;
        petal.color = sf::Color(
            255,
            180 + pinkVariation,
            200 + pinkVariation,
            240
        );

        petal.size = (3.0f + (rand() % 20) / 10.0f) * sizeMultiplier;
        petal.maxLife = 200 + rand() % 100;
        petal.life = petal.maxLife;
        petal.layer = 4;
        petal.isFlowerPetal = true;
        petal.petalAngle = angle;
        petal.petalRotationSpeed = (rand() % 10 - 5) / 20.0f;

        particles.push_back(petal);
    }

    void createApricotBlossomPetal(float angle, float distance, float sizeMultiplier = 1.0f) {
        Particle petal;
        petal.position = explosionPos;

        float speed = 0.6f + (rand() % 25) / 100.0f;
        petal.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);

        int yellowVariation = rand() % 50;
        petal.color = sf::Color(
            255,
            230 + yellowVariation,
            150 - yellowVariation / 2,
            240
        );

        petal.size = (2.5f + (rand() % 15) / 10.0f) * sizeMultiplier;
        petal.maxLife = 180 + rand() % 80;
        petal.life = petal.maxLife;
        petal.layer = 5;
        petal.isFlowerPetal = true;
        petal.petalAngle = angle;
        petal.petalRotationSpeed = (rand() % 10 - 5) / 25.0f;

        particles.push_back(petal);
    }

    void explodeLayer(int layer) {
        currentLayer = layer;

        int count = 0;
        float speed = 0;
        float psize = 0;

        if (layer == 0) {
            count = 15;
            speed = 2.5f;
            psize = 1.0f;
        } else if (layer == 1) {
            count = 20;
            speed = 2.0f;
            psize = 0.5f;
        } else if (layer == 2) {
            count = 25;
            speed = 2.5f;
            psize = 2.0f;
        } else if (layer == 3) {
            count = 25;
            speed = 2.0f;
            psize = 2.5f;
        } else {
            count = 10;
            speed = 2.5f;
            psize = 2.0f;
        }

        for (int i = 0; i < count; ++i) {
            Particle p;
            p.position = explosionPos;

            float angle = (i * 2.0f * 3.14159f / count) + (explosionCount * 0.3f) + (rand() % 80 / 500.0f);
            float velVar = 1.0f + (rand() % 50) / 100.0f;

            float dirX = cos(angle) * speed * velVar;
            float dirY = sin(angle) * speed * velVar;

            p.velocity = sf::Vector2f(dirX, dirY);
            p.color = getVariedColor(baseColor, layer, 1.0f);
            p.size = psize * (2.0f + (rand() % 40) / 100.0f);

            p.maxLife = 100.0f + layer * 10 + rand() % 40;
            p.life = p.maxLife;
            p.layer = layer;
            p.isFalling = false;
            p.timeSinceExpand = 0.0f;
            p.hasCreatedTrail = false;
            particles.push_back(p);
        }

        createSparkles(layer >= 3 ? 35 : 25, 1.0f);

        LightFlash flash;
        flash.position = explosionPos;
        flash.color = baseColor;
        flash.radius = 10.0f;
        flash.maxRadius = 80.0f;
        flash.maxLife = 25.0f;
        flash.life = flash.maxLife;
        lightFlashes.push_back(flash);

        if (layer == 3) {
            explosionCount++;
            if (explosionCount >= 2) {
                allLayersDone = true;
            } else {
                layerTimer = 0.7f;
                currentLayer = -1;
            }
        }
    }

    void createFlowerEffectForText(bool isApricot = false) {
        int petalCount = 60;

        for (int i = 0; i < petalCount; ++i) {
            float angle = (i * 2.0f * 3.14159f / petalCount);
            float distance = 50 + rand() % 100;

            if (isApricot) {
                createApricotBlossomPetal(angle, distance, 1.2f);
            } else {
                createPeachBlossomPetal(angle, distance, 1.0f);
            }
        }

        for (int i = 0; i < 20; ++i) {
            Particle center;
            center.position = explosionPos;
            float angle = rand() % 628 / 100.0f;
            float speed = 0.1f + (rand() % 10) / 100.0f;
            center.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);

            if (isApricot) {
                center.color = sf::Color(255, 200, 50, 255);
            } else {
                center.color = sf::Color(255, 100, 150, 255);
            }

            center.size = 2.0f + (rand() % 10) / 5.0f;
            center.maxLife = 150 + rand() % 50;
            center.life = center.maxLife;
            center.layer = 6;
            particles.push_back(center);
        }

        LightFlash flash;
        flash.position = explosionPos;
        flash.color = isApricot ? sf::Color(255, 220, 150) : sf::Color(255, 180, 200);
        flash.radius = 10.0f;
        flash.maxRadius = 120.0f;
        flash.maxLife = 40.0f;
        flash.life = flash.maxLife;
        lightFlashes.push_back(flash);

        createSparkles(40, 1.0f);
    }

public:
    Firework(sf::Color mainColor, bool textMode = false, const std::string& txt = "")
        : baseColor(mainColor), isTextMode(textMode), textContent(txt) {

        if (textMode) {
            explosionPos = sf::Vector2f(WIDTH / 2.0f, HEIGHT / 2.0f);

            if (textContent == "2026") {
                createFlowerEffectForText(true);
            } else {
                createFlowerEffectForText(false);
            }

            allLayersDone = false;
            createTextParticles();
        } else {
            float startX = 120 + rand() % (WIDTH - 240);
            explosionPos = sf::Vector2f(startX, HEIGHT + 15);
            targetHeight = HEIGHT * (0.2f + (rand() % 30) / 100.0f);

            Particle rocket;
            rocket.position = explosionPos;
            float angle = 3.14159f * 1.5f;
            angle += (rand() % 120 - 60) / 500.0f;
            float speed = 7.0f + (rand() % 40) / 10.0f;
            rocket.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);
            rocket.color = mainColor;
            rocket.size = 5.0f;
            rocket.life = rocket.maxLife = 500;
            rocket.layer = -1;
            rocket.hasCreatedTrail = false;
            particles.push_back(rocket);

            rocketAlive = true;
            hasReachedPeak = false;
            timeToExplode = 6.0f;
        }
    }

    void createTextParticles() {
        int particleCount = 250;
        for (int i = 0; i < particleCount; ++i) {
            Particle p;
            p.position = explosionPos;
            float angle = rand() % 628 / 100.0f;
            float distance = 100 + rand() % 200;
            float speed = 0.8f + (rand() % 40) / 100.0f;
            p.velocity = sf::Vector2f(cos(angle) * speed, sin(angle) * speed);

            if (textContent == "2026") {
                p.color = sf::Color(
                    255,
                    220 + rand() % 35,
                    150 + rand() % 50,
                    220
                );
            } else {
                p.color = sf::Color(
                    255,
                    180 + rand() % 50,
                    200 + rand() % 55,
                    220
                );
            }

            p.size = 3.0f + (rand() % 30) / 10.0f;
            p.maxLife = 250;
            p.life = p.maxLife;
            p.layer = 3;
            p.hasCreatedTrail = false;

            if (rand() % 3 == 0) {
                p.isFlowerPetal = true;
                p.petalAngle = angle;
                p.petalRotationSpeed = (rand() % 10 - 5) / 30.0f;
            }

            particles.push_back(p);
        }
    }

    void explode() {
        if (exploded) return;

        exploded = true;
        explosionCount = 0;
        rocketAlive = false;

        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.layer == -1; }), particles.end());

        explodeLayer(0);
        layerTimer = 0.15f;
    }

    bool isDone() const {
        return allLayersDone && particles.empty() && lightFlashes.empty();
    }

    void update(float dt) {
        for (auto& flash : lightFlashes) {
            flash.life -= dt * 40.0f;
            flash.radius += (flash.maxRadius - flash.radius) * 0.1f;
        }

        lightFlashes.erase(std::remove_if(lightFlashes.begin(), lightFlashes.end(),
            [](const LightFlash& f) { return f.life <= 0; }), lightFlashes.end());

        if (isTextMode) {
            for (auto& p : particles) {
                p.position += p.velocity;
                p.velocity.y += GRAVITY * 0.03f;
                p.velocity *= 0.995f;
                p.life -= dt * 35.0f;

                if (p.isFlowerPetal) {
                    p.petalAngle += p.petalRotationSpeed * dt;
                    p.velocity.x += sin(p.petalAngle * 2.0f) * 0.02f;
                }

                if (p.isFlowerPetal && rand() % 4 == 0 && p.life > 150) {
                    TrailPoint t{p.position, p.color, p.size * 0.8f};
                    t.color.a = 100;
                    p.trail.push_back(t);
                }

                if (p.trail.size() > 8) p.trail.erase(p.trail.begin());

                for (auto& t : p.trail) {
                    t.color.a = static_cast<sf::Uint8>(t.color.a * 0.92f);
                    t.size *= 0.95f;
                }
            }

            particles.erase(std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.life < 20; }), particles.end());

            if (particles.empty()) allLayersDone = true;
            return;
        }

        if (rocketAlive && !particles.empty()) {
            Particle& rocket = particles[0];
            rocket.position += rocket.velocity;
            rocket.velocity.y += GRAVITY * 0.3f;

            if (rand() % 2 == 0) {
                TrailPoint t{rocket.position, rocket.color, rocket.size * 0.8f};
                t.color.a = 220;
                rocket.trail.push_back(t);
            }

            bool shouldExplode = false;

            if (rocket.position.y <= targetHeight) {
                shouldExplode = true;
            } else if (rocket.velocity.y > 0) {
                shouldExplode = true;
            }

            timeToExplode -= dt;
            if (timeToExplode <= 0) {
                shouldExplode = true;
            }

            if (shouldExplode) {
                explosionPos = rocket.position;
                explode();
            }

            if (rocket.trail.size() > 25) rocket.trail.erase(rocket.trail.begin());

            for (auto& t : rocket.trail) {
                t.color.a = static_cast<sf::Uint8>(t.color.a * 0.92f);
                t.size *= 0.94f;
            }
        }

        if (exploded && layerTimer > 0 && !allLayersDone && currentLayer == -1) {
            layerTimer -= dt;
            if (layerTimer <= 0) {
                explodeLayer(0);
                layerTimer = 0.15f;
            }
        }

        for (auto& p : particles) {
            if (p.layer == -1) continue;

            p.timeSinceExpand += dt;
            float lifeRatio = p.life / p.maxLife;

            if (p.layer >= 0 && p.layer <= 3) {
                p.color = getVariedColor(baseColor, p.layer, lifeRatio);
            }

            if (p.layer >= 2 || p.timeSinceExpand > 0.4f) {
                if (!p.isFalling && p.timeSinceExpand > 0.4f) {
                    p.isFalling = true;
                    p.velocity.x *= 0.75f;
                }

                if (p.isFalling) {
                    p.velocity.y += GRAVITY * 2.2f;
                    p.velocity.x *= 0.98f;

                    if (!p.hasCreatedTrail || rand() % 2 == 0) {
                        TrailPoint t{p.position, p.color, p.size * 1.2f};
                        t.color.a = static_cast<sf::Uint8>(std::max(60.0f, lifeRatio * 240));
                        p.trail.push_back(t);
                        p.hasCreatedTrail = true;
                    }
                } else {
                    p.velocity.y += GRAVITY * 0.7f;
                }
            } else if (p.layer == -2) {
                p.velocity.y += GRAVITY * 0.4f;
                p.velocity *= 0.97f;

                if (rand() % 3 == 0) {
                    TrailPoint t{p.position, p.color, p.size * 0.8f};
                    t.color.a = static_cast<sf::Uint8>(lifeRatio * 180);
                    p.trail.push_back(t);
                }
            }

            p.position += p.velocity;
            p.life *= 0.992f;

            if (p.trail.size() > 20) p.trail.erase(p.trail.begin());

            for (auto& t : p.trail) {
                t.color.a = static_cast<sf::Uint8>(t.color.a * TRAIL_FADE);
                t.size *= 0.93f;
            }

            if (rand() % 10 == 0) {
                p.hasCreatedTrail = false;
            }
        }

        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) {
                return p.life < 15 ||
                       p.position.y > HEIGHT + 200 ||
                       p.position.x < -200 ||
                       p.position.x > WIDTH + 200;
            }), particles.end());
    }

    void draw(sf::RenderWindow& window) const {
        for (const auto& flash : lightFlashes) {
            if (flash.life > 0) {
                float ratio = flash.life / flash.maxLife;
                sf::Color flashColor = flash.color;
                flashColor.a = static_cast<sf::Uint8>(50 * ratio);

                for (int i = 0; i < 3; ++i) {
                    float radius = flash.radius * (0.7f + i * 0.2f);
                    sf::CircleShape glow(radius);
                    glow.setFillColor(sf::Color(flashColor.r, flashColor.g, flashColor.b,
                                               static_cast<sf::Uint8>(flashColor.a * (0.7f - i * 0.2f))));
                    glow.setOrigin(radius, radius);
                    glow.setPosition(flash.position);
                    window.draw(glow, sf::BlendAdd);
                }
            }
        }

        for (const auto& p : particles) {
            for (const auto& t : p.trail) {
                if (t.color.a > 15) {
                    sf::CircleShape c(t.size);
                    c.setFillColor(t.color);
                    c.setOrigin(t.size / 2, t.size / 2);
                    c.setPosition(t.position);

                    if (p.isFlowerPetal) {
                        c.setPointCount(5);
                        c.setRotation(p.petalAngle * 180 / 3.14159f);
                    } else {
                        c.setPointCount(12);
                    }

                    window.draw(c, sf::BlendAdd);
                }
            }
        }

        for (const auto& p : particles) {
            if (p.life > 15) {
                sf::CircleShape c(p.size);
                sf::Color col = p.color;
                col.a = static_cast<sf::Uint8>(std::min(255.0f, p.life * (p.layer == -2 ? 2.2f : 1.5f)));
                c.setFillColor(col);
                c.setOrigin(p.size / 2, p.size / 2);
                c.setPosition(p.position);

                if (p.isFlowerPetal) {
                    c.setPointCount(5);
                    c.setRotation(p.petalAngle * 180 / 3.14159f);
                    c.setScale(1.0f, 0.7f);
                } else if (p.layer == -2) {
                    c.setPointCount(7);
                } else if (p.layer >= 4 && p.layer <= 6) {
                    c.setPointCount(6);
                } else {
                    c.setPointCount(14);
                }

                window.draw(c, sf::BlendAdd);
            }
        }
    }
};

// Hàm chuyển int sang wstring
std::wstring intToWString(int value) {
    std::wstringstream wss;
    wss << value;
    return wss.str();
}

int main() {
    // Set locale để hỗ trợ Unicode
    std::locale::global(std::locale(""));

    srand(time(nullptr));
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), L"Chúc Mừng Năm Mới 2026 - Pháo Hoa Rực Rỡ");
    window.setFramerateLimit(60);

    // Load font TIẾNG VIỆT - quan trọng!
    sf::Font font;
    bool fontLoaded = false;

    // Ưu tiên font hỗ trợ tiếng Việt
    const char* fontPaths[] = {
        "arial.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/tahoma.ttf",       // Tahoma hỗ trợ tốt tiếng Việt
        "C:/Windows/Fonts/segoeui.ttf",      // Segoe UI (Windows 10+)
        "C:/Windows/Fonts/verdana.ttf",      // Verdana
        "C:/Windows/Fonts/times.ttf",
        "NotoSans-Regular.ttf",              // Font Google Noto Sans
        "Roboto-Regular.ttf",                // Font Roboto
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };

    for (const char* path : fontPaths) {
        if (font.loadFromFile(path)) {
            fontLoaded = true;
            std::cout << "Da tai font: " << path << std::endl;
            break;
        }
    }

    if (!fontLoaded) {
        std::cout << "Loi: Khong the tai font nao!" << std::endl;
        return -1;
    }

    // Load âm thanh (nếu có)
    sf::SoundBuffer boomBuffer;
    sf::Sound boomSound;
    bool soundLoaded = false;

    const char* soundPaths[] = {
        "firework_boom.wav",
        "boom.wav",
        "explosion.wav"
    };

    for (const char* path : soundPaths) {
        if (boomBuffer.loadFromFile(path)) {
            boomSound.setBuffer(boomBuffer);
            soundLoaded = true;
            std::cout << "Da tai am thanh: " << path << std::endl;
            break;
        }
    }

    if (!soundLoaded) {
        std::cout << "Khong tim thay file am thanh." << std::endl;
    }

    std::vector<Firework> fireworks;
    std::vector<sf::Color> colors = {
        sf::Color(255, 80, 120), sf::Color(80, 255, 180), sf::Color(120, 150, 255),
        sf::Color(255, 220, 80), sf::Color(255, 100, 200), sf::Color(100, 255, 255),
        sf::Color(255, 150, 50), sf::Color(180, 80, 255), sf::Color(80, 220, 255),
        sf::Color(255, 80, 180), sf::Color(180, 255, 80), sf::Color(80, 180, 255),
        sf::Color(255, 120, 80), sf::Color(120, 255, 80), sf::Color(80, 120, 255)
    };

    sf::Clock deltaClock;
    sf::Clock spawnClock;
    sf::Clock blinkClock;
    float spawnTimer = 0.6f;
    int textMode = 0;
    bool autoFire = true;

    float blinkTime = 0.0f;

    // Thêm pháo hoa ban đầu
    for (int i = 0; i < 4; i++) {
        fireworks.emplace_back(colors[rand() % colors.size()]);
    }

    // Chuẩn bị văn bản TIẾNG VIỆT cho câu đối
    std::wstring coupletLeftText = L"XUÂN\nAN\nKHANG\nĐỊNH\nCÁT";
    std::wstring coupletRightText = L"TÂN\nNIÊN\nTHỊNH\nVƯỢNG\nPHÁT\nTÀI";

    while (window.isOpen()) {
        float dt = deltaClock.restart().asSeconds();
        blinkTime += dt;

        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed)
                window.close();
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Return) {
                    fireworks.clear();
                    textMode = (textMode + 1) % 3;

                    if (textMode == 1) {
                        fireworks.emplace_back(sf::Color(255, 180, 200), true, "HAPPY NEW YEAR");

                        if (soundLoaded) {
                            boomSound.play();
                        }

                        autoFire = false;
                    } else if (textMode == 2) {
                        fireworks.emplace_back(sf::Color(255, 220, 150), true, "2026");

                        if (soundLoaded) {
                            boomSound.play();
                        }
                    } else {
                        autoFire = true;
                        for (int i = 0; i < 4; i++) {
                            fireworks.emplace_back(colors[rand() % colors.size()]);
                        }
                    }
                } else if (e.key.code == sf::Keyboard::Space) {
                    int count = 2 + rand() % 4;
                    for (int i = 0; i < count; i++) {
                        fireworks.emplace_back(colors[rand() % colors.size()]);

                        if (soundLoaded && rand() % 3 == 0) {
                            boomSound.play();
                        }
                    }
                } else if (e.key.code == sf::Keyboard::A) {
                    autoFire = !autoFire;
                } else if (e.key.code == sf::Keyboard::R) {
                    fireworks.clear();
                    textMode = 0;
                    autoFire = true;
                    for (int i = 0; i < 3; i++) {
                        fireworks.emplace_back(colors[rand() % colors.size()]);
                    }
                }
            }
        }

        if (autoFire && textMode == 0) {
            spawnTimer -= dt;
            if (spawnTimer <= 0) {
                int count = 1 + rand() % 4;
                for (int i = 0; i < count; i++) {
                    fireworks.emplace_back(colors[rand() % colors.size()]);

                    if (soundLoaded && rand() % 4 == 0) {
                        boomSound.play();
                    }
                }
                spawnTimer = 0.4f + (rand() % 20) / 10.0f;
            }
        }

        for (auto& f : fireworks) {
            f.update(dt);
        }

        fireworks.erase(std::remove_if(fireworks.begin(), fireworks.end(),
            [](const Firework& f) { return f.isDone(); }), fireworks.end());

        if (fireworks.size() > 20) {
            fireworks.erase(fireworks.begin(), fireworks.begin() + (fireworks.size() - 20));
        }

        window.clear(sf::Color(10, 10, 30));

        sf::VertexArray bg(sf::Quads, 4);
        bg[0] = {{0,0}, sf::Color(15,15,45)};
        bg[1] = {{WIDTH,0}, sf::Color(15,15,45)};
        bg[2] = {{WIDTH,HEIGHT}, sf::Color(5,5,20)};
        bg[3] = {{0,HEIGHT}, sf::Color(5,5,20)};
        window.draw(bg);

        static std::vector<sf::Vector2f> stars;
        static std::vector<float> starSizes;
        static std::vector<float> starSpeeds;
        static bool starsInitialized = false;
        if (!starsInitialized) {
            for (int i = 0; i < 200; i++) {
                stars.push_back(sf::Vector2f(rand() % WIDTH, rand() % HEIGHT));
                starSizes.push_back(0.8f + (rand() % 25) / 10.0f);
                starSpeeds.push_back(0.5f + (rand() % 10) / 10.0f);
            }
            starsInitialized = true;
        }

        static float starTime = 0.0f;
        starTime += dt;
        for (size_t i = 0; i < stars.size(); i++) {
            float brightness = 0.6f + 0.4f * sin(starTime * starSpeeds[i] + i * 0.1f);
            sf::CircleShape star(starSizes[i] * brightness);
            star.setFillColor(sf::Color(230, 240, 255, static_cast<sf::Uint8>(120 * brightness)));
            star.setPosition(stars[i]);
            window.draw(star);
        }

        // VẼ CÂU ĐỐI TIẾNG VIỆT CÓ DẤU
        if (fontLoaded) {
            // Câu đối bên trái
            sf::Text coupletLeft;
            coupletLeft.setFont(font);
            coupletLeft.setString(coupletLeftText);  // Dùng wstring
            coupletLeft.setCharacterSize(36);
            coupletLeft.setFillColor(sf::Color(220, 40, 40));
            coupletLeft.setOutlineColor(sf::Color(255, 225, 50));
            coupletLeft.setOutlineThickness(3.0f);
            coupletLeft.setStyle(sf::Text::Bold);
            coupletLeft.setRotation(-8.0f);
            coupletLeft.setPosition(120, HEIGHT/2 - 120);
            window.draw(coupletLeft);

            // Câu đối bên phải
            sf::Text coupletRight;
            coupletRight.setFont(font);
            coupletRight.setString(coupletRightText);  // Dùng wstring
            coupletRight.setCharacterSize(36);
            coupletRight.setFillColor(sf::Color(220, 40, 40));
            coupletRight.setOutlineColor(sf::Color(255, 225, 50));
            coupletRight.setOutlineThickness(3.0f);
            coupletRight.setStyle(sf::Text::Bold);
            coupletRight.setRotation(8.0f);
            coupletRight.setPosition(WIDTH - 220, HEIGHT/2 - 120);
            window.draw(coupletRight);
        }

        for (auto& f : fireworks) {
            f.draw(window);
        }

        if (fontLoaded && (textMode == 1 || textMode == 2)) {
            float blinkAlpha = 200 + 55 * sin(blinkTime * 3.0f);
            float outlineAlpha = 150 + 105 * sin(blinkTime * 3.0f + 1.0f);

            if (textMode == 1) {
                sf::Text textDisplay;
                textDisplay.setFont(font);
                textDisplay.setString("HAPPY NEW YEAR");
                textDisplay.setCharacterSize(64);

                textDisplay.setFillColor(sf::Color(255, 255, 255, (sf::Uint8)blinkAlpha));
                textDisplay.setOutlineColor(sf::Color(255, 182, 193, (sf::Uint8)outlineAlpha));
                textDisplay.setOutlineThickness(5.0f);
                textDisplay.setStyle(sf::Text::Bold);

                sf::FloatRect bounds = textDisplay.getLocalBounds();
                textDisplay.setOrigin(bounds.left + bounds.width / 2.0f,
                                     bounds.top + bounds.height / 2.0f);
                textDisplay.setPosition(WIDTH / 2.0f, HEIGHT / 2.0f);

                window.draw(textDisplay);
            } else if (textMode == 2) {
                sf::Text textDisplay;
                textDisplay.setFont(font);
                textDisplay.setString("2026");
                textDisplay.setCharacterSize(180);

                textDisplay.setFillColor(sf::Color(255, 255, 180, (sf::Uint8)blinkAlpha));
                textDisplay.setOutlineColor(sf::Color(255, 165, 0, (sf::Uint8)outlineAlpha));
                textDisplay.setOutlineThickness(8.0f);
                textDisplay.setStyle(sf::Text::Bold);

                sf::FloatRect bounds = textDisplay.getLocalBounds();
                textDisplay.setOrigin(bounds.left + bounds.width / 2.0f,
                                     bounds.top + bounds.height / 2.0f);
                textDisplay.setPosition(WIDTH / 2.0f, HEIGHT / 2.0f);

                window.draw(textDisplay);
            }
        }

        if (fontLoaded) {
    float guideAlpha = 180 + 40 * sin(blinkTime * 2.0f);

    // Hướng dẫn (có thể dùng string không dấu cho đơn giản)
    sf::Text guide1("ENTER: chuyen che do | SPACE: ban them | A: bat/tat tu dong | R: reset", font, 18);
    guide1.setFillColor(sf::Color(200, 220, 255, (sf::Uint8)guideAlpha));
    guide1.setPosition(20, HEIGHT - 80);
    window.draw(guide1);

    // Tạo chuỗi trạng thái
    std::string statusText = "Che do: ";
    if (textMode == 0) {
        statusText += "Phao hoa (4 lop no) [" + std::to_string(fireworks.size()) + "]";
    }
    else if (textMode == 1) statusText += "HAPPY NEW YEAR (Hoa Dao)";
    else statusText += "2026 (Hoa Mai)";

    sf::Text guide2(statusText, font, 18);
    guide2.setFillColor(sf::Color(255, 200, 100, 250));
    guide2.setPosition(20, HEIGHT - 55);
    window.draw(guide2);

   /* std::string autoStatus = "Tu dong: " + std::string(autoFire ? "BAT" : "TAT");
    sf::Text guide3(autoStatus, font, 18);
    guide3.setFillColor(autoFire ? sf::Color(100, 255, 100, 250) : sf::Color(255, 100, 100, 250));
    guide3.setPosition(WIDTH - 200, HEIGHT - 75);
    window.draw(guide3);

    std::string soundStatus = "Am thanh: " + std::string(soundLoaded ? "CO" : "KHONG");
    sf::Text guide4(soundStatus, font, 18);
    guide4.setFillColor(soundLoaded ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100));
    guide4.setPosition(WIDTH - 200, HEIGHT - 100);
    window.draw(guide4);

    // Phần thông tin thêm
    sf::Text guide5("Chu va so hien o GIUA MAN HINH - Co hieu ung nhap nhay + toa sang", font, 16);
    guide5.setFillColor(sf::Color(180, 200, 255, 200));
    guide5.setPosition(20, HEIGHT - 50);
    window.draw(guide5);

    // Câu đối vẫn dùng wstring cho tiếng Việt có dấu
    // Còn hướng dẫn dùng string không dấu cho đơn giản
    sf::Text guide6("HAI BEN LA CAU DOI TET: XUAN AN KHANG DINH CAT - TAN NIEN THINH VUONG PHAT TAI", font, 16);
    guide6.setFillColor(sf::Color(255, 100, 100, 200));
    guide6.setPosition(20, HEIGHT - 30);
    window.draw(guide6);*/
}

        window.display();
    }

    return 0;
}
