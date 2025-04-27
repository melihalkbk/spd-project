#define SFML_DEBUG  // Add for debug messages
#include <GL/glew.h> //Add for graphic design
#include <GLFW/glfw3.h> //Add for graphic design
#include <SFML/Audio.hpp> //Add for sound
#include <iostream> //For input-output stream
#include <vector> //For vector operations
#include <cstdlib> //For random number generation
#include <ctime> //For time
#include <sstream> //For string stream
#include <filesystem> // For directory operations
#include <optional>   // For optional type 
#include <map>  // For std::map
#include <memory>
#include <algorithm>

// Define sound buffer objects
sf::SoundBuffer collisionBuffer;
sf::SoundBuffer powerUpBuffer;
sf::SoundBuffer levelUpBuffer;
sf::SoundBuffer gameOverBuffer;

// Define sound objects with initial buffer
sf::Sound collisionSound(collisionBuffer);
sf::Sound powerUpSound(powerUpBuffer);
sf::Sound levelUpSound(levelUpBuffer);
sf::Sound gameOverSound(gameOverBuffer);
sf::Music sigma;

float playerX = 0.0f; //Player position(Horizontal)
float playerSpeed = 0.07f;
float blockSpeed = 0.01f; // Initial block speed(Increases with level)
int score = 0;
int health = 3;
int level = 1;  
bool gameOver = false;
bool gameStarted = false;
float backgroundColor = 0.0f;
bool colorIncreasing = true; // Background color animation direction

// Sound control
bool isMuted = false; 
float previousVolume = 30.0f;  // Store previous volume for unmuting

// Game state
bool isPaused = false;

// Transition effects
bool fadeInEffect = false;
bool fadeOutEffect = false;
float fadeAlpha = 1.0f;

const int SCORE_PER_LEVEL = 20; // Score needed to level up
const float LEVEL_SPEED_INCREASE = 0.0003f; // Speed increase per level
const float NORMAL_SPEED_INCREASE = 0.00003f; // Tiny speed increase per score
const int MAX_BLOCKS = 10; // Maximum number of blocks

struct Block {
    float x, y;
    int shape;  // 0 = square, 1 = triangle, 2 = circle
    float r, g, b; // Block color
    int movementPattern; // 0 = linear, 1 = zigzag, 2 = circular
    float movementTimer; // For tracking movement cycles
    float originX; // Original X position for circular/zigzag patterns
};

struct PowerUp {
    float x, y;
    int type;  // 1 = speed, 2 = block reset, 3 = invisibility, 4 = time slow, 5 = shield, 6 = extra life
    float duration;  // Power-up duration
};

// Handle time slow power-up
bool hasTimeSlow = false;
float timeSlowTimer = 0.0f;
float timeSlowFactor = 1.0f;

// Handle shield power-up
bool hasShield = false;
float shieldTimer = 0.0f;

// Font texture and character info
struct Character {
    float advanceX;    // Advance X offset
    float advanceY;    // Advance Y offset
    float width;       // Character width
    float height;      // Character height
    float texX;        // Texture X offset
    float texY;        // Texture Y offset
    float texWidth;    // Texture width
    float texHeight;   // Texture height
};

// Font texture and characters
GLuint fontTextureID = 0;
std::map<char, Character> characters;
bool fontLoaded = false;

// Font texture size
const int FONT_TEXTURE_WIDTH = 512;
const int FONT_TEXTURE_HEIGHT = 512;

std::vector<Block> blocks;
std::vector<PowerUp> powerUps;
bool isInvisible = false; // Invisibility state
float invisibilityTimer = 0.0f; 

// Power-up variables
bool hasSpeedBoost = false;
float speedBoostTimer = 0.0f;
float originalPlayerSpeed = 0.05f;
bool hasBlockReset = false;
float blockResetTimer = 0.0f;

// Particle structure
struct Particle {
    float x, y;          // Position
    float vx, vy;        // Velocity vector
    float r, g, b, a;    // Color (red, green, blue, alpha)
    float lifetime;      // Lifetime
    float size;          // Size
    float rotation;      // Rotation angle (degrees)
    float rotationSpeed; // Rotation speed
};

// Global variables for particle system
std::vector<Particle> particles;
const int MAX_PARTICLES = 200;

// Function to create a particle
void createParticle(float x, float y, float vx, float vy, 
                   float r, float g, float b, float a,
                   float lifetime, float size, float rotation = 0.0f, float rotationSpeed = 0.0f) {
    if (particles.size() < MAX_PARTICLES) {
        particles.push_back({
            x, y, vx, vy, r, g, b, a, lifetime, size, rotation, rotationSpeed
        });
    }
}

// updateParticles fonksiyonunu değiştirin:
void updateParticles(float deltaTime) {
    // Parçacık sayısı 0 ise erken çık
    if (particles.empty()) {
        return;
    }
    
    // Güvenlik kontrolü ve sayı sınırlaması
    if (particles.size() > MAX_PARTICLES) {
        particles.resize(MAX_PARTICLES);
    }
    
    // Remove-erase idiom kullanarak ölü parçacıkları güvenle sil
    particles.erase(
        std::remove_if(
            particles.begin(), 
            particles.end(),
            [&](Particle& p) {
                // Lifetime güncellemesi
                p.lifetime -= deltaTime;
                
                // Eğer yaşam süresi bittiyse, kaldır
                if (p.lifetime <= 0.0f) {
                    return true; // Sil
                }
                
                // Pozisyon güncelleme
                p.x += p.vx * deltaTime;
                p.y += p.vy * deltaTime;
                
                // Rotasyon güncelleme
                p.rotation += p.rotationSpeed * deltaTime;
                
                // Yerçekimi ve yavaşlama 
                p.vy -= 0.002f;
                p.vx *= 0.98f;
                p.vy *= 0.98f;
                
                // Alpha güncelleme
                p.a = std::min(1.0f, p.lifetime);
                
                // Ekran dışındaysa sil
                if (p.x < -2.0f || p.x > 2.0f || p.y < -2.0f || p.y > 2.0f) {
                    return true; // Sil
                }
                return false; // Sakla
            }
        ),
        particles.end()
    );
}

// Function to draw different particle shapes
void drawParticle(const Particle& p) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glPushMatrix();
    glTranslatef(p.x, p.y, 0.0f);
    glRotatef(p.rotation, 0.0f, 0.0f, 1.0f);
    
    // Square particle
    glColor4f(p.r, p.g, p.b, p.a);
    glBegin(GL_QUADS);
        glVertex2f(-p.size/2, -p.size/2);
        glVertex2f(p.size/2, -p.size/2);
        glVertex2f(p.size/2, p.size/2);
        glVertex2f(-p.size/2, p.size/2);
    glEnd();
    
    glPopMatrix();
    glDisable(GL_BLEND);
}

// Parçacık çizim fonksiyonunu güvenceye al
void drawParticles() {
    int maxParticlesToDraw = 100; // Limiti azalt
    int count = 0;
    for (const auto& p : particles) {
        if (count++ > maxParticlesToDraw) break; // Çizilecek parçacık sayısını sınırla
        drawParticle(p);
    }
}

