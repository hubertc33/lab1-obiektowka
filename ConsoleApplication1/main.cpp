#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <string>
#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}
class Pickup {
public:
	enum class Type { HEALTH, TRIPLESHOT };

	Pickup(Vector2 pos, Type t) : type(t), position(pos), lifetime(10.f) {
		if (!TexturesLoaded()) LoadTextures();

		switch (type) {
		case Type::HEALTH:
			radius = (GetHeartTexture().width * HEALTH_SCALE) * 0.5f * COLLISION_SCALE;
			break;
		case Type::TRIPLESHOT:
			radius = (GetTripleTexture().width * TRIPLE_SCALE) * 0.5f * COLLISION_SCALE;
			break;
		}
	}

	bool Update(float dt) {
		lifetime -= dt;
		return lifetime <= 0.f;
	}

	void Draw() const {
		float alpha = 0.5f + 0.5f * sinf(GetTime() * 4.0f);
		Color tint = ColorAlpha(WHITE, alpha);

		switch (type) {
		case Type::HEALTH: {
			Vector2 pos = {
				position.x - (GetHeartTexture().width * HEALTH_SCALE) * 0.5f,
				position.y - (GetHeartTexture().height * HEALTH_SCALE) * 0.5f
			};
			DrawTextureEx(GetHeartTexture(), pos, 0.0f, HEALTH_SCALE, tint);
			break;
		}
		case Type::TRIPLESHOT: {
			Vector2 pos = {
				position.x - (GetTripleTexture().width * TRIPLE_SCALE) * 0.5f,
				position.y - (GetTripleTexture().height * TRIPLE_SCALE) * 0.5f
			};
			DrawTextureEx(GetTripleTexture(), pos, 0.0f, TRIPLE_SCALE, tint);
			break;
		}
		}
	}

	Type GetType() const { return type; }
	Vector2 GetPosition() const { return position; }
	float GetRadius() const { return radius; }

private:
	Type type;
	Vector2 position;
	float radius = 0.f;
	float lifetime = 10.f;

	static constexpr float HEALTH_SCALE = 0.2f;
	static constexpr float TRIPLE_SCALE = 0.1f;
	static constexpr float COLLISION_SCALE = 0.1f;

	static Texture2D& GetHeartTexture() {
		static Texture2D tex{};
		return tex;
	}

	static Texture2D& GetTripleTexture() {
		static Texture2D tex{};
		return tex;
	}

	static bool& TexturesLoaded() {
		static bool loaded = false;
		return loaded;
	}

	static void LoadTextures() {
		GetHeartTexture() = LoadTexture("heart.png");
		GetTripleTexture() = LoadTexture("exp.png");
		SetTextureFilter(GetHeartTexture(), TEXTURE_FILTER_BILINEAR);
		SetTextureFilter(GetTripleTexture(), TEXTURE_FILTER_BILINEAR);
		TexturesLoaded() = true;
	}
};


struct DamagePopup {
	Vector2 position;
	std::string text;
	float time;       
	float maxTime;   
	Color color;

	DamagePopup(Vector2 pos, int dmg)
		: position(pos), text("-" + std::to_string(dmg)), time(0.f), maxTime(1.f), color{ 255, 0, 0, 255 } {}

	bool Update(float dt) {
		time += dt;
		position.y -= 30 * dt; 
		color.a = static_cast<unsigned char>(255 * (1.f - time / maxTime)); 
		return time >= maxTime;
	}

	void Draw() const {
		DrawText(text.c_str(), static_cast<int>(position.x), static_cast<int>(position.y), 20, color);
	}
};


// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, WHITE);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	virtual  bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

	void TakeDamage(int dmg) {
		hp -= dmg;
	}

	bool IsDestroyed() const {
		return hp <= 0;
	}

	int GetHP() const {
		return hp;
	}

	int GetBaseHP() const {
		return baseHp;
	}

