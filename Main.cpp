#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window/Joystick.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <string>
#include <chrono>
#include <thread>
#define M_PI 3.14159

int rad = 20;
int w = 1960;
int h = 1160;
int magsNotClips = 30;
int totalammo = 210;
int addCapacity = 30;
float globalTimeScale = 1.0f;



class Ammo
{
    sf::Vector2f pos;
    sf::Vector2f velocity;
    sf::Sprite sprite;
    static sf::Texture bullet;

public:
    Ammo(float pos_x, float pos_y, float dim_l, float dim_b, sf::Vector2f vel)

    {
        pos.x = pos_x;
        pos.y = pos_y;
        velocity = vel;

        sf::Image image;

        if (!image.loadFromFile("bullet.png")) {
            throw std::runtime_error("Failed to load image from file: bullet.png");
        }

        image.setPixel(0, 0, sf::Color::White);

        if (!bullet.loadFromImage(image)) {
            throw std::runtime_error("Failed to load texture from image");
        }

        sprite.setTexture(bullet);
        sprite.setScale(dim_l / bullet.getSize().x, dim_b / bullet.getSize().y);
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
        sprite.setPosition(pos);

        sf::Vector2f direction = velocity;

        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        direction.x /= length;
        direction.y /= length;

        float rotation = std::atan2(direction.y, direction.x);

        sprite.setRotation(rotation * 180 / M_PI);

    }

    void update()
    {
        pos += velocity;
        sprite.setPosition(pos);
    }

    void render(sf::RenderWindow& wind)
    {
        wind.draw(sprite);
    }

    sf::Vector2f getPosition() const {
        return pos;
    }

    sf::FloatRect getBounds() const
    {
        return sprite.getGlobalBounds();
    }
};

class Player
{
    sf::Vector2f pos;
    sf::CircleShape s;
    sf::Vector2f currentDirection = { 0.0f, 0.0f };
    float dashSpeed = 450.0f;
    bool isDashing = false;
    sf::Clock dashClock;
    float dashCooldown = 4.0f;
    float dashDuration = 0.2f;
    float slowMotionScale = 0.2f;

public:
    float health;
    Player(float pos_x, float pos_y, int initialHealth)
        : pos(pos_x, pos_y), health(initialHealth) {
        s.setFillColor(sf::Color::Yellow);
        s.setRadius(rad);
        s.setOrigin(rad, rad); // set origin to the center
        s.setPosition(pos);

    }

    void render(sf::RenderWindow& wind)
    {
        wind.draw(s);
    }

    void move(float dx, float dy) {
        pos.x += dx;
        pos.y += dy;
        s.setPosition(pos);

        if (dx != 0 || dy != 0) {
            currentDirection = sf::Vector2f(dx, dy);
            float length = std::sqrt(currentDirection.x * currentDirection.x + currentDirection.y * currentDirection.y);
            if (length != 0) {
                currentDirection /= length;
            }
        }
    }

    void dash() {
        if (canDash()) {
            isDashing = true;
            dashClock.restart(); // Start the dash timer
            pos += currentDirection * dashSpeed;
            s.setPosition(pos);
            globalTimeScale = slowMotionScale; // Apply slow-motion effect
        }
    }

    bool canDash() {
        return dashClock.getElapsedTime().asSeconds() >= dashCooldown;
    }

    void update(float dt) {
        if (isDashing) {
            if (dashClock.getElapsedTime().asSeconds() >= dashDuration) {
                isDashing = false;
                globalTimeScale = 1.0f; // Reset time scale to normal
            }
        }

        // Update position based on the current time scale
        move(currentDirection.x * dt, currentDirection.y * dt);
    }

    sf::CircleShape& getShape()
    {
        return s;
    }

    void setHealth(int newhealth) {
        health = newhealth;
    }

    void checkBoundaryCollision()
    {
        if (pos.x - rad < 0) pos.x = rad;
        if (pos.x + rad > w) pos.x = w - rad;
        if (pos.y - rad < 0) pos.y = rad;
        if (pos.y + rad > h) pos.y = h - rad;
        s.setPosition(pos);
    }

    bool checkCollision(const sf::RectangleShape& obstacle) const {
        sf::FloatRect obstacleBounds = obstacle.getGlobalBounds();
        sf::FloatRect playerBounds(pos.x - rad, pos.y - rad, 2 * rad, 2 * rad);

        return playerBounds.intersects(obstacleBounds);
    }


    bool isAlive() {
        if (health > 0) {
            return true;
        }
    }

