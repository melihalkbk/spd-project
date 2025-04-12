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
    int type;  // 1 = speed, 2 = block reset, 3 = invisibility
    float duration;  // Power-up duration
};

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

// Function to update particles
void updateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        // Decrease lifetime
        it->lifetime -= deltaTime;
        
        // If lifetime is over, delete
        if (it->lifetime <= 0.0f) {
            it = particles.erase(it);
        } else {
            // Update position
            it->x += it->vx * deltaTime;
            it->y += it->vy * deltaTime;
            
            // Update rotation
            it->rotation += it->rotationSpeed * deltaTime;
            
            // Gravity effect (optional)
            it->vy -= 0.002f; // Slight gravity effect
            
            // Slowing down effect (optional)
            it->vx *= 0.98f;
            it->vy *= 0.98f;
            
            // Reduce alpha value based on lifetime
            it->a = it->lifetime;
            
            ++it;
        }
    }
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

// Function to draw all particles
void drawParticles() {
    for (const auto& p : particles) {
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

// Function for level up light burst effect
void createLevelUpEffect() {
    // Central ring effect
    const int numRings = 3;
    const int particlesPerRing = 30;
    
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
    
    // Add random white sparkles
    for (int i = 0; i < 50; i++) {
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
    
    // Removed the particle system

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
    }
    drawRectangle(powerUp.x, powerUp.y, 0.08f, 0.08f, r, g, b);
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
    // Update position based on movement pattern
    switch (block.movementPattern) {
        case 0: // Linear - just move down
            // y position is updated in the main loop
            break;
            
        case 1: // Zigzag - horizontal sine wave
            // Make sure the block doesn't go off screen
            block.x = block.originX + sin(block.movementTimer * 3.0f) * 0.3f;
            // Clamp to screen boundaries
            if (block.x < -0.95f) block.x = -0.95f;
            if (block.x > 0.95f) block.x = 0.95f;
            block.movementTimer += 0.016f; // Increment timer
            break;
            
        case 2: // Circular - orbit around a center point
            // Make gentler circular movement
            block.x = block.originX + sin(block.movementTimer * 1.5f) * 0.15f;
            // Limit the vertical modification to prevent skipping collision detection
            block.y += cos(block.movementTimer * 1.5f) * 0.003f; 
            block.movementTimer += 0.016f;
            // Clamp to screen boundaries
            if (block.x < -0.95f) block.x = -0.95f;
            if (block.x > 0.95f) block.x = 0.95f;
            break;
    }
}

int main() {
    srand(time(0));
    
    // Enable SFML error messages
    sf::err().rdbuf(std::cerr.rdbuf());

    // Use correct project directory
    std::filesystem::path projectPath = "/Users/melih/GitHub/spd-project";
    std::filesystem::path soundPath = projectPath / "sounds";
    
    // Print debug info
    std::cout << "Project directory: " << projectPath.string() << std::endl;
    std::cout << "Sound files directory: " << soundPath.string() << std::endl;

    // Directory check
    if (!std::filesystem::exists(soundPath)) {
        std::cerr << "Sounds directory not found! Creating..." << std::endl;
        std::filesystem::create_directory(soundPath);
        
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

    // Main game loop
    while (!glfwWindowShouldClose(window)) {
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

            // Create ONLY when gameStarted and !gameOver
            if (gameStarted && !gameOver && rand() % 500 == 0) {
                powerUps.push_back({
                    (rand() % 200 - 100) / 100.0f, 
                    1.0f, 
                    rand() % 3 + 1,
                    5.0f  // 5 seconds duration
                });
            }

            // Update and draw power-ups
            for (auto it = powerUps.begin(); it != powerUps.end();) {
                it->y -= blockSpeed;
                drawPowerUp(*it);

                // Simple version for PowerUp collection code block
                if (it->y < -0.7f && it->y > -0.9f && it->x < playerX + 0.1f && it->x + 0.1f > playerX) {
                    powerUpSound.play();
                    
                    // PowerUp switch statement fix:
                    switch (it->type) {
                        case 1: { // Speed
                            hasSpeedBoost = true;
                            speedBoostTimer = 20.0f;
                            playerSpeed = originalPlayerSpeed + 0.1f;
                            break;
                        }
                        
                        case 2: { // Block reset
                            hasBlockReset = true;
                            blockResetTimer = 20.0f;
                            blocks.clear();
                            
                            // Create at least one valid block with correct initialization
                            float xPos = (rand() % 200 - 100) / 100.0f;
                            float r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                            float g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            float b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            
                            blocks.push_back({
                                xPos,      // x
                                1.0f,      // y
                                rand() % 3, // shape
                                r, g, b,   // color
                                0,         // movement pattern (start with linear)
                                0.0f,      // timer
                                xPos       // originX
                            });
                            break;
                        }
                        
                        case 3: { // Invisibility
                            isInvisible = true;
                            invisibilityTimer = 20.0f;
                            break;
                        }
                    }
                    
                    // Remove the PowerUp
                    it = powerUps.erase(it);
                } else if (it->y < -1.0f) {
                    it = powerUps.erase(it); // Delete if it's gone off screen
                } else {
                    ++it; // Move to next power-up
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
                    // Restore normal block generation
                    while (blocks.size() < level) {
                        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
                    }
                }
            }

            if (isInvisible) {
                invisibilityTimer -= 0.016f;
                if (invisibilityTimer <= 0) {
                    isInvisible = false;
                }
            }

            // In main game loop, use for drawing and updating blocks
            for (auto& block : blocks) {
                // Update block's y position (common for all blocks)
                block.y -= blockSpeed;
                
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
                    
                    score++;
                    
                    // Level system
                    const int MAX_BLOCKS = 10;

                    // Increase probability of more difficult movement patterns as level increases
                    // Updated level-up code using constants
                    if (score % SCORE_PER_LEVEL == 0) {
                        levelUpSound.play();
                        level++;
                        blockSpeed += LEVEL_SPEED_INCREASE;
                        
                        // Create light burst effect for level up
                        createLevelUpEffect();
                        
                        // Other level up code...
                        
                        // Add new block only if under the maximum
                        if (blocks.size() < MAX_BLOCKS) {
                            float r = 0.7f + ((float)rand() / RAND_MAX) * 0.3f;
                            float g = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            float b = 0.0f + ((float)rand() / RAND_MAX) * 0.3f;
                            
                            float newXPos = (rand() % 200 - 100) / 100.0f;
                            
                            // Increase probability of complex patterns as level increases
                            int movementPattern;
                            
                            // Only linear movement (0) until level 3
                            if (level < 3) {
                                movementPattern = 0; // Only linear movement
                            } else {
                                // Complex movements for level 3 and beyond
                                int randomVal = rand() % 100;
                                if (randomVal < 30) { 
                                    movementPattern = 0; // 30% linear
                                } else if (randomVal < 70) { 
                                    movementPattern = 1; // 40% zigzag
                                } else {
                                    movementPattern = 2; // 30% circular
                                }
                            }
                            
                            blocks.push_back({
                                newXPos,               // x
                                1.0f,                  // y
                                rand() % 3,            // shape
                                r, g, b,               // color
                                movementPattern,       // movement pattern (level-based)
                                0.0f,                  // movement timer
                                newXPos                // origin X position
                            });
                        }
                    } else {
                        blockSpeed += NORMAL_SPEED_INCREASE;
                    }
                }

                // Adjust collision detection based on different shapes
                bool collision = false;
                
                // Collision detection based on shape
                switch (block.shape) {
                    case 0: // Square - regular rectangle collision detection
                        collision = (block.y < -0.7f && block.y > -0.9f && 
                                     block.x < playerX + 0.1f && block.x + 0.1f > playerX);
                        break;
                    case 1: // Triangle - triangle collision detection (simpler approach)
                        collision = (block.y - 0.05f < -0.7f && block.y > -0.9f && 
                                     block.x < playerX + 0.1f && block.x + 0.1f > playerX);
                        break;
                    case 2: // Circle - circle collision detection
                        {
                            // Simplify circle collision to be more reliable
                            float circleX = block.x + 0.05f; // Circle center X
                            float circleY = block.y - 0.05f; // Circle center Y
                            float radius = 0.05f;           // Circle radius
                            
                            // Player rectangle bounds
                            float playerLeft = playerX;
                            float playerRight = playerX + 0.1f;
                            float playerTop = -0.8f;
                            float playerBottom = -0.9f;
                            
                            // Find closest point on rectangle to circle
                            float closestX = std::max(playerLeft, std::min(circleX, playerRight));
                            float closestY = std::max(playerBottom, std::min(circleY, playerTop));
                            
                            // Calculate distance from closest point to circle center
                            float distanceX = circleX - closestX;
                            float distanceY = circleY - closestY;
                            float distanceSquared = distanceX * distanceX + distanceY * distanceY;
                            
                            // If distance is less than radius, collision occurred
                            collision = (distanceSquared <= (radius * radius));
                        }
                        break;
                }
                
                // When resetting block after collision
                if (collision) {
                    if (!isInvisible) {
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
                    
                    // Reset block position regardless of invisibility
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

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}