// Function for collision animation
void createBlockExplosion(float x, float y, float r, float g, float b) {
    // Number of particles
    const int numParticles = 20;
    
    // Spread range of particles
    const float spread = 0.15f;
    
    for (int i = 0; i < numParticles; i++) {
        // Create random velocity vector
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.1f + (rand() % 100) / 500.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed;
        
        // Particle size
        float size = 0.01f + (rand() % 100) / 2000.0f;
        
        // Lifetime
        float lifetime = 0.5f + (rand() % 100) / 200.0f;
        
        // Add color variation
        float colorVar = 0.2f;
        float rVal = r + ((rand() % 100) / 100.0f - 0.5f) * colorVar;
        float gVal = g + ((rand() % 100) / 100.0f - 0.5f) * colorVar;
        float bVal = b + ((rand() % 100) / 100.0f - 0.5f) * colorVar;
        
        // Clamp values
        rVal = std::max(0.0f, std::min(1.0f, rVal));
        gVal = std::max(0.0f, std::min(1.0f, gVal));
        bVal = std::max(0.0f, std::min(1.0f, bVal));
        
        // Random starting position (around the block)
        float startX = x + ((rand() % 100) / 100.0f - 0.5f) * 0.1f;
        float startY = y + ((rand() % 100) / 100.0f - 0.5f) * 0.1f;
        
        // Create particle
        float rotation = rand() % 360;
        float rotationSpeed = ((rand() % 200) - 100) * 2.0f; // -200 to 200 degs/sec
        
        createParticle(startX, startY, vx, vy, rVal, gVal, bVal, 1.0f, lifetime, size, rotation, rotationSpeed);
    }
}

// Level up efektini oluşturan fonksiyonu düzelt
void createLevelUpEffect() {
    // Clear existing particles to prevent overflow
    particles.clear(); // Mevcut tüm parçacıkları temizle, kararlılık için
    
    // Limit the number of particles
    const int numRings = 2; // 3'ten 2'ye düşür
    const int particlesPerRing = 20; // 30'dan 20'ye düşür
    
    for (int ring = 0; ring < numRings; ring++) {
        float ringRadius = 0.2f + ring * 0.2f;
        float ringLifetime = 1.0f - ring * 0.2f;
        
        for (int i = 0; i < particlesPerRing; i++) {
            float angle = (i * 360.0f / particlesPerRing) * 3.14159f / 180.0f;
            
            // Particles move outward in circular motion
            float vx = cos(angle) * 0.2f;
            float vy = sin(angle) * 0.2f;
            
            // Starting position on ring
            float x = cos(angle) * (0.05f + ring * 0.05f); // Start near center
            float y = sin(angle) * (0.05f + ring * 0.05f);
            
            // Golden-yellow particles
            float r = 1.0f;
            float g = 0.9f - ring * 0.2f;
            float b = 0.4f - ring * 0.1f;
            
            createParticle(x, y, vx, vy, r, g, b, 0.8f, ringLifetime, 0.03f, angle * 57.3f, 60.0f);
        }
    }
    
    // Limit white sparkles
    const int maxSparkles = 20; // 50'den 20'ye düşür
    for (int i = 0; i < maxSparkles; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float dist = (rand() % 100) / 200.0f; // 0 to 0.5
        
        float x = cos(angle) * dist;
        float y = sin(angle) * dist;
        
        // Velocity vector outward from center
        float speed = 0.05f + (rand() % 100) / 500.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed;
        
        float size = 0.01f + (rand() % 100) / 2000.0f;
        float lifetime = 0.5f + (rand() % 100) / 200.0f;
        
        // White sparkle
        float whiteness = 0.8f + (rand() % 20) / 100.0f; // 0.8 to 1.0
        createParticle(x, y, vx, vy, whiteness, whiteness, whiteness, 0.9f, lifetime, size);
    }
}

// Effect for collecting extra life
void createHeartEffect(float x, float y) {
    // Create heart-shaped particles
    for (int i = 0; i < 20; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.05f + (rand() % 100) / 1000.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed + 0.01f; // Slight upward bias
        
        float size = 0.01f + (rand() % 100) / 5000.0f;
        float lifetime = 1.0f + (rand() % 100) / 200.0f;
        
        // Heart-shaped particles are red/pink
        createParticle(x, y, vx, vy, 1.0f, 0.2f + (rand() % 50) / 100.0f, 0.4f, 
                      1.0f, lifetime, size, 0, (rand() % 200) - 100);
    }
}

// Effect for shield breaking
void createShieldBreakEffect(float x, float y) {
    const int numParticles = 30;
    const float radius = 0.15f;
    
    for (int i = 0; i < numParticles; i++) {
        float angle = (i * 360.0f / numParticles) * 3.14159f / 180.0f;
        
        // Particles start at shield radius
        float startX = x + cos(angle) * radius;
        float startY = y + sin(angle) * radius;
        
        // Velocity outward
        float speed = 0.1f + (rand() % 100) / 500.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed;
        
        // Shield particles are blue/cyan
        createParticle(startX, startY, vx, vy, 0.3f, 0.8f, 1.0f, 
                      0.8f, 0.5f, 0.02f, rand() % 360, (rand() % 400) - 200);
    }
}

// Massive explosion effect
void createMassiveExplosion(float x, float y, float radius) {
    // First, create a bright flash
    for (int i = 0; i < 50; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float distance = (rand() % 100) / 100.0f * radius;
        float startX = x + cos(angle) * distance;
        float startY = y + sin(angle) * distance;
        
        // Flash particles - bright white/yellow and short-lived
        createParticle(startX, startY, 0, 0, 1.0f, 1.0f, 0.8f, 
                      0.9f, 0.2f, 0.05f + (rand() % 100) / 1000.0f);
    }
    
    // Then create explosion debris
    for (int i = 0; i < 100; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.1f + (rand() % 200) / 500.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed;
        
        // Explosion particles - red/orange with longer lifetime
        float r = 0.8f + (rand() % 20) / 100.0f;
        float g = 0.3f + (rand() % 40) / 100.0f;
        float b = 0.0f;
        
        createParticle(x, y, vx, vy, r, g, b, 
                      1.0f, 1.0f, 0.02f + (rand() % 100) / 2000.0f, 
                      rand() % 360, (rand() % 400) - 200);
    }
    
    // Add smoke particles that linger
    for (int i = 0; i < 40; i++) {
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.03f + (rand() % 100) / 2000.0f;
        float vx = cos(angle) * speed;
        float vy = sin(angle) * speed + 0.01f; // Slight upward drift
        
        float gray = 0.2f + (rand() % 60) / 100.0f;
        
        // Create larger, slower smoke particles
        createParticle(x, y, vx, vy, gray, gray, gray, 
                      0.7f, 2.0f + (rand() % 100) / 100.0f, 
                      0.04f + (rand() % 100) / 1000.0f, 
                      rand() % 360, (rand() % 100) - 50);
    }
}