    int getHealth() {
        return health;
    }

    void takeDamage(float damage, sf::RenderWindow& wind) {
        health -= damage;
        if (health <= 0) {
            wind.close();
        }
    }

    float calculateAngle(const sf::Vector2f& from, const sf::Vector2f& to) {
        float deltaX = to.x - from.x;
        float deltaY = to.y - from.y;
        return std::atan2(deltaY, deltaX) * 180 / M_PI;
    }

    void rotateTowards(const sf::Vector2f& target) {
        float angle = calculateAngle(pos, target);
        s.setRotation(angle);
    }

    sf::Vector2f getPosition() const {
        return pos;
    }

};

class Obstacle
{
    sf::Vector2f pos;
    sf::RectangleShape r;
    sf::Vector2f dim;

public:
    Obstacle(float pos_x, float pos_y, float dim_l, float dim_b)
    {
        pos.x = pos_x;
        pos.y = pos_y;
        dim.x = dim_l;
        dim.y = dim_b;

        r.setPosition(pos);
        r.setFillColor(sf::Color::White);
        r.setSize(dim);
    }

    void render(sf::RenderWindow& wind)
    {
        wind.draw(r);
    }

    sf::RectangleShape& getShape()
    {
        return r;
    }
};

class Gun {
    sf::Vector2f pos;
    sf::Sprite sprite;
    static sf::Texture gun;
    sf::Vector2f dim;
    sf::Vector2f offset;
    std::vector<Ammo> ammoList;
    sf::Clock shootClock;


public:
    int magSize;
    int maxAmmo;
    int currentAmmo;
    float shootCooldown;

    Gun(float pos_x, float pos_y, float dim_l, float dim_b, float cooldown, int magazine, int totalAmmo, sf::Vector2f offset)
        : pos(pos_x, pos_y), dim(dim_l, dim_b), shootCooldown(cooldown), currentAmmo(magazine), magSize(magazine), maxAmmo(totalAmmo), offset(offset)
    {
        sf::Image image;

        if (!image.loadFromFile("gun.png")) {
            throw std::runtime_error("Failed to load image from file: gun.png");
        }

        image.setPixel(0, 0, sf::Color::White);

        if (!gun.loadFromImage(image)) {
            throw std::runtime_error("Failed to load texture from image");
        }

        sprite.setTexture(gun);
        sprite.setScale(dim_l / gun.getSize().x, dim_b / gun.getSize().y);
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
        sprite.setPosition(pos);

        shootClock.restart(); // start the clock
    }

    void render(sf::RenderWindow& wind)
    {
        wind.draw(sprite);
        for (auto& ammo : ammoList)
        {
            ammo.render(wind);
        }
    }

    float calculateAngle(const sf::Vector2f& from, const sf::Vector2f& to) {
        float deltaX = to.x - from.x;
        float deltaY = to.y - from.y;
        return std::atan2(deltaY, deltaX) * 180 / M_PI; // Convert radians to degrees
    }

    sf::Vector2f getPosition() const {
        return pos;
    }

    sf::FloatRect getBounds() const
    {
        return sprite.getGlobalBounds();
    }

    bool isPlayerNear(const sf::CircleShape& player) const {
        sf::FloatRect playerBounds = player.getGlobalBounds();
        sf::FloatRect gunBounds(pos.x - rad, pos.y - rad, 2 * rad, 2 * rad);

        return gunBounds.intersects(playerBounds);
    }

    void updatePos(const Player& player) {
        pos = player.getPosition() + offset;;
        sprite.setPosition(pos);
    }

    void setTotalAmmo(int newTotalAmmo) {
        maxAmmo = newTotalAmmo;
    }

    bool canShoot() const {
        return magSize > 0;
    }

    void reload() {
        if (currentAmmo < magSize && maxAmmo > 0) {
            int needed = magSize - currentAmmo;
            if (maxAmmo >= needed) {
                currentAmmo += needed;
                maxAmmo -= needed;
            }
            else {
                currentAmmo += maxAmmo;
                maxAmmo = 0;
            }
        }
    }

    void shoot(const sf::Vector2f& mousePos)
    {
        if (currentAmmo > 0) {
            float ammoSpeed = 15.0f;
            float ammoWidth = 50.0f;
            float ammoHeight = 25.0f;
            sf::Vector2f direction = mousePos - pos;
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            sf::Vector2f velocity = direction / length;
            Ammo newAmmo(pos.x, pos.y, ammoWidth, ammoHeight, velocity * ammoSpeed);

            if (shootClock.getElapsedTime().asSeconds() >= shootCooldown)
            {
                ammoList.push_back(newAmmo);
                currentAmmo--;
                shootClock.restart();

            }
        }

    }