protected:
	int hp = 1;
	int baseHp = 1;
	enum class Type { TRIANGLE, SQUARE, PENTAGON, OCTAGON, BOSS };
	Type asteroidType;


	void init(int screenW, int screenH) {


		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);

		
		int size = GetSize();
		int typeBonus = 0;

		switch (asteroidType) {
		case Type::TRIANGLE: typeBonus = 10; break;
		case Type::SQUARE: typeBonus = 30; break;
		case Type::PENTAGON: typeBonus = 50; break;
		case Type::OCTAGON: typeBonus = 70; break;
		case Type::BOSS:; break;
		}


		baseHp = size * 20 + typeBonus;
		hp = baseHp;

	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 5;  asteroidType = Type::TRIANGLE;
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);

		float barW = GetRadius() * 2;
		float barH = 5;
		float ratio = (float)hp / (float)baseHp;
		float filledW = barW * ratio;
		Vector2 pos = { transform.position.x - barW / 2, transform.position.y - GetRadius() - 12 };
		DrawRectangleV(pos, { barW, barH }, DARKGRAY);
		DrawRectangleV(pos, { filledW, barH }, GREEN);
	}

};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 10;   asteroidType = Type::SQUARE;
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
		float barW = GetRadius() * 2;
		float barH = 5;
		float ratio = (float)hp / (float)baseHp;
		float filledW = barW * ratio;
		Vector2 pos = { transform.position.x - barW / 2, transform.position.y - GetRadius() - 12 };
		DrawRectangleV(pos, { barW, barH }, DARKGRAY);
		DrawRectangleV(pos, { filledW, barH }, GREEN);
	}

};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 15;   asteroidType = Type::PENTAGON;
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
		float barW = GetRadius() * 2;
		float barH = 5;
		float ratio = (float)hp / (float)baseHp;
		float filledW = barW * ratio;
		Vector2 pos = { transform.position.x - barW / 2, transform.position.y - GetRadius() - 12 };
		DrawRectangleV(pos, { barW, barH }, DARKGRAY);
		DrawRectangleV(pos, { filledW, barH }, GREEN);
	}

};
class OctagonAsteroid : public Asteroid {
public:
	OctagonAsteroid(int w, int h) : Asteroid(w, h) {
		baseDamage = 15;   asteroidType = Type::OCTAGON;
	}
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 8, GetRadius(), transform.rotation);
		float barW = GetRadius() * 2;
		float barH = 5;
		float ratio = (float)hp / (float)baseHp;
		float filledW = barW * ratio;
		Vector2 pos = { transform.position.x - barW / 2, transform.position.y - GetRadius() - 12 };
		DrawRectangleV(pos, { barW, barH }, DARKGRAY);
		DrawRectangleV(pos, { filledW, barH }, GREEN);
	}

};
// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, OCTAGON = 8, RANDOM = 0 };




// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape) {
	switch (shape) {
	case AsteroidShape::TRIANGLE:
		return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE:
		return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON:
		return std::make_unique<PentagonAsteroid>(w, h);
	case AsteroidShape::OCTAGON:
		return std::make_unique<OctagonAsteroid>(w, h);
	default: {
		const int shapes[] = { 3, 4, 5, 8 };
		int randomIndex = GetRandomValue(0, 3);
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(shapes[randomIndex]));
	}
	}
}



// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, ROCKET, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt, bool isEnemy = false)
		: isEnemy(isEnemy)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}
	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));

		if (transform.position.x < 0 ||
			transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 ||
			transform.position.y > Renderer::Instance().Height())
		{
			return true;
		}
		return false;
	}
	void Draw() const {
		switch (type) {
		case WeaponType::BULLET:
			if (isEnemy)
				DrawCircleV(transform.position, 6.f, PURPLE); 
			else
				DrawCircleV(transform.position, 5.f, WHITE); 
			break;

		case WeaponType::LASER:
			DrawRectangle(transform.position.x - 2.f, transform.position.y - 30.f, 4.f, 30.f, RED);
			break;
		case WeaponType::ROCKET:
			DrawRectangle(transform.position.x - 4.f, transform.position.y - 10.f, 8.f, 20.f, ORANGE);
			break;
		}
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	float GetRadius() const {
		return (type == WeaponType::BULLET) ? 5.f : 2.f;
	}

	int GetDamage() const {
		return baseDamage;
	}

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;
	bool       isEnemy = false;
};
inline static Projectile MakeProjectile(WeaponType wt, const Vector2 pos, float speed, float angleDeg = 0.f)
{
	float angleRad = DEG2RAD * angleDeg;
	Vector2 vel = {
		sinf(angleRad) * speed,
		-cosf(angleRad) * speed
	};

	int dmg = 0;
	switch (wt) {
	case WeaponType::LASER:  dmg = 10; break;
	case WeaponType::BULLET: dmg = 30; break;
	case WeaponType::ROCKET: dmg = 200; break;
	}

	return Projectile(pos, vel, dmg, wt);
}