// Set block movement patterns based on level in resetGame() function
void resetGame() {
    playerX = 0.0f;
    score = 0;
    health = 3;
    level = 1;
    blockSpeed = 0.01f;
    gameOver = false;
    blocks.clear();
    powerUps.clear();  
    isInvisible = false;
    invisibilityTimer = 0.0f;
    playerSpeed = originalPlayerSpeed;
    backgroundColor = 0.0f;
    colorIncreasing = true;
    hasSpeedBoost = false;
    speedBoostTimer = 0.0f;
    hasBlockReset = false;
    blockResetTimer = 0.0f;
    hasTimeSlow = false;
    timeSlowTimer = 0.0f;
    timeSlowFactor = 1.0f;
    hasShield = false;
    shieldTimer = 0.0f;
    
    // Parçacıkları temizle
    particles.clear();

    // Create blocks with level-based movement pattern assignment in resetGame()
    for (int i = 0; i < 3; i++) {
        float r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f; // Predominantly red color
        float g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f; 
        float b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
        
        float xPos = (rand() % 200 - 100) / 100.0f;
        
        // Only linear movement (0) before level 3
        int movementPattern = 0; // Always linear initially
        
        blocks.push_back({
            xPos,                      // x
            1.0f,                      // y
            rand() % 3,                // shape (0, 1, or 2)
            r, g, b,                   // color
            movementPattern,           // movement pattern (always 0 initially)
            0.0f,                      // movement timer
            xPos                       // origin X position
        });
    }

    std::cout << "Game Reset! New game started!" << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Forward declarations
void drawText(const std::string& text, float x, float y, float size, float r, float g, float b);
bool loadFont();
void renderText(const std::string& text, float x, float y, float scale, float r, float g, float b);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (!gameStarted && key == GLFW_KEY_ENTER) {
            gameStarted = true;
            resetGame(); // We call this function
            powerUps.clear();  // Clear all power-ups
            fadeInEffect = true;
            fadeAlpha = 1.0f;
            sigma.play();
        } 
        else if (gameOver && key == GLFW_KEY_ENTER) {
            resetGame();
            powerUps.clear(); 
            fadeInEffect = true;
            fadeAlpha = 1.0f;
            sigma.play();
        }
        else if (gameStarted && !gameOver) {
            if (key == GLFW_KEY_P) {
                isPaused = !isPaused;
                if (isPaused) {
                    sigma.pause();
                    std::cout << "Game Paused" << std::endl;
                } else {
                    sigma.play();
                    std::cout << "Game Resumed" << std::endl;
                }
            }
            else if (key == GLFW_KEY_M) {
                isMuted = !isMuted;
                if (isMuted) {
                    previousVolume = sigma.getVolume();
                    sigma.setVolume(0.0f);
                    collisionSound.setVolume(0.0f);
                    powerUpSound.setVolume(0.0f);
                    levelUpSound.setVolume(0.0f);
                    gameOverSound.setVolume(0.0f);
                    std::cout << "Sound muted" << std::endl;
                } else {
                    sigma.setVolume(previousVolume);
                    collisionSound.setVolume(100.0f);
                    powerUpSound.setVolume(100.0f);
                    levelUpSound.setVolume(100.0f);
                    gameOverSound.setVolume(100.0f);
                    std::cout << "Sound unmuted" << std::endl;
                }
            }
            
            // Player movement in active game
            if (!isPaused) {
                if (key == GLFW_KEY_LEFT && playerX > -0.9f) playerX -= playerSpeed;
                if (key == GLFW_KEY_RIGHT && playerX < 0.9f) playerX += playerSpeed;
            }
        }
    }
}

void drawRectangle(float x, float y, float width, float height, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y - height);
        glVertex2f(x, y - height);
    glEnd();
}

bool loadFont() {
    // Create a pixel array for a simple monospaced font
    unsigned char* fontData = new unsigned char[FONT_TEXTURE_WIDTH * FONT_TEXTURE_HEIGHT * 4];
    
    // Clear font texture to transparent black
    for (int i = 0; i < FONT_TEXTURE_WIDTH * FONT_TEXTURE_HEIGHT * 4; i += 4) {
        fontData[i] = 255;     // R
        fontData[i+1] = 255;   // G
        fontData[i+2] = 255;   // B
        fontData[i+3] = 0;     // A (transparent)
    }
    
    // Simple bitmap font
    const int charWidth = 16;
    const int charHeight = 24;
    const int charsPerRow = FONT_TEXTURE_WIDTH / charWidth;
    
    // Draw basic characters
    for (int c = 32; c < 128; c++) {
        int row = (c - 32) / charsPerRow;
        int col = (c - 32) % charsPerRow;
        
        int startX = col * charWidth;
        int startY = row * charHeight;
        
        // Draw character
        for (int y = 0; y < charHeight; y++) {
            for (int x = 0; x < charWidth; x++) {
                int pixelPos = ((startY + y) * FONT_TEXTURE_WIDTH + (startX + x)) * 4;
                
                // Simple algorithm to draw letter shapes
                bool isPixelSet = false;
                
                switch (c) {
                    case 'A':
                    case 'a':
                        isPixelSet = (x == 0 || x == charWidth-1 || y == 0 || y == charHeight/2);
                        break;
                    case 'B':
                    case 'b':
                        isPixelSet = (x == 0 || 
                                    (y == 0 && x < charWidth-2) || 
                                    (y == charHeight-1 && x < charWidth-2) ||
                                    (y == charHeight/2 && x < charWidth-2) || 
                                    (x == charWidth-2 && (y > 0 && y < charHeight/2-1)) ||
                                    (x == charWidth-2 && y > charHeight/2+1 && y < charHeight-1));
                        break;
                    case 'C':
                    case 'c':
                        isPixelSet = ((x == 0 && y > 0 && y < charHeight-1) || 
                                    (y == 0 && x > 0) || 
                                    (y == charHeight-1 && x > 0));
                        break;
                    default:
                        // For other characters, create a simple placeholder
                        isPixelSet = (x == 0 || y == 0 || x == charWidth-1 || y == charHeight-1 || 
                                     x == y || x == charWidth-y);
                }
                
                if (isPixelSet) {
                    fontData[pixelPos] = 255;     // R
                    fontData[pixelPos+1] = 255;   // G
                    fontData[pixelPos+2] = 255;   // B
                    fontData[pixelPos+3] = 255;   // A (opaque)
                }
            }
        }
        
        // Add character to mapping
        Character ch;
        ch.advanceX = charWidth;
        ch.advanceY = 0;
        ch.width = charWidth;
        ch.height = charHeight;
        ch.texX = (float)startX / FONT_TEXTURE_WIDTH;
        ch.texY = (float)startY / FONT_TEXTURE_HEIGHT;
        ch.texWidth = (float)charWidth / FONT_TEXTURE_WIDTH;
        ch.texHeight = (float)charHeight / FONT_TEXTURE_HEIGHT;
        
        characters[c] = ch;
    }
    
    // Create the texture
    glGenTextures(1, &fontTextureID);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_TEXTURE_WIDTH, FONT_TEXTURE_HEIGHT, 0, 
                GL_RGBA, GL_UNSIGNED_BYTE, fontData);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Clean up
    delete[] fontData;
    
    fontLoaded = true;
    return true;
}