    int getCurrentAmmo() const {
        return currentAmmo;
    }

    int getTotalAmmo() const {
        return maxAmmo;
    }

    void updateAmmo()
    {
        for (auto& ammo : ammoList)
        {
            ammo.update();
        }

        // remove off-screen ammo
        ammoList.erase(
            std::remove_if(ammoList.begin(), ammoList.end(), [](const Ammo& ammo) {
                return ammo.getBounds().top + ammo.getBounds().height < 0;
                }),
            ammoList.end()
        );
    }

    void rotateTowards(const sf::Vector2f& target) {
        float angle = calculateAngle(pos, target);
        sprite.setRotation(angle);
    }

    // getter for ammoList
    std::vector<Ammo>& getAmmoList() {
        return ammoList;
    }

};

class Enemy {
    sf::Vector2f pos;
    sf::Sprite sprite;
    static sf::Texture enemy;
    sf::Vector2f dim;
    float health;


public:
    Enemy(float pos_x, float pos_y, float dim_l, float dim_b, int initialHealth = 100, float cooldown = 1.0f)
        : pos(pos_x, pos_y), dim(dim_l, dim_b), health(initialHealth)
    {
        sf::Image image;

        if (!image.loadFromFile("enemy.png")) {
            throw std::runtime_error("Failed to load image from file: enemy.png");
        }

        image.setPixel(0, 0, sf::Color::White); // Set the top-left pixel to white (assuming it's the transparent color)

        if (!enemy.loadFromImage(image)) {
            throw std::runtime_error("Failed to load texture from image");
        }

        sprite.setTexture(enemy);
        sprite.setScale(dim_l / enemy.getSize().x, dim_b / enemy.getSize().y);
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
        sprite.setPosition(pos);

    }


    void render(sf::RenderWindow& wind)
    {
        if (health > 0) {
            wind.draw(sprite);
        }
    }

    void update() {
        if (health <= 0) {
            health = 0;
        }

    }

    sf::Vector2f getPosition() const {
        return pos;
    }

    sf::FloatRect getBounds() const
    {
        return sprite.getGlobalBounds();
    }

    int getHealth() {
        return health;
    }

    bool isAlive() const {
        return health > 0;
    }

    void move(float dx, float dy)
    {
        pos.x += dx;
        pos.y += dy;
        sprite.setPosition(pos);
    }

    bool isPlayerNear(const sf::CircleShape& player) const {
        sf::FloatRect playerBounds = player.getGlobalBounds();
        sf::FloatRect enemyBounds(pos.x + rad, pos.y + rad, 2 * rad, 2 * rad);

        return enemyBounds.intersects(playerBounds);
    }

    void checkBoundaryCollision()
    {
        if (pos.x - rad < 0) pos.x = rad;
        if (pos.x + rad > w) pos.x = w - rad;
        if (pos.y - rad < 0) pos.y = rad;
        if (pos.y + rad > h) pos.y = h - rad;
        sprite.setPosition(pos);
    }

    bool checkCollision(const sf::RectangleShape& obstacle) const
    {
        sf::FloatRect obstacleBounds = obstacle.getGlobalBounds();
        sf::FloatRect enemyBounds(pos.x - dim.x / 2, pos.y - dim.y / 2, dim.x, dim.y);

        return enemyBounds.intersects(obstacleBounds);
    }

    bool findPlayer(const Player& player) {
        sf::Vector2f playerPos = player.getPosition();

        float distance = std::sqrt(std::pow(pos.x - playerPos.x, 2) + std::pow(pos.y - playerPos.y, 2));

        const float detectionRange = 100000.0f; // adjust as needed

        return distance <= detectionRange;
    }

    bool isCollidingWith(const Enemy& other) const {
        return sprite.getGlobalBounds().intersects(other.sprite.getGlobalBounds());
    }

    void resolveCollisionWith(Enemy& other) {
        if (!isCollidingWith(other)) return;

        sf::Vector2f direction = pos - other.pos;
        float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // calculate the amount of overlap
        float overlap = rad * 3 - distance;

        if (distance < 10.0f) {

            direction /= distance;

            pos += (direction * overlap) / 2.f;
            other.pos -= (direction * overlap) / 2.f;

            sprite.setPosition(pos);
            other.sprite.setPosition(other.pos);
        }
    }

