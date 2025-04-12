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
float playerSpeed = 0.05f;
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

struct Block {
    float x, y;
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

    for (int i = 0; i < 3; i++) {
        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
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
                    
                    switch (it->type) {
                        case 1: // Speed
                            hasSpeedBoost = true;
                            speedBoostTimer = 20.0f;
                            playerSpeed = originalPlayerSpeed + 0.1f;
                            break;
                            
                        case 2: // Block reset
                            hasBlockReset = true;
                            blockResetTimer = 20.0f;
                            blocks.clear();
                            blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
                            break;
                            
                        case 3: // Invisibility
                            isInvisible = true;
                            invisibilityTimer = 20.0f;
                            break;
                    }
                    
                    // Remove the PowerUp
                    it = powerUps.erase(it);
                } else if (it->y < -1.0f) {
                    it = powerUps.erase(it);
                } else {
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

            // Update and draw blocks
            for (auto& block : blocks) {
                block.y -= blockSpeed;
                drawRectangle(block.x, block.y, 0.1f, 0.1f, 1.0f, 0.0f, 0.0f);

                if (block.y < -1.0f) {
                    block.x = (rand() % 200 - 100) / 100.0f;
                    block.y = 1.0f;
                    score++;
                    
                    // Level system
                    const int MAX_BLOCKS = 10;

                    // Simple version for level up
                    if (score % 10 == 0) {
                        levelUpSound.play();
                        level++;
                        blockSpeed += 0.0005f;
                        
                        // Add new block only if under the maximum
                        if (blocks.size() < MAX_BLOCKS) {
                            blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
                        }
                    } else {
                        blockSpeed += 0.00005f; 
                    }
                }

                // Simple version for collision
                if (block.y < -0.7f && block.y > -0.9f && block.x < playerX + 0.1f && block.x + 0.1f > playerX) {
                    if (!isInvisible) {
                        health--;
                        collisionSound.play();
                        
                        if (health <= 0) {
                            gameOverSound.play();
                            sigma.stop();
                            gameOver = true;
                            fadeOutEffect = true;
                            fadeAlpha = 0.0f;
                        }
                    }
                    
                    // Reset block position regardless of invisibility
                    block.y = 1.0f;  
                    block.x = (rand() % 200 - 100) / 100.0f;
                }
            }
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