void renderText(const std::string& text, float x, float y, float scale, float r, float g, float b) {
    if (!fontLoaded) {
        if (!loadFont()) {
            // If font loading fails, fall back to rectangle-based text
            drawText(text, x, y, scale, r, g, b);
            return;
        }
    }
    
    // Enable texturing and blending
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTextureID);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set text color
    glColor3f(r, g, b);
    
    float currentX = x;
    float currentY = y;
    
    // Render each character
    for (char c : text) {
        // Newline handling
        if (c == '\n') {
            currentY -= 30.0f * scale;
            currentX = x;
            continue;
        }
        
        // Space handling
        if (c == ' ') {
            currentX += 16.0f * scale;
            continue;
        }
        
        // Skip characters not in our font
        if (characters.find(c) == characters.end()) {
            currentX += 16.0f * scale;
            continue;
        }
        
        Character ch = characters[c];
        
        float xpos = currentX;
        float ypos = currentY - ch.height * scale;
        
        // Render the character quad
        glBegin(GL_QUADS);
            glTexCoord2f(ch.texX, ch.texY + ch.texHeight);
            glVertex2f(xpos, ypos);
            
            glTexCoord2f(ch.texX + ch.texWidth, ch.texY + ch.texHeight);
            glVertex2f(xpos + ch.width * scale, ypos);
            
            glTexCoord2f(ch.texX + ch.texWidth, ch.texY);
            glVertex2f(xpos + ch.width * scale, ypos + ch.height * scale);
            
            glTexCoord2f(ch.texX, ch.texY);
            glVertex2f(xpos, ypos + ch.height * scale);
        glEnd();
        
        // Advance cursor
        currentX += ch.advanceX * scale;
        currentY += ch.advanceY * scale;
    }
    
    // Disable texture and blending when done
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void drawPowerUp(const PowerUp& powerUp) {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    switch (powerUp.type) {
        case 1: // Speed - Green
            g = 1.0f;
            break;
        case 2: // Block reset - Blue
            b = 1.0f;
            break;
        case 3: // Invisibility - Yellow
            r = 1.0f;
            g = 1.0f;
            break;
        case 4: // Time slow - Cyan
            g = 1.0f;
            b = 1.0f;
            break;
        case 5: // Shield - Magenta
            r = 1.0f;
            b = 1.0f;
            break;
        case 6: // Extra life - Bright Pink/Purple
            r = 1.0f;
            g = 0.2f;  // Add a little green
            b = 0.8f;  // Add blue for purple tones
            break;
    }
    
    // Extra life için özel parıltı efekti => // Special glow effect for Extra Life
    if (powerUp.type == 6) {
        // Parlak bir arka plan çizimi => // Draw a bright background
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(r, g, b, 0.3f);
        
        // Extra Life için daha büyük bir arka plan hale => // Larger background halo for Extra Life
        float size = 0.12f;
        glBegin(GL_QUADS);
            glVertex2f(powerUp.x - (size-0.08f)/2, powerUp.y + (size-0.08f)/2);
            glVertex2f(powerUp.x + 0.08f + (size-0.08f)/2, powerUp.y + (size-0.08f)/2);
            glVertex2f(powerUp.x + 0.08f + (size-0.08f)/2, powerUp.y - 0.08f - (size-0.08f)/2);
            glVertex2f(powerUp.x - (size-0.08f)/2, powerUp.y - 0.08f - (size-0.08f)/2);
        glEnd();
        
        glDisable(GL_BLEND);
    }
    
    // Normal power-up çizimi => // Normal power-up drawing
    drawRectangle(powerUp.x, powerUp.y, 0.08f, 0.08f, r, g, b);
    
    // Extra life için kalp sembolü ekle => // Add heart symbol for Extra Life
    if (powerUp.type == 6) {
        // Kalp şekli için kırmızı renk => // Red color for heart shape
        glColor3f(1.0f, 0.0f, 0.0f);
        
        // Basit bir kalp şekli (üstte iki yarım daire, altta üçgen) => // Simple heart shape (two half circles on top, triangle at bottom)
        float centerX = powerUp.x + 0.04f;
        float centerY = powerUp.y - 0.04f;
        float size = 0.03f;
        
        // Sol yarım daire => // Left half circle
        const int segments = 10;
        glBegin(GL_POLYGON);
            for (int i = 0; i <= segments; i++) {
                float angle = i * 3.14159f / segments;
                float x = centerX - size/2 + cos(angle) * size/2;
                float y = centerY + sin(angle) * size/2;
                glVertex2f(x, y);
            }
        glEnd();
        
        // Sağ yarım daire => // Right half circle 
        glBegin(GL_POLYGON);
            for (int i = 0; i <= segments; i++) {
                float angle = i * 3.14159f / segments;
                float x = centerX + size/2 + cos(angle) * size/2;
                float y = centerY + sin(angle) * size/2;
                glVertex2f(x, y);
            }
        glEnd();
        
        // Alt üçgen => // Bottom triangle
        glBegin(GL_TRIANGLES);
            glVertex2f(centerX - size, centerY);
            glVertex2f(centerX + size, centerY);
            glVertex2f(centerX, centerY - size*1.5f);
        glEnd();
    }
}

void updateWindowTitle(GLFWwindow* window) {
    std::ostringstream title;
    if (!gameStarted) {
        title << "Welcome to the Game! Press ENTER.";
    } else if (gameOver) {
        title << "Game Over! Score: " << score << " | Press ENTER to Restart";
    } else if (isPaused) {
        title << "PAUSED | Press P to Resume";
    } else {
        title << "Avoidance Game | Level: " << level << " | Score: " << score << " | Health: " << health;
    }
    glfwSetWindowTitle(window, title.str().c_str());
}

// Create a function to draw text (basic approach with rectangles)
void drawText(const std::string& text, float x, float y, float size, float r, float g, float b) {
    float spacing = size * 0.6f;
    
    for (char c : text) {
        if (c == ' ') {
            x += spacing;
            continue;
        }
        
        switch (c) {
            case 'A':
                drawRectangle(x, y, size/8, size, r, g, b);
                drawRectangle(x + size/8*7, y, size/8, size, r, g, b);
                drawRectangle(x, y, size, size/8, r, g, b);
                drawRectangle(x, y - size/2, size, size/8, r, g, b);
                break;
            default:
                drawRectangle(x, y, size/2, size, r, g, b);
                break;
        }
        x += spacing;
    }
}

void drawTriangle(float x, float y, float size, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLES);
        glVertex2f(x, y);
        glVertex2f(x + size, y);
        glVertex2f(x + size/2, y - size);
    glEnd();
}

// Fix circle drawing function
void drawCircle(float x, float y, float radius, float r, float g, float b) {
    const int segments = 20;
    const float fullCircle = 2.0f * 3.14159f;
    
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        // Center point - make sure this is within bounds
        float centerX = x + radius/2;
        float centerY = y - radius/2;
        glVertex2f(centerX, centerY);
        
        // Circle outline points
        for (int i = 0; i <= segments; i++) {
            float angle = i * fullCircle / segments;
            float px = centerX + cos(angle) * radius/2;
            float py = centerY + sin(angle) * radius/2;
            glVertex2f(px, py);
        }
    glEnd();
}

// Change block color to visually indicate difficulty
void drawBlock(const Block& block) {
    // Color modification based on movement pattern
    float r = block.r;
    float g = block.g;
    float b = block.b;
    
    // Increase brightness for more difficult movement patterns
    if (block.movementPattern > 0) {
        // Slightly brighten color for zigzag and circular movements
        float brightnessFactor = 1.0f + (block.movementPattern * 0.2f);
        r = std::min(1.0f, r * brightnessFactor);
        g = std::min(1.0f, g * brightnessFactor);
        b = std::min(1.0f, b * brightnessFactor);
    }
    
    switch (block.shape) {
        case 0: // Square
            drawRectangle(block.x, block.y, 0.1f, 0.1f, r, g, b);
            break;
        case 1: // Triangle
            drawTriangle(block.x, block.y, 0.1f, r, g, b);
            break;
        case 2: // Circle
            drawCircle(block.x, block.y, 0.1f, r, g, b);
            break;
    }
}