    void moveTowardsPlayer(const Player& player, float moveSpeed) {
        sf::Vector2f playerPos = player.getPosition();
        sf::Vector2f direction = playerPos - pos;
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        // avoid division by zero
        if (length != 0) {
            direction /= length; // normalize direction
            pos += direction * moveSpeed; // move towards the player
            sprite.setPosition(pos);
        }
    }

    void takeDamage(float damage) {
        health -= damage;
        if (health <= 0) {
            health = 0;
        }
    }

    bool isHit(const Ammo& ammo) const {
        return sprite.getGlobalBounds().intersects(ammo.getBounds());
    }
};

class AmmoPickups {
    sf::Vector2f pos;
    sf::Sprite sprite;
    static sf::Texture ammo;
    sf::Vector2f dim;

public:
    AmmoPickups(float pos_x, float pos_y, float dim_l, float dim_b)
        : pos(pos_x, pos_y), dim(dim_l, dim_b) {
        sf::Image image;

        if (!image.loadFromFile("ammoCrate.png")) {
            throw std::runtime_error("Failed to load image from file: ammoCrate.png");
        }

        image.setPixel(0, 0, sf::Color::White); // Set the top-left pixel to white (assuming it's the transparent color)

        if (!ammo.loadFromImage(image)) {
            throw std::runtime_error("Failed to load texture from image");
        }

        sprite.setTexture(ammo);
        sprite.setScale(dim_l / ammo.getSize().x, dim_b / ammo.getSize().y);
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
        sprite.setPosition(pos);
    }

    void render(sf::RenderWindow& wind) {
        wind.draw(sprite);
    }

    sf::FloatRect getBounds() const
    {
        return sprite.getGlobalBounds();
    }

    bool isPlayerNear(const sf::CircleShape& player) const {
        sf::FloatRect playerBounds = player.getGlobalBounds();
        sf::FloatRect pickupBounds(pos.x, pos.y, dim.x, dim.y);
        return pickupBounds.intersects(playerBounds);
    }

    void addAmmo(Gun& gun) {
        int totalAmmo = gun.getTotalAmmo();
        int ammoToAdd = magsNotClips;

        if (totalAmmo <= totalammo - magsNotClips) {
            gun.setTotalAmmo(totalAmmo + ammoToAdd);
        }
    }

    void updateAmmoPickups(std::vector<AmmoPickups>& ammoPickupsList, const sf::CircleShape& player) {
        ammoPickupsList.erase(
            std::remove_if(ammoPickupsList.begin(), ammoPickupsList.end(), [&player](const AmmoPickups& pickup) {
                return pickup.isPlayerNear(player);
                }),
            ammoPickupsList.end()
        );
    }
};

class healthPickups {
    sf::Vector2f pos;
    sf::Sprite sprite;
    static sf::Texture health;
    sf::Vector2f dim;

public:
    healthPickups(float pos_x, float pos_y, float dim_l, float dim_b)
        : pos(pos_x, pos_y), dim(dim_l, dim_b) {
        sf::Image image;

        if (!image.loadFromFile("health.png")) {
            throw std::runtime_error("Failed to load image from file: health.png");
        }

        image.setPixel(0, 0, sf::Color::White); // Set the top-left pixel to white (assuming it's the transparent color)

        if (!health.loadFromImage(image)) {
            throw std::runtime_error("Failed to load texture from image");
        }

        sprite.setTexture(health);
        sprite.setScale(dim_l / health.getSize().x, dim_b / health.getSize().y);
        sprite.setOrigin(sprite.getLocalBounds().width / 2, sprite.getLocalBounds().height / 2);
        sprite.setPosition(pos);
    }

    void render(sf::RenderWindow& wind) {
        wind.draw(sprite);
    }

    sf::FloatRect getBounds() const
    {
        return sprite.getGlobalBounds();
    }

    bool isPlayerNear(const sf::CircleShape& player) const {
        sf::FloatRect playerBounds = player.getGlobalBounds();
        sf::FloatRect pickupBounds(pos.x, pos.y, dim.x, dim.y);
        return pickupBounds.intersects(playerBounds);
    }

    void addHealth(Player& player) {
        int currentHealth = player.getHealth();
        const int maxHealth = 200;
        const int healthToAdd = 50;

        if (currentHealth < maxHealth) {
            currentHealth += healthToAdd;
            if (currentHealth > maxHealth) {
                currentHealth = maxHealth;
            }
            player.setHealth(currentHealth);
        }
    }

};

class PickupSpawner {
public:
    PickupSpawner() : rng(std::random_device{}()), dist(0.0f, 1.0f) {}

    void spawn(std::vector<AmmoPickups>& ammoP, std::vector<healthPickups>& healthP, sf::Vector2f position, float chance) {
        if (dist(rng) < chance) {
            float pickupChance = dist(rng);
            if (pickupChance < 0.5f) {
                ammoP.push_back(AmmoPickups(position.x, position.y, 200, 150));
            }
            else {
                healthP.push_back(healthPickups(position.x, position.y, 80, 50));
            }
        }
    }

private:
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
};

sf::Texture Ammo::bullet;
sf::Texture AmmoPickups::ammo;
sf::Texture healthPickups::health;
sf::Texture Enemy::enemy;
sf::Texture Gun::gun;


int main()
{
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window(sf::VideoMode(w, h), "democracy manifest", NULL, settings);
    window.setFramerateLimit(60);

    sf::Texture t;
    t.loadFromFile("bg.png");
    sf::Sprite s(t);
    sf::Vector2u textureSize = t.getSize();
    float textureWidth = static_cast<float>(textureSize.x);
    float textureHeight = static_cast<float>(textureSize.y);


    float scaleX = w / textureWidth;
    float scaleY = h / textureHeight;

    // use the larger scale factor to make sure the image covers the entire screen
    float scale = std::max(scaleX, scaleY);
    s.setScale(scale, scale);
    std::srand(std::time(0));

    int min1 = 10;
    int max1 = 1900;

    int min2 = 10;
    int max2 = 1100;

    sf::Vector2f gunOffset(20, -10);

    Player player(700, 600, 200);
    //Obstacle obstacle(100, 600, 30, 30);
    Gun gun(500, 500, 350, 1000, 0.09f, magsNotClips, totalammo, gunOffset);

    std::vector<Enemy> enemies;
    std::vector<AmmoPickups> ammoP;
    std::vector<healthPickups> healthP;

    PickupSpawner pickupSpawner;

    sf::Font font;
    font.loadFromFile("ARIAL.TTF");

    sf::Text fpsText;
    fpsText.setFont(font);
    fpsText.setCharacterSize(24);
    fpsText.setFillColor(sf::Color::White);
    fpsText.setPosition(10, 10);

    sf::Text playerHealthText;
    playerHealthText.setFont(font);
    playerHealthText.setCharacterSize(24);
    playerHealthText.setFillColor(sf::Color::Green);

    sf::Text enemyHealthText;
    enemyHealthText.setFont(font);
    enemyHealthText.setCharacterSize(24);
    enemyHealthText.setFillColor(sf::Color::Red);

    sf::Text ammoCount;
    ammoCount.setFont(font);
    ammoCount.setCharacterSize(24);
    ammoCount.setFillColor(sf::Color::White);

    sf::Text score;
    score.setFont(font);
    score.setCharacterSize(24);
    score.setFillColor(sf::Color::White);

    sf::Clock clock;
    sf::Clock spawnClock; // clock to track enemy spawning
    sf::Clock enemyAttackClock;

    float spawnInterval = 3.5f;
    float enemyAttackInterval = 0.97f;
    float minSpawnInterval = 0.8f;
    float intervalDecrement = 0.098f;
    float hiScore = 0.0f;
    float playerMoveSpeed = 10.0f;
    float enemyMoveSpeed = 8.5f;
    float playerDoesDamage = 33.0f;
    float enemyDoesDamage = 66.0f;
    bool isEquipped = false;


    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) window.close();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) window.close();
        }

        sf::Time deltatime = clock.restart();
        float dt = deltatime.asSeconds() * globalTimeScale;

        sf::Vector2f moveDir(0.0f, 0.0f);

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            moveDir.y -= playerMoveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
            moveDir.y += playerMoveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            moveDir.x -= playerMoveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            moveDir.x += playerMoveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::R)) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(300));
            gun.reload();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E)) {
            if (gun.isPlayerNear(player.getShape())) {
                isEquipped = !isEquipped;
            }

        }

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f worldMousePos = window.mapPixelToCoords(mousePos);

        if (isEquipped) {
            gun.rotateTowards(worldMousePos);
        }

        // shooting with mouse
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            // get mouse position relative to the window
            player.rotateTowards(worldMousePos);
            if (gun.canShoot() && isEquipped) {
                gun.shoot(worldMousePos);
                gun.updatePos(player);

            }
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && player.canDash()) {
            player.dash();

        }

        player.update(dt);

        player.move(moveDir.x, moveDir.y);
        player.checkBoundaryCollision();


        /*if (player.checkCollision(obstacle.getShape())) {
            player.move(-moveDir.x, -moveDir.y);
        }*/

        for (auto it = ammoP.begin(); it != ammoP.end(); ) {
            if (it->isPlayerNear(player.getShape())) {
                it->addAmmo(gun);
                it = ammoP.erase(it);
            }
            else {
                ++it;
            }
        }

        for (auto it = healthP.begin(); it != healthP.end(); ) {
            if (it->isPlayerNear(player.getShape())) {
                it->addHealth(player);
                it = healthP.erase(it); // Remove pickup after use
            }
            else {
                ++it;
            }
        }

        auto& ammoList = gun.getAmmoList(); // access ammolist
        for (auto it = ammoList.begin(); it != ammoList.end(); )
        {
            bool ammoHit = false;
            for (auto& enemy : enemies) {
                if (enemy.isHit(*it)) {
                    enemy.takeDamage(playerDoesDamage);
                    ammoHit = true;
                    break;
                }
            }
            if (ammoHit) {
                it = ammoList.erase(it); // remove the ammo if it hits an enemy
            }
            else {
                ++it;
            }
        }

        if (isEquipped) {
            gun.updatePos(player);
        }

        gun.updateAmmo();

        if (spawnClock.getElapsedTime().asSeconds() >= spawnInterval) {
            int randomPosX = min1 + std::rand() % (max1 - min1 + 1);
            int randomPosY = min2 + std::rand() % (max2 - min2 + 1);
            enemies.push_back(Enemy(randomPosX, randomPosY, 100, 75, 100));


            spawnInterval = std::max(minSpawnInterval, spawnInterval - intervalDecrement);
            spawnClock.restart();
        }

        window.clear(sf::Color::Blue);
        window.draw(s);


        auto it = enemies.begin();
        while (it != enemies.end()) {
            Enemy& enemy = *it;

            if (enemy.isAlive()) {
                enemy.render(window);
                enemy.update();
                enemy.checkBoundaryCollision();

                enemyHealthText.setString("health: " + std::to_string(static_cast<int>(enemy.getHealth())));
                enemyHealthText.setPosition(enemy.getPosition().x - 50, enemy.getPosition().y - 50);

                window.draw(enemyHealthText);

                if (enemy.findPlayer(player)) {
                    enemy.moveTowardsPlayer(player, enemyMoveSpeed);
                }

                for (auto& other : enemies) {
                    if (&enemy != &other && enemy.isAlive() && other.isAlive()) {
                        enemy.resolveCollisionWith(other);
                    }
                }

                if (enemyAttackClock.getElapsedTime().asSeconds() >= enemyAttackInterval) {
                    if (enemy.isPlayerNear(player.getShape())) {
                        player.takeDamage(enemyDoesDamage, window);
                    }
                    enemyAttackClock.restart();
                }

                ++it;
            }

            else {

                hiScore += 10;
                sf::Vector2f spawnPosition = enemy.getPosition();
                float spawnChance = 0.5f;
                pickupSpawner.spawn(ammoP, healthP, enemy.getPosition(), spawnChance);
                it = enemies.erase(it);
            }
        }


        playerHealthText.setString("health: " + std::to_string(static_cast<int>(player.getHealth())));
        playerHealthText.setPosition(player.getPosition().x - 50, player.getPosition().y - 50); // position above player

        ammoCount.setString("ammo: " + std::to_string(gun.getCurrentAmmo()) + "/" + std::to_string(gun.getTotalAmmo()));
        ammoCount.setPosition(25, 50);

        score.setString("score: " + std::to_string(static_cast<int>(hiScore)));
        score.setPosition(25, 100);

        // update fps counter
        sf::Time deltaTime = clock.restart();
        float fps = 1.0f / deltaTime.asSeconds();
        fpsText.setString("fps: " + std::to_string(static_cast<int>(fps)));
        fpsText.setPosition(1800, 50);


        if (player.isAlive()) {
            player.render(window);
        }

        for (auto& a : ammoP) {
            a.render(window); // render all ammo pickups
        }

        for (auto& h : healthP) {
            h.render(window); // render all health pickups
        }

        //obstacle.render(window);

        gun.render(window);
        window.draw(fpsText);
        window.draw(playerHealthText);
        window.draw(ammoCount);
        window.draw(score);

        window.display();
    }

    return 0;
}