class BossAsteroid : public Asteroid {
public:
	BossAsteroid(int w, int h, std::vector<Projectile>& bossProjectilesRef)
		: Asteroid(w, h), bossProjectiles(bossProjectilesRef) {

		baseDamage = 50;
		baseHp = 10000;
		hp = baseHp;
		asteroidType = Type::BOSS;
		texture = LoadTexture("boss.png");
		scale = 0.3f;

		
		transform.position = { w * 0.5f, -100 };
		physics.velocity = { 0.f, 50.f }; 
		transform.rotation = 0.f;
	}

	~BossAsteroid() {
		UnloadTexture(texture);
	}
	bool movingRight = true;
	float horizontalSpeed = 100.f;

	bool Update(float dt) override {
		shootTimer += dt;

		
		if (transform.position.y < 200) {
			transform.position.y += physics.velocity.y * dt;
		}
		else {
			if (movingRight) {
				transform.position.x += horizontalSpeed * dt;
				if (transform.position.x > Renderer::Instance().Width() - 300) {
					movingRight = false;
				}
			}
			else {
				transform.position.x -= horizontalSpeed * dt;
				if (transform.position.x < 300) {
					movingRight = true;
				}
			}
		}

		if (shootTimer >= shootInterval) {
			Vector2 vel = { 0, 900 };
			Vector2 firePos = transform.position;
			firePos.x += 20.f;
			bossProjectiles.emplace_back(firePos, vel, 40, WeaponType::BULLET, true);
			shootTimer = 0.f;
		}

		return true; 
	}



	void Draw() const override {
		Vector2 pos = {
			transform.position.x - (texture.width * scale) / 2.f,
			transform.position.y - (texture.height * scale) / 2.f
		};
		DrawTextureEx(texture, pos, transform.rotation, scale, WHITE);

		float barW = 300.f;
		float barH = 30.f;  
		float ratio = (float)hp / baseHp;
		float filledW = barW * ratio;

		Vector2 hpPos = {
			transform.position.x - barW / 2.f + 20.f,
			pos.y + 5.f

		};

		DrawRectangleV(hpPos, { barW, barH }, DARKGRAY);
		DrawRectangleV(hpPos, { filledW, barH }, RED);

	
		std::string hpText = std::to_string(hp) + " / " + std::to_string(baseHp);
		int textWidth = MeasureText(hpText.c_str(), 20);
		int textX = static_cast<int>(hpPos.x + barW / 2 - textWidth / 2);
		int textY = static_cast<int>(hpPos.y + barH / 2 - 10);

		DrawText(hpText.c_str(), textX, textY, 20, WHITE);
	}
	float GetRadius() const override {
		return (texture.width * scale) * 0.4f; 
	}


private:
	std::vector<Projectile>& bossProjectiles;
	Texture2D texture;
	float scale = 0.4f;
	float shootTimer = 0.f;
	float shootInterval = 2.0f;
	bool dying = false;
	float deathFadeTimer = 0.f;
	float fadeDuration = 2.f;
	bool spawnedOnDeath = false;

};



// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
												 screenW * 0.5f,
												 screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 20.f; // shots/sec
		fireRateBullet = 15.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 200.f;
		fireRateRocket = 2.0f;
		spacingRocket = 500.f;

	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;

		if (hp > 100) hp = 100;
		if (hp <= 0) alive = false;
	}


	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		if (wt == WeaponType::ROCKET) return fireRateRocket;
		if (wt == WeaponType::LASER) return fireRateLaser;
		if (wt == WeaponType::BULLET) return fireRateBullet;
		return fireRateBullet;

	}

	float GetSpacing(WeaponType wt) const {
		if (wt == WeaponType::ROCKET) return spacingRocket;
		if (wt == WeaponType::LASER) return spacingLaser;
		if (wt == WeaponType::BULLET) return spacingBullet;
		return fireRateBullet;

	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float fireRateLaser;
	float fireRateBullet;
	float fireRateRocket;
	float spacingLaser;
	float spacingBullet;
	float spacingRocket;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("spaceship1.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		SetTextureFilter(texture, 2);
		scale = 0.25f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		DrawTextureEx(texture, dstPos, 0.0f, scale, WHITE);
	}

	float GetRadius() const override {
		return (texture.width * scale) * 0.5f;
	}

private:
	Texture2D texture;
	float     scale;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}

	void Run() {
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Asteroids OOP");

		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;
			pickupSpawnTimer += dt;
			if (tripleShotActive) {
				tripleShotTimer -= dt;
				if (tripleShotTimer <= 0.f) {
					tripleShotActive = false;
				}
			}

			if (pickupSpawnTimer >= PICKUP_SPAWN_INTERVAL) {
				Vector2 pos = {
					Utils::RandomFloat(100, C_WIDTH - 100),
					Utils::RandomFloat(100, C_HEIGHT - 100)
				};
				Pickup::Type type = (GetRandomValue(0, 1) == 0) ? Pickup::Type::HEALTH : Pickup::Type::TRIPLESHOT;
				pickups.emplace_back(pos, type);
				pickupSpawnTimer = 0.f;
			}

			// Update player
			player->Update(dt);

			// Restart logic
			if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
				player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
				asteroids.clear();
				projectiles.clear();
				pickups.clear();
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
				asteroidsDestroyed = 0;
				bossSpawned = false;
			}
			// Asteroid shape switch
			if (IsKeyPressed(KEY_ONE)) {
				currentShape = AsteroidShape::TRIANGLE;
			}
			if (IsKeyPressed(KEY_TWO)) {
				currentShape = AsteroidShape::SQUARE;
			}
			if (IsKeyPressed(KEY_THREE)) {
				currentShape = AsteroidShape::PENTAGON;
			}
			if (IsKeyPressed(KEY_FOUR)) {
				currentShape = AsteroidShape::OCTAGON;
			}
			if (IsKeyPressed(KEY_FIVE)) {
				currentShape = AsteroidShape::RANDOM;
			}

			// Weapon switch
			if (IsKeyPressed(KEY_TAB)) {
				currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
			}

			// Shooting
			{
				if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
					shotTimer += dt;
					float interval = 1.f / player->GetFireRate(currentWeapon);
					float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

					while (shotTimer >= interval) {
						Vector2 p = player->GetPosition();
						p.y -= player->GetRadius();
						projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed));

						projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed, 0.0f));
						if (tripleShotActive) {
							projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed, -15.0f));
							projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed, 15.0f));
						}


						shotTimer -= interval;
					}
				}
				else {
					float maxInterval = 1.f / player->GetFireRate(currentWeapon);

					if (shotTimer > maxInterval) {
						shotTimer = fmodf(shotTimer, maxInterval);
					}
				}
			}

			// Spawn asteroids
			if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
				asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape));
				spawnTimer = 0.f;
				spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
			}

			if (asteroidsDestroyed > 0 && asteroidsDestroyed % 50 == 0 && !bossSpawned) {
				asteroids.push_back(std::make_unique<BossAsteroid>(C_WIDTH, C_HEIGHT, bossProjectiles));
				bossSpawned = true;
			}

			// Update projectiles - check if in boundries and move them forward
			{
				auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
					[dt](auto& projectile) {
						return projectile.Update(dt);
					});
				projectiles.erase(projectile_to_remove, projectiles.end());
			}

			auto boss_remove = std::remove_if(bossProjectiles.begin(), bossProjectiles.end(),
				[dt](auto& bp) { return bp.Update(dt); });
			bossProjectiles.erase(boss_remove, bossProjectiles.end());

			// Projectile-Asteroid collisions O(n^2)
			for (auto pit = projectiles.begin(); pit != projectiles.end();) {
				bool removed = false;

				for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
					float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
					if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
						(*ait)->TakeDamage(pit->GetDamage());
						damagePopups.emplace_back((*ait)->GetPosition(), pit->GetDamage());
						pit = projectiles.erase(pit);
						if ((*ait)->IsDestroyed()) {
							if (dynamic_cast<BossAsteroid*>(ait->get()) != nullptr) {
								bossSpawned = false;
							}
							ait = asteroids.erase(ait);
							asteroidsDestroyed++;
						}
						else {
							++ait;
						}
						removed = true;
						break;
					}
				}
				if (!removed) {
					++pit;
				}
			}

			// Asteroid-Ship collisions
			{
				auto remove_collision =
					[&player, dt](auto& asteroid_ptr_like) -> bool {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

						if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
							player->TakeDamage(asteroid_ptr_like->GetDamage());
							return true; // Mark asteroid for removal due to collision
						}
					}
					if (!asteroid_ptr_like->Update(dt)) {
						return true;
					}
					return false; // Keep the asteroid
					};
				auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
				asteroids.erase(asteroid_to_remove, asteroids.end());
				// Update damage popups
				{
					auto to_remove = std::remove_if(damagePopups.begin(), damagePopups.end(),
						[dt](auto& popup) {
							return popup.Update(dt);
						});
					damagePopups.erase(to_remove, damagePopups.end());
				}
				auto pickup_to_remove = std::remove_if(pickups.begin(), pickups.end(),
					[&player, dt, this](auto& pickup) {

						if (Vector2Distance(player->GetPosition(), pickup.GetPosition()) < pickup.GetRadius() + player->GetRadius()) {
							if (pickup.GetType() == Pickup::Type::HEALTH) {
								if (player->GetHP() < 100)
									player->TakeDamage(-25);
							}
							else if (pickup.GetType() == Pickup::Type::TRIPLESHOT) {
								tripleShotActive = true;
								tripleShotTimer = 5.f;
							}
							return true; 
						}
						return pickup.Update(dt);
					});
				pickups.erase(pickup_to_remove, pickups.end());
				// --- KOLIZJE POCISKÓW BOSSOWYCH Z GRACZEM ---
				for (auto it = bossProjectiles.begin(); it != bossProjectiles.end(); ) {
					if (player->IsAlive()) {
						float dist = Vector2Distance(player->GetPosition(), it->GetPosition());
						if (dist < player->GetRadius() + it->GetRadius()) {
							player->TakeDamage(it->GetDamage());
							it = bossProjectiles.erase(it);
							continue;
						}
					}
					++it;
				}


			}

			// Render everything
			{
				Renderer::Instance().Begin();

				// --- Pasek HP ---
				float hpRatio = static_cast<float>(player->GetHP()) / 100.0f;
				Color hpColor = GREEN;

				if (hpRatio < 0.3f)
					hpColor = RED;
				else if (hpRatio < 0.6f)
					hpColor = YELLOW;

				DrawRectangle(10, 10, 200, 20, DARKGRAY);  
				DrawRectangle(10, 10, static_cast<int>(200 * hpRatio), 20, hpColor);
				DrawText("HP", 215, 10, 20, hpColor);



				const char* weaponName = "UNKNOWN";
				switch (currentWeapon) {
				case WeaponType::LASER: weaponName = "LASER"; break;
				case WeaponType::BULLET: weaponName = "BULLET"; break;
				case WeaponType::ROCKET: weaponName = "ROCKET"; break;
				}

				DrawText(TextFormat("Weapon: %s", weaponName), 10, 40, 20, BLUE);


				DrawText(TextFormat("Destroyed: %d", asteroidsDestroyed), 10, 70, 20, ORANGE);


				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}
				for (const auto& popup : damagePopups) {
					popup.Draw();
				}
				for (const auto& p : pickups) {
					p.Draw();
				}
				for (const auto& bp : bossProjectiles)
					bp.Draw();

				player->Draw();

				Renderer::Instance().End();
			}
		}
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};
	std::vector<Projectile> bossProjectiles;
	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;
	std::vector<DamagePopup> damagePopups;
	std::vector<Pickup> pickups;
	float pickupSpawnTimer = 0.f;
	static constexpr float PICKUP_SPAWN_INTERVAL = 15.f;
	bool tripleShotActive = false;
	float tripleShotTimer = 0.f;
	bool bossSpawned = false;
	AsteroidShape currentShape = AsteroidShape::TRIANGLE;
	int asteroidsDestroyed = 0;

	static constexpr int C_WIDTH = 2500;
	static constexpr int C_HEIGHT = 1300;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
};

int main() {
	Application::Instance().Run();
	return 0;
}