// Fix the block movement function to prevent potential out-of-bounds issues
void updateBlockMovement(Block& block) {
    // Level 3'e özel güvenlik kontrolü => // Special security check for Level 3
    if (level == 3 && block.movementPattern != 0) {
        // Level 3'te tüm blokları doğrusal hareket ettir => // Make all blocks move linearly in Level 3
        block.movementPattern = 0;
    }

    try {
        // Update position based on movement pattern
        switch (block.movementPattern) {
            case 0: // Linear - just move down
                // y position is updated in the main loop
                break;
                
            case 1: // Zigzag - horizontal sine wave (daha az agresif) => // Zigzag - horizontal sine wave (less aggressive) 
                // Make sure the block doesn't go off screen
                block.x = block.originX + sin(block.movementTimer * 2.0f) * 0.2f; // Daha az genlik => // Less amplitude
                // Clamp to screen boundaries
                if (block.x < -0.95f) block.x = -0.95f;
                if (block.x > 0.95f) block.x = 0.95f;
                block.movementTimer += 0.01f; // Daha yavaş => // Slower
                break;
                
            case 2: // Circular - orbit around a center point
                // Circular hareketi devre dışı bırak - hata kaynağı olabilir => // Disable circular movement - could be source of errors
                block.movementPattern = 1;
                block.x = block.originX + sin(block.movementTimer * 2.0f) * 0.15f;
                block.movementTimer += 0.01f;
                break;
                
            default:
                // Geçersiz bir hareket paterni için güvenli davranış => // Safe behavior for invalid movement pattern
                block.movementPattern = 0;
                break;
        }
    }
    catch (...) {
        // Herhangi bir hata durumunda güvenli değerler ayarla => // Set safe values in case of any error
        block.movementPattern = 0; // Doğrusal harekete zorla => // Force linear movement
    }
}

void drawBackgroundEffects() {
    // Stars in the background - statik değişkeni yerel değişkenle değiştir
    static std::vector<std::tuple<float, float, float>> stars;
    
    // Bu fonksiyon içindeki static değişkenler sorun çıkarabilir
    // Özellikle background lines kısmında
    // Especially in the background lines section
    
    // Level-based background effects için daha güvenli kod
    if (level >= 5) {
        static float lineTime = 0.0f; // Bu tek değişken olsun
        lineTime += 0.0005f;
        
        const int maxLines = 3; // 5'ten 3'e düşür
        for (int i = 0; i < maxLines; i++) {
            float y1 = -1.0f + (sin(lineTime + i) + 1) * 0.5f;
            float y2 = -1.0f + (sin(lineTime + i + 3.14159f) + 1) * 0.5f;
            
            float r = 0.3f + sin(lineTime * 2.5f + i) * 0.2f;
            float g = 0.2f + cos(lineTime * 1.7f + i) * 0.1f;
            float b = 0.5f + sin(lineTime * 3.1f + i) * 0.2f;
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(r, g, b, 0.2f);
            
            glBegin(GL_LINES);
                glVertex2f(-1.0f, y1);
                glVertex2f(1.0f, y2);
            glEnd();
            
            glDisable(GL_BLEND);
        }
    }
}

// cleanup fonksiyonunu değiştirin:
void cleanup() {
    // Vektörleri temizle => // Clear vectors
    blocks.clear();
    powerUps.clear();
    particles.clear();
    
    // SFML seslerini temizle => // Clear SFML sounds
    collisionSound.stop();
    powerUpSound.stop();
    levelUpSound.stop();
    gameOverSound.stop();
    sigma.stop();
    
    // OpenGL kaynaklarını temizle => // Clear OpenGL resources
    if (fontTextureID != 0) {
        glDeleteTextures(1, &fontTextureID);
        fontTextureID = 0;
    }
    
    // Zamanlanmış değişkenleri sıfırla => // Reset timed variables
    timeSlowTimer = 0.0f;
    speedBoostTimer = 0.0f;
    invisibilityTimer = 0.0f;
    blockResetTimer = 0.0f;
    shieldTimer = 0.0f;
}

