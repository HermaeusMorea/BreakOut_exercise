#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "breakout/assets/asset_manager.h"
#include "breakout/core/game.h"

namespace
{
bool ShouldSpawn(unsigned int chance)
{
    unsigned int random = rand() % chance;
    return random == 0;
}

bool CheckCollision(GameObject& one, GameObject& two)
{
    const bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    const bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    return collisionX && collisionY;
}

Direction VectorDirection(glm::vec2 target)
{
    const glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, -1.0f),
        glm::vec2(-1.0f, 0.0f)
    };

    float max = 0.0f;
    unsigned int best_match = 0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        const float dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return static_cast<Direction>(best_match);
}

Collision CheckCollision(BallObject& one, GameObject& two)
{
    const glm::vec2 center(one.Position + one.Radius);
    const glm::vec2 aabb_half_extents(two.Size.x / 2.0f, two.Size.y / 2.0f);
    const glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    glm::vec2 difference = center - aabb_center;
    const glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    const glm::vec2 closest = aabb_center + clamped;
    difference = closest - center;

    if (glm::length(difference) < one.Radius)
        return std::make_tuple(true, VectorDirection(difference), difference);

    return std::make_tuple(false, UP, glm::vec2(0.0f, 0.0f));
}
}

Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{

}

Game::~Game() = default;

void Game::Init()
{
    // load shaders
    AssetManager::LoadShader("shaders/sprite.vs", "shaders/sprite.frag", "", "sprite");
    AssetManager::LoadShader("shaders/particle.vs", "shaders/particle.frag", "", "particle");
    AssetManager::LoadShader("shaders/post_processing.vs", "shaders/post_processing.frag", "", "postprocessing");
    // configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(this->Width),
        static_cast<float>(this->Height), 0.0f, -1.0f, 1.0f);
    AssetManager::GetShader("sprite").Use().SetInteger("image", 0);
    AssetManager::GetShader("sprite").SetMatrix4("projection", projection);
    AssetManager::GetShader("particle").Use().SetInteger("sprite", 0);
    AssetManager::GetShader("particle").SetMatrix4("projection", projection);
    // load textures
    AssetManager::LoadTexture("textures/background.jpg", false, "background");
    AssetManager::LoadTexture("textures/awesomeface.png", true, "face");
    AssetManager::LoadTexture("textures/block.png", false, "block");
    AssetManager::LoadTexture("textures/block_solid.png", false, "block_solid");
    AssetManager::LoadTexture("textures/paddle.png", true, "paddle");
    AssetManager::LoadTexture("textures/particle.png", true, "particle");
    AssetManager::LoadTexture("textures/powerup_speed.png", true, "powerup_speed");
    AssetManager::LoadTexture("textures/powerup_sticky.png", true, "powerup_sticky");
    AssetManager::LoadTexture("textures/powerup_increase.png", true, "powerup_increase");
    AssetManager::LoadTexture("textures/powerup_confuse.png", true, "powerup_confuse");
    AssetManager::LoadTexture("textures/powerup_chaos.png", true, "powerup_chaos");
    AssetManager::LoadTexture("textures/powerup_passthrough.png", true, "powerup_passthrough");
    // set render-specific controls
    Shader sprite_shader = AssetManager::GetShader("sprite");
    renderer_ = std::make_unique<SpriteRenderer>(sprite_shader);
    particles_ = std::make_unique<ParticleGenerator>(AssetManager::GetShader("particle"), AssetManager::GetTexture("particle"), 500);
    effects_ = std::make_unique<PostProcessor>(AssetManager::GetShader("postprocessing"), this->Width, this->Height);
    // load levels
    GameLevel one; one.Load("levels/one.lvl", this->Width, this->Height / 2);
    GameLevel two; two.Load("levels/two.lvl", this->Width, this->Height / 2);
    GameLevel three; three.Load("levels/three.lvl", this->Width, this->Height / 2);
    GameLevel four; four.Load("levels/four.lvl", this->Width, this->Height / 2);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Level = 0;
    // configure game objects
    glm::vec2 playerPos = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    player_ = std::make_unique<GameObject>(playerPos, PLAYER_SIZE, AssetManager::GetTexture("paddle"));
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -BALL_RADIUS * 2.0f);
    ball_ = std::make_unique<BallObject>(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, AssetManager::GetTexture("face"));
}