int main() {
    srand(time(0));
    
    // Define sound directory path
    std::filesystem::path soundPath;
    
    // Check common locations for sound files
    if (std::filesystem::exists("sounds")) {
        soundPath = "sounds";
    } else if (std::filesystem::exists("../sounds")) {
        soundPath = "../sounds";
    } else if (std::filesystem::exists("../../sounds")) {
        soundPath = "../../sounds";
    } else {
        // Try to create sounds directory
        try {
            std::filesystem::create_directory("sounds");
            soundPath = "sounds";
            std::cout << "Created sounds directory at: " << std::filesystem::absolute(soundPath).string() << std::endl;
        } catch(const std::exception& e) {
            std::cerr << "Error creating sounds directory: " << e.what() << std::endl;
            std::cerr << "Please create a 'sounds' folder in the executable directory." << std::endl;
            return -1;
        }
    }
    
    std::cout << "Using sound directory: " << std::filesystem::absolute(soundPath).string() << std::endl;
    
    // Check if sound directory exists and contains required files
    if (!std::filesystem::exists(soundPath) || 
        !std::filesystem::exists(soundPath / "collision.wav") ||
        !std::filesystem::exists(soundPath / "pickup.wav") ||
        !std::filesystem::exists(soundPath / "levelup.wav") ||
        !std::filesystem::exists(soundPath / "gameover.wav") ||
        !std::filesystem::exists(soundPath / "sigma.wav")) {
        
        std::cerr << "Sound directory missing required files!" << std::endl;
        // Enable SFML error messages
        std::cerr << "Please place the following WAV files in " << soundPath.string() << ":" << std::endl;
        std::cerr << "- collision.wav\n- pickup.wav\n- levelup.wav\n- gameover.wav\n- background.wav\n- sigma.wav" << std::endl;
        return -1;
    }

    // Load sound files
    if (!collisionBuffer.loadFromFile((soundPath / "collision.wav").string())) {
        std::cerr << "Collision sound failed to load: " << (soundPath / "collision.wav").string() << std::endl;
        std::cerr << "File size: " << std::filesystem::file_size(soundPath / "collision.wav") << " bytes" << std::endl;
    } else {
        std::cout << "Collision sound loaded successfully!" << std::endl;
        std::cout << "Sample rate: " << collisionBuffer.getSampleRate() << std::endl;
        std::cout << "Channel count: " << collisionBuffer.getChannelCount() << std::endl;
        std::cout << "Duration: " << collisionBuffer.getDuration().asSeconds() << " seconds" << std::endl;
        collisionSound.setBuffer(collisionBuffer);
    }

    if (!powerUpBuffer.loadFromFile((soundPath / "pickup.wav").string())) {
        std::cerr << "Power-up sound failed to load: " << (soundPath / "pickup.wav").string() << std::endl;
        return -1;
    } else {
        powerUpSound.setBuffer(powerUpBuffer);
    }

    if (!levelUpBuffer.loadFromFile((soundPath / "levelup.wav").string())) {
        std::cerr << "Level-up sound failed to load: " << (soundPath / "levelup.wav").string() << std::endl;
        return -1;
    } else {
        levelUpSound.setBuffer(levelUpBuffer);
    }

    if (!gameOverBuffer.loadFromFile((soundPath / "gameover.wav").string())) {
        std::cerr << "Game-over sound failed to load: " << (soundPath / "gameover.wav").string() << std::endl;
        return -1;
    } else {
        gameOverSound.setBuffer(gameOverBuffer);
    }

    // Load background music
    if (!sigma.openFromFile((soundPath / "sigma.wav").string())) {
        std::cerr << "Background music failed to load: " << (soundPath / "sigma.wav").string() << std::endl;
        return -1;
    } else {
        sigma.setVolume(30.0f);
        sigma.play();
    }

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Avoidance Game", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // Initial state: game should be not started
    gameStarted = false;
    gameOver = false;
    powerUps.clear();
    blocks.clear(); // Clear the blocks
    // When game is first launched, just set variables instead of calling resetGame
    // resetGame(); - Remove this call

    // Daha agresif exception handling => // More aggressive exception handling
    while (!glfwWindowShouldClose(window)) {
        try {
            // Main loop başlangıcında vektörleri kontrol et ve sınırla => // Check and limit vectors at the beginning of main loop
            if (blocks.size() > MAX_BLOCKS) {
                blocks.resize(MAX_BLOCKS);
            }
            
            if (powerUps.size() > 10) {
                powerUps.resize(10);
            }
            
            if (particles.size() > MAX_PARTICLES) {
                particles.resize(MAX_PARTICLES);
            }

            // Mevcut main loop kodu => // Current main loop code
            // Music control
            if (gameStarted && !isPaused && !gameOver && sigma.getStatus() != sf::Music::Status::Playing) {
                sigma.play();
            }
            
            // Background color animation
            if (colorIncreasing) {
                backgroundColor += 0.001f; 
            } else {
                backgroundColor -= 0.001f;
            }
            
            if (backgroundColor >= 1.0f) {
                colorIncreasing = false;
            }
            if (backgroundColor <= 0.0f) {
                colorIncreasing = true;
            }

            // Dynamic background color
            glClearColor(
                backgroundColor * 0.2f,
                backgroundColor * 0.1f,
                0.3f + backgroundColor * 0.2f,
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT);

            updateWindowTitle(window);

            // Game state handling
            if (!gameStarted) {
                // Welcome screen - just blue background
                // Nothing will be drawn
            }
            else if (gameOver) {
                // Game over screen - just blue background
                // Nothing will be drawn
            }
            else if (!isPaused) {
                // Active gameplay
                // Draw player
                if (!isInvisible) {
                    drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f);
                } else {
                    glEnable(GL_BLEND);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    glColor4f(0.0f, 1.0f, 0.0f, 0.5f); // Semi-transparent green
                    glBegin(GL_QUADS);
                        glVertex2f(playerX, -0.8f);
                        glVertex2f(playerX + 0.1f, -0.8f);
                        glVertex2f(playerX + 0.1f, -0.9f);
                        glVertex2f(playerX, -0.9f);
                    glEnd();
                    glDisable(GL_BLEND);
                }

                // PowerUp oluşturma kodu - seviye bazlı ihtimal artışı => // PowerUp creation code - level-based probability increase
                if (gameStarted && !gameOver) {
                    // Seviyeye bağlı olarak düşme ihtimalini belirle => // Determine drop probability based on level
                    int powerUpChance;
                    if (level <= 3) {
                        powerUpChance = 500; // 1/500 ihtimal (seviye 3 ve öncesi)
                    } else if (level <= 5) {
                        powerUpChance = 300; // 1/300 ihtimal (seviye 4-5)
                    } else if (level <= 8) {
                        powerUpChance = 200; // 1/200 ihtimal (seviye 6-8)
                    } else {
                        powerUpChance = 100; // 1/100 ihtimal (seviye 9+)
                    }

                    // Power-up oluştur => // Create power-up
                    if (rand() % powerUpChance == 0 && powerUps.size() < 10) { // Power-up sayısını sınırlama ekle
                        // Power-up türü dağılımını seviyeye göre ayarla => // Adjust power-up type distribution based on level
                        int powerUpType;
                        int r = rand() % 100;
                        
                        if (level <= 3) {
                            // Temel power-up'lar daha yaygın (1-3) => // Basic power-ups more common (1-3)
                            powerUpType = (r < 80) ? (rand() % 3 + 1) : (rand() % 3 + 4);
                        } else if (level <= 6) {
                            // Dağılım biraz daha dengeli => // Distribution is more balanced
                            powerUpType = (r < 60) ? (rand() % 3 + 1) : (rand() % 3 + 4);
                        } else {
                            // Gelişmiş power-up'lar daha yaygın (4-6) => // Advanced power-ups more common (4-6)
                            powerUpType = (r < 40) ? (rand() % 3 + 1) : (rand() % 3 + 4);
                        }
                        
                        powerUps.push_back({
                            (rand() % 200 - 100) / 100.0f, 
                            1.0f, 
                            powerUpType, // Belirlenen power-up türü => // Determined power-up type
                            5.0f  // 5 saniye süre
                        });
                    }
                }

                // PowerUp güncelleme kodunu değiştirin => // Update the PowerUp code
                for (auto it = powerUps.begin(); it != powerUps.end();) {
                    // PowerUp'ı güvenli sınırlar içinde tut => // Keep PowerUp within safe boundaries
                    if (it->y < -1.5f || it->y > 1.5f || it->x < -1.5f || it->x > 1.5f) {
                        it = powerUps.erase(it);
                        continue;
                    }
                    
                    it->y -= blockSpeed * (hasTimeSlow ? timeSlowFactor : 1.0f);
                    
                    // PowerUp çizimini try-catch içine al => // Put PowerUp drawing in try-catch block
                    try {
                        drawPowerUp(*it);
                    } catch (...) {
                        std::cerr << "Error drawing powerup" << std::endl;
                    }

                    // Çarpışma algılama => // Collision detection
                    float powerUpCenterX = it->x + 0.04f;
                    float powerUpCenterY = it->y - 0.04f;
                    float playerCenterX = playerX + 0.05f;
                    float playerCenterY = -0.85f;
                    
                    float dx = powerUpCenterX - playerCenterX;
                    float dy = powerUpCenterY - playerCenterY;
                    float distance = sqrt(dx*dx + dy*dy);
                    
                    bool collected = (distance < 0.12f);
                    
                    if (collected) {
                        try {
                            powerUpSound.play();
                            
                            switch (it->type) {
                                case 1: // Speed
                                    hasSpeedBoost = true;
                                    speedBoostTimer = 20.0f;
                                    playerSpeed = originalPlayerSpeed + 0.1f;
                                    break;
                                    
                                case 2: // Block Reset
                                    hasBlockReset = true;
                                    blockResetTimer = 20.0f;
                                    blocks.clear();
                                    // Yeni bir blok ekle => // Add a new block
                                    blocks.push_back({
                                        (rand() % 180 - 90) / 100.0f, // x
                                        1.0f, // y
                                        0, // shape
                                        0.7f, 0.0f, 0.0f, // color
                                        0, // movement
                                        0.0f, // timer
                                        (rand() % 180 - 90) / 100.0f // originX
                                    });
                                    break;
                                    
                                case 3: // Invisibility
                                    isInvisible = true;
                                    invisibilityTimer = 20.0f;
                                    break;
                                    
                                case 4: // Time Slow 
                                    hasTimeSlow = true;
                                    timeSlowTimer = 15.0f;
                                    timeSlowFactor = 0.5f;
                                    break;
                                    
                                case 5: // Shield
                                    hasShield = true;
                                    shieldTimer = 10.0f;
                                    break;
                                    
                                case 6: // Extra Life
                                    health++;
                                    // Sadece kısıtlı sayıda parçacıklar ekle => // Add only a limited number of particles
                                    for (int i = 0; i < 5; i++) {
                                        float angle = (rand() % 360) * 3.14159f / 180.0f;
                                        createParticle(playerX + 0.05f, -0.8f, 
                                                     cos(angle)*0.05f, sin(angle)*0.05f, 
                                                     1.0f, 0.2f, 0.4f, 
                                                     1.0f, 0.5f, 0.02f);
                                    }
                                    break;
                            }
                            
                            it = powerUps.erase(it);
                        }
                        catch (...) {
                            std::cerr << "Error processing powerup" << std::endl;
                            ++it; // Yine de ilerlemeliyiz => // We still need to proceed
                        }
                    }
                    else if (it->y < -1.0f) {
                        it = powerUps.erase(it);
                    }
                    else {
                        ++it;
                    }
                }

                // Update power-up timers
                if (hasSpeedBoost) {
                    speedBoostTimer -= 0.016f;
                    if (speedBoostTimer <= 0) {
                        hasSpeedBoost = false;
                        playerSpeed = originalPlayerSpeed;
                    }
                }

                if (hasBlockReset) {
                    blockResetTimer -= 0.016f;
                    if (blockResetTimer <= 0) {
                        hasBlockReset = false;
                        // Restore normal block generation - doğru bir şekilde blokları oluştur => // Restore normal block generation - create blocks properly
                        blocks.clear(); // İlk önce tüm blokları temizle => // First clear all blocks
                        for (int i = 0; i < level && i < MAX_BLOCKS; i++) {
                            float r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                            float g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            float b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            
                            float xPos = (rand() % 200 - 100) / 100.0f;
                            
                            blocks.push_back({
                                xPos,     // x
                                1.0f + (i * 0.3f), // y - yeni blokların üst üste gelmesini önlemek için aralık bırakın => // leave spacing to prevent new blocks from stacking
                                rand() % 3, // shape
                                r, g, b,    // color
                                (level < 3) ? 0 : rand() % 3, // movement pattern
                                0.0f,       // movement timer
                                xPos        // originX
                            });
                        }
                    }
                }

                if (isInvisible) {
                    invisibilityTimer -= 0.016f;
                    if (invisibilityTimer <= 0) {
                        isInvisible = false;
                    }
                }

                // Handle time slow effect
                if (hasTimeSlow) {
                    timeSlowTimer -= 0.016f;
                    if (timeSlowTimer <= 0) {
                        hasTimeSlow = false;
                        timeSlowFactor = 1.0f;
                    } else {
                        // Visual effect to show time slow
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glColor4f(0.0f, 0.4f, 0.8f, 0.2f);
                        glBegin(GL_QUADS);
                            glVertex2f(-1.0f, 1.0f);
                            glVertex2f(1.0f, 1.0f);
                            glVertex2f(1.0f, -1.0f);
                            glVertex2f(-1.0f, -1.0f);
                        glEnd();
                        glDisable(GL_BLEND);
                    }
                }

                // Handle shield power-up
                if (hasShield) {
                    shieldTimer -= 0.016f;
                    if (shieldTimer <= 0) {
                        hasShield = false;
                    } else {
                        // Draw shield around player
                        const int segments = 20;
                        const float fullCircle = 2.0f * 3.14159f;
                        
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        glColor4f(0.3f, 0.8f, 1.0f, 0.5f);
                        glBegin(GL_TRIANGLE_FAN);
                            glVertex2f(playerX + 0.05f, -0.85f);
                            float radius = 0.15f;
                            for (int i = 0; i <= segments; i++) {
                                float angle = i * fullCircle / segments;
                                float px = playerX + 0.05f + cos(angle) * radius;
                                float py = -0.85f + sin(angle) * radius;
                                glVertex2f(px, py);
                            }
                        glEnd();
                        glDisable(GL_BLEND);
                        
                        // Note: Collision handling is now moved inside the block loop
                        // where the collision variable is defined
                    }
                }

                // In main game loop, use for drawing and updating blocks
                for (auto& block : blocks) {
                    // Update block's y position (common for all blocks)
                    block.y -= blockSpeed * (hasTimeSlow ? timeSlowFactor : 1.0f);
                    
                    // Update block's movement based on pattern
                    updateBlockMovement(block);
                    
                    // Draw block
                    drawBlock(block);

                    // Code at the end of block's fall in main game loop (block.y < -1.0f condition)
                    if (block.y < -1.0f) {
                        float xPos = (rand() % 200 - 100) / 100.0f;
                        block.x = xPos;
                        block.y = 1.0f;
                        block.originX = xPos; // Set new origin X
                        // Assign new shape, color and movement pattern
                        block.shape = rand() % 3;
                        block.r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                        block.g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                        block.b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                        
                        // Only linear movement (0) until level 3
                        block.movementPattern = (level < 3) ? 0 : rand() % 3;
                        block.movementTimer = 0.0f;
                        
                        // Blok düşüşü sonrası puan güncellemesi => // Score update after block drop
                        if (!gameOver) {
                            score++;
                            
                            // Level up kodunu daha da güvenceye al - main.cpp dosyasındaki level up işlemi için => // Make level up code even more secure - for level up process in main.cpp
                            if (score > 0 && score % SCORE_PER_LEVEL == 0) {
                                try {
                                    std::cout << "Level up! Score: " << score << ", New level: " << level + 1 << std::endl;
                                    level++;
                                    blockSpeed += LEVEL_SPEED_INCREASE;
                                    
                                    // Öncelikle, tüm efektleri ve parçacıkları temizle - bu önemli! => // First, clear all effects and particles - this is important!
                                    particles.clear();
                                    
                                    // Level up sound güvenli bir şekilde çal => // Play level up sound safely
                                    levelUpSound.play();
                                    
                                    // Parçacık efekti yaratmayı basitleştir ve sınırla => // Simplify and limit particle effect creation
                                    float centerX = 0.0f;
                                    float centerY = 0.0f;
                                    
                                    // Sadece 5 basit parçacık yarat => // Create only 5 simple particles
                                    for (int i = 0; i < 5; i++) {
                                        float angle = (i * 360.0f / 5) * 3.14159f / 180.0f;
                                        float vx = cos(angle) * 0.1f;
                                        float vy = sin(angle) * 0.1f;
                                        
                                        // Basit sarı parçacıklar => // Simple yellow particles
                                        createParticle(centerX, centerY, vx, vy, 
                                                      1.0f, 1.0f, 0.0f, // yellow
                                                      1.0f, 0.5f, 0.03f); // alpha, lifetime, size
                                    }
                                    
                                    // Yeni blok ekleme - eğer level 3'e geçiyorsak dikkatli olalım => // Add new block - be careful if transitioning to level 3
                                    if (level == 3) {
                                        // Level 3'e geçişte özel güvenlik kontrolü => // Special security check when transitioning to level 3
                                        std::cout << "Transitioning to level 3 (special handling)" << std::endl;
                                        
                                        // Mevcut blokları güvenceye al - çok fazla blok varsa sil => // Secure existing blocks - delete if there are too many
                                        if (blocks.size() > MAX_BLOCKS / 2) {
                                            blocks.resize(MAX_BLOCKS / 2);
                                        }
                                        
                                        // Sadece bir adet basit blok ekle => // Add just one simple block
                                        float xPos = 0.0f; // Merkeze yakın güvenli bir pozisyon => // Safe position near center
                                        blocks.push_back({
                                            xPos,      // x
                                            1.0f,      // y
                                            0,         // shape - square (simplest)
                                            1.0f, 0.0f, 0.0f, // red
                                            0,         // hareket - doğrusal (en basit) => // movement - linear (simplest)
                                            0.0f,      // timer
                                            xPos       // originX
                                        });
                                    }
                                    // Diğer levellar için normal blok eklemeyi kullan => // Use normal block addition for other levels
                                    else if (blocks.size() < MAX_BLOCKS) {
                                        float xPos = (rand() % 180 - 90) / 100.0f;
                                        blocks.push_back({
                                            xPos, 1.0f, 
                                            0, // shape
                                            0.7f, 0.0f, 0.0f, // color
                                            (level < 3) ? 0 : (rand() % 2), // Bazı hareket çeşitlerini sınırla => // Limit some movement types
                                            0.0f, xPos
                                        });
                                    }
                                }
                                catch (const std::exception& e) {
                                    std::cerr << "CRITICAL - Level up exception: " << e.what() << std::endl;
                                    // Kritik hata - minimum güvenlik önlemleri => // Critical error - minimum security measures
                                    level++; // Yine de level'ı artır => // Still increase the level
                                }
                                catch (...) {
                                    std::cerr << "CRITICAL - Unknown level up exception" << std::endl;
                                    level++; // Yine de level'ı artır => // Still increase the level
                                }
                            } else {
                                blockSpeed += NORMAL_SPEED_INCREASE;
                            }
                        }
                    }

                    // Adjust collision detection based on different shapes
                    bool collision = false;

                    // Çarpışma algılaması öncesi sınır kontrolü => // Boundary check before collision detection
                    if (block.y >= -1.5f && block.y <= 1.5f && 
                        block.x >= -1.5f && block.x <= 1.5f) {
                        
                        // Çarpışma kontrolünü basitleştirin => // Simplify collision check
                        float blockCenterX = block.x + 0.05f;
                        float blockCenterY = block.y - 0.05f;
                        float playerCenterX = playerX + 0.05f;
                        float playerCenterY = -0.85f;
                        
                        // İki merkezin uzaklığını hesapla => // Calculate distance between two centers
                        float dx = blockCenterX - playerCenterX;
                        float dy = blockCenterY - playerCenterY;
                        float distance = sqrt(dx*dx + dy*dy);
                        
                        // Basitleştirilmiş çarpışma kontrolü => // Simplified collision detection
                        collision = (distance < 0.1f);
                    }
                    
                    // When resetting block after collision
                    if (collision) {
                        if (!isInvisible) {
                            // Check if shield is active
                            if (hasShield) {
                                // Just disable shield instead of taking damage
                                hasShield = false;
                                shieldTimer = 0.0f;
                                // Shield breaking effect
                                createShieldBreakEffect(playerX + 0.05f, -0.85f);
                            } else {
                                // No shield, take damage
                                health--;
                                collisionSound.play();
                                
                                // Add collision animation
                                createBlockExplosion(block.x + 0.05f, block.y - 0.05f, block.r, block.g, block.b);
                                
                                if (health <= 0) {
                                    gameOverSound.play();
                                    sigma.stop();
                                    gameOver = true;
                                    fadeOutEffect = true;
                                    fadeAlpha = 0.0f;
                                }
                            }
                        }
                        
                        // Reset block position regardless of invisibility or shield
                        float xPos = (rand() % 200 - 100) / 100.0f;
                        block.x = xPos;
                        block.y = 1.0f;
                        block.originX = xPos;
                        
                        // Assign new shape, color and movement pattern
                        block.shape = rand() % 3;
                        // Only linear movement (0) until level 3
                        block.movementPattern = (level < 3) ? 0 : rand() % 3;
                        block.movementTimer = 0.0f;
                        block.r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                        block.g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                        block.b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                    }
                }

                // Update and draw particles
                updateParticles(0.016f);
                drawParticles();
            } else {
                // Paused state
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
                glBegin(GL_QUADS);
                    glVertex2f(-1.0f, 1.0f);
                    glVertex2f(1.0f, 1.0f);
                    glVertex2f(1.0f, -1.0f);
                    glVertex2f(-1.0f, -1.0f);
                glEnd();
                glDisable(GL_BLEND);
                
                renderText("PAUSED", -0.2f, 0.1f, 0.15f, 1.0f, 1.0f, 1.0f);
                renderText("Press P to Resume", -0.4f, -0.1f, 0.08f, 0.8f, 0.8f, 0.8f);
            }

            // Handle fade effects
            if (fadeInEffect) {
                fadeAlpha -= 0.01f;
                if (fadeAlpha <= 0.0f) {
                    fadeAlpha = 0.0f;
                    fadeInEffect = false;
                }
                
                // Draw black overlay
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(0.0f, 0.0f, 0.0f, fadeAlpha);
                glBegin(GL_QUADS);
                    glVertex2f(-1.0f, 1.0f);
                    glVertex2f(1.0f, 1.0f);
                    glVertex2f(1.0f, -1.0f);
                    glVertex2f(-1.0f, -1.0f);
                glEnd();
                glDisable(GL_BLEND);
            }

            if (fadeOutEffect) {
                fadeAlpha += 0.01f;
                if (fadeAlpha >= 1.0f) {
                    fadeAlpha = 1.0f;
                    fadeOutEffect = false;
                }
                
                // Draw black overlay
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(0.0f, 0.0f, 0.0f, fadeAlpha);
                glBegin(GL_QUADS);
                    glVertex2f(-1.0f, 1.0f);
                    glVertex2f(1.0f, 1.0f);
                    glVertex2f(1.0f, -1.0f);
                    glVertex2f(-1.0f, -1.0f);
                glEnd();
                glDisable(GL_BLEND);
            }

            // Debug çıktısını azalt - her karede yazdırma => // Reduce debug output - don't print every frame
            // Bu satırları kaldırın veya yorum haline getirin
            /*
            std::cout << "Blocks: " << blocks.size() 
                    << ", PowerUps: " << powerUps.size() 
                    << ", Particles: " << particles.size() 
                    << ", Health: " << health 
                    << ", Level: " << level 
                    << std::endl;
            */

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        catch (const std::exception& e) {
            std::cerr << "EXCEPTION: " << e.what() << std::endl;
            // Kritik hata - oyunu güvenli bir duruma getir => // Critical error - bring game to a safe state
            blocks.clear();
            powerUps.clear();
            particles.clear();
        }
        catch (...) {
            std::cerr << "UNKNOWN EXCEPTION" << std::endl;
            blocks.clear();
            powerUps.clear();
            particles.clear();
        }
    }

    // Cleanup
    cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}