void Game::Update(float dt)
{
    // update objects
    ball_->Move(dt, this->Width);
    // check for collisions
    this->DoCollisions();
    // update particles
    particles_->Update(dt, *ball_, 2, glm::vec2(ball_->Radius / 2.0f));
    // update PowerUps
    this->UpdatePowerUps(dt);
    // reduce shake time
    if (shake_time_ > 0.0f)
    {
        shake_time_ -= dt;
        if (shake_time_ <= 0.0f)
            effects_->Shake = false;
    }
    // check loss condition
    if (ball_->Position.y >= this->Height)
    {
        this->ResetLevel();
        this->ResetPlayer();
    }
}

void Game::ProcessInput(float dt)
{
    if (this->State == GAME_ACTIVE)
    {
        float velocity = PLAYER_VELOCITY * dt;
        // move playerboard
        if (this->Keys[GLFW_KEY_A])
        {
            if (player_->Position.x >= 0.0f)
            {
                player_->Position.x -= velocity;
                if (ball_->Stuck)
                    ball_->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (player_->Position.x <= this->Width - player_->Size.x)
            {
                player_->Position.x += velocity;
                if (ball_->Stuck)
                    ball_->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE])
            ball_->Stuck = false;
    }
}

void Game::Render()
{
    if (this->State == GAME_ACTIVE)
    {
        // begin rendering to postprocessing framebuffer
        effects_->BeginRender();
        // draw background
        Texture2D background = AssetManager::GetTexture("background");
        renderer_->DrawSprite(background, glm::vec2(0.0f, 0.0f), glm::vec2(this->Width, this->Height), 0.0f);
        // draw level
        this->Levels[this->Level].Draw(*renderer_);
        // draw player
        player_->Draw(*renderer_);
        // draw PowerUps
        for (PowerUp& powerUp : this->PowerUps)
            if (!powerUp.Destroyed)
                powerUp.Draw(*renderer_);
        // draw particles	
        particles_->Draw();
        // draw ball
        ball_->Draw(*renderer_);
        // end rendering to postprocessing framebuffer
        effects_->EndRender();
        // render postprocessing quad
        effects_->Render(glfwGetTime());
    }
}


void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("levels/one.lvl", this->Width, this->Height / 2);
    else if (this->Level == 1)
        this->Levels[1].Load("levels/two.lvl", this->Width, this->Height / 2);
    else if (this->Level == 2)
        this->Levels[2].Load("levels/three.lvl", this->Width, this->Height / 2);
    else if (this->Level == 3)
        this->Levels[3].Load("levels/four.lvl", this->Width, this->Height / 2);
}

void Game::ResetPlayer()
{
    // reset player/ball stats
    player_->Size = PLAYER_SIZE;
    player_->Position = glm::vec2(this->Width / 2.0f - PLAYER_SIZE.x / 2.0f, this->Height - PLAYER_SIZE.y);
    ball_->Reset(player_->Position + glm::vec2(PLAYER_SIZE.x / 2.0f - BALL_RADIUS, -(BALL_RADIUS * 2.0f)), INITIAL_BALL_VELOCITY);
    // also disable all active powerups
    effects_->Chaos = effects_->Confuse = false;
    ball_->PassThrough = ball_->Sticky = false;
    player_->Color = glm::vec3(1.0f);
    ball_->Color = glm::vec3(1.0f);
}

void Game::UpdatePowerUps(float dt)
{
    for (PowerUp& powerUp : this->PowerUps)
    {
        powerUp.Position += powerUp.Velocity * dt;
        if (powerUp.Activated)
        {
            powerUp.Duration -= dt;

            if (powerUp.Duration <= 0.0f)
            {
                // remove powerup from list (will later be removed)
                powerUp.Activated = false;
                // deactivate effects
                if (powerUp.Type == "sticky")
                {
                    if (!IsOtherPowerUpActive("sticky"))
                    {	// only reset if no other PowerUp of type sticky is active
                        ball_->Sticky = false;
                        player_->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "pass-through")
                {
                    if (!IsOtherPowerUpActive("pass-through"))
                    {	// only reset if no other PowerUp of type pass-through is active
                        ball_->PassThrough = false;
                        ball_->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse")
                {
                    if (!IsOtherPowerUpActive("confuse"))
                    {	// only reset if no other PowerUp of type confuse is active
                        effects_->Confuse = false;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    if (!IsOtherPowerUpActive("chaos"))
                    {	// only reset if no other PowerUp of type chaos is active
                        effects_->Chaos = false;
                    }
                }
            }
        }
    }
    // Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
    // Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp& powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
    ), this->PowerUps.end());
}

void Game::SpawnPowerUps(GameObject& block)
{
    if (ShouldSpawn(50)) // 1 in 75 chance
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, AssetManager::GetTexture("powerup_speed")));
    if (ShouldSpawn(50))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, AssetManager::GetTexture("powerup_sticky")));
    if (ShouldSpawn(50))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, AssetManager::GetTexture("powerup_passthrough")));
    if (ShouldSpawn(50))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, AssetManager::GetTexture("powerup_increase")));
    if (ShouldSpawn(10)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, AssetManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(10))
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, AssetManager::GetTexture("powerup_chaos")));
}

void Game::ActivatePowerUp(PowerUp& powerUp)
{
    if (powerUp.Type == "speed")
    {
        ball_->Velocity *= 1.2f;
    }
    else if (powerUp.Type == "sticky")
    {
        ball_->Sticky = true;
        player_->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        ball_->PassThrough = true;
        ball_->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        player_->Size.x += 50.0f;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!effects_->Chaos)
            effects_->Confuse = true;
    }
    else if (powerUp.Type == "chaos")
    {
        if (!effects_->Confuse)
            effects_->Chaos = true;
    }
}

bool Game::IsOtherPowerUpActive(const std::string& type) const
{
    for (const PowerUp& powerUp : this->PowerUps)
    {
        if (powerUp.Activated && powerUp.Type == type)
            return true;
    }
    return false;
}

void Game::DoCollisions()
{
    for (GameObject& box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*ball_, box);
            if (std::get<0>(collision)) // if collision is true
            {
                // destroy block if not solid
                if (!box.IsSolid)
                {
                    box.Destroyed = true;
                    shake_time_ = 0.05f;
                    effects_->Shake = true;
                    this->SpawnPowerUps(box);
                }
                else
                {
                    shake_time_ = 0.05f;
                    effects_->Shake = true;
                }
                // collision resolution
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (!(ball_->PassThrough && !box.IsSolid))
                {
                    if (dir == LEFT || dir == RIGHT)
                    {
                        ball_->Velocity.x = -ball_->Velocity.x;
                        const float penetration = ball_->Radius - std::abs(diff_vector.x);
                        if (dir == LEFT)
                            ball_->Position.x += penetration;
                        else
                            ball_->Position.x -= penetration;
                    }
                    else
                    {
                        ball_->Velocity.y = -ball_->Velocity.y;
                        const float penetration = ball_->Radius - std::abs(diff_vector.y);
                        if (dir == UP)
                            ball_->Position.y -= penetration;
                        else
                            ball_->Position.y += penetration;
                    }
                }
            }
        }
    }

    // also check collisions on PowerUps and if so, activate them
    for (PowerUp& powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            // first check if powerup passed bottom edge, if so: keep as inactive and destroy
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = true;

            if (CheckCollision(*player_, powerUp))
            {
                ActivatePowerUp(powerUp);
                powerUp.Destroyed = true;
                powerUp.Activated = true;
            }
        }
    }

    Collision result = CheckCollision(*ball_, *player_);
    if (!ball_->Stuck && std::get<0>(result))
    {
        const float centerBoard = player_->Position.x + player_->Size.x / 2.0f;
        const float distance = (ball_->Position.x + ball_->Radius) - centerBoard;
        const float percentage = distance / (player_->Size.x / 2.0f);
        const float strength = 2.0f;
        const glm::vec2 oldVelocity = ball_->Velocity;
        ball_->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        ball_->Velocity = glm::normalize(ball_->Velocity) * glm::length(oldVelocity);
        ball_->Velocity.y = -std::abs(ball_->Velocity.y);
        ball_->Stuck = ball_->Sticky;
    }
}
