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
bool colorIncreasing = true; // Background color animation direction(Changes between light/dark tones)

// Add after other global variables
bool isMuted = false; // Mute toggle(Sound off at start)
float previousVolume = 30.0f;  // Store previous volume for unmuting(Default volume)

// Add after other global variables
bool isPaused = false;

// Global variables
bool fadeInEffect = false;
bool fadeOutEffect = false;
float fadeAlpha = 1.0f;

struct Block {
    float x, y;
};

struct PowerUp {
    float x, y;
    int type;  // 1 = speed, 2 = block reset, 3 = invisibility
    float duration;  // Power-up duration(Time active)
};

// Structures for particle system
struct Particle {
    float x, y;          // Position
    float vx, vy;        // Velocity vector
    float r, g, b, a;    // Color and transparency (r,g,b,a)
    float lifetime;      // Lifetime
    float size;          // Size
};

std::vector<Block> blocks;
std::vector<PowerUp> powerUps;
bool isInvisible = false; // Invisibility state
float invisibilityTimer = 0.0f; 

// Add these global variables after other power-up related variables
bool hasSpeedBoost = false;
float speedBoostTimer = 0.0f;
float originalPlayerSpeed = 0.05f;  // Store original speed
bool hasBlockReset = false;
float blockResetTimer = 0.0f;

// Add to global variables section
std::vector<Particle> particles;
const int MAX_PARTICLES = 100;  // Maximum number of particles

// Add these new global variables at the top with other game state variables
enum class GameState {
    MAIN_MENU,
    GAMEPLAY,
    PAUSED,
    GAME_OVER
};

GameState currentState = GameState::MAIN_MENU;
int menuSelection = 0; // 0 = Play, 1 = Controls, 2 = Exit

void resetGame() {
    playerX = 0.0f;
    score = 0; // Reset score
    health = 3; // Reset health
    level = 1;  // Reset level
    blockSpeed = 0.01f;  // Update initial speed here as well
    gameOver = false; // Reset game over state
    blocks.clear(); // Clear blocks
    powerUps.clear(); // Clear power-ups
    isInvisible = false; // Reset invisibility state
    invisibilityTimer = 0.0f;   // Reset invisibility timer
    playerSpeed = 0.05f; // Reset player speed
    backgroundColor = 0.0f; // Reset background color 
    colorIncreasing = true; // Reset color direction
    hasSpeedBoost = false;
    speedBoostTimer = 0.0f;
    playerSpeed = originalPlayerSpeed;
    hasBlockReset = false;
    blockResetTimer = 0.0f;

    for (int i = 0; i < 3; i++) {
        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f}); // Add 3 blocks
    }

    std::cout << "Game Reset! New game started!" << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height); // Set viewport size
}

// Update key_callback function
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (currentState) {
            case GameState::MAIN_MENU:
                // Menu navigation
                if (key == GLFW_KEY_UP && menuSelection > 0) {
                    menuSelection--;
                }
                else if (key == GLFW_KEY_DOWN && menuSelection < 2) {
                    menuSelection++;
                }
                else if (key == GLFW_KEY_ENTER) {
                    switch (menuSelection) {
                        case 0: // Play
                            currentState = GameState::GAMEPLAY;
                            gameStarted = true;
                            resetGame();
                            fadeInEffect = true;
                            fadeAlpha = 1.0f;
                            sigma.play();
                            break;
                        case 1: // Controls
                            // Show controls screen
                            menuSelection = -1; // Special state for controls
                            break;
                        case 2: // Exit
                            glfwSetWindowShouldClose(window, GLFW_TRUE);
                            break;
                    }
                }
                else if (key == GLFW_KEY_ESCAPE && menuSelection == -1) {
                    // Return from controls screen
                    menuSelection = 1;
                }
                break;
                
            case GameState::GAMEPLAY:
                // Game controls
                if (key == GLFW_KEY_P && !gameOver) {
                    isPaused = !isPaused;
                    if (isPaused) {
                        currentState = GameState::PAUSED;
                        sigma.pause();
                        std::cout << "Game Paused" << std::endl;
                    } else {
                        currentState = GameState::GAMEPLAY;
                        sigma.play();
                        std::cout << "Game Resumed" << std::endl;
                    }
                }
                else if (key == GLFW_KEY_M) {
                    // Mute toggle (existing code)
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
                else if (key == GLFW_KEY_ESCAPE) {
                    // Return to main menu
                    currentState = GameState::MAIN_MENU;
                    menuSelection = 0;
                    sigma.stop();
                }
                
                // Player movement
                if (!gameOver && !isPaused) {
                    if (key == GLFW_KEY_LEFT && playerX > -0.9f) playerX -= playerSpeed;
                    if (key == GLFW_KEY_RIGHT && playerX < 0.9f) playerX += playerSpeed;
                }
                break;
                
            case GameState::PAUSED:
                if (key == GLFW_KEY_P) {
                    isPaused = false;
                    currentState = GameState::GAMEPLAY;
                    sigma.play();
                    std::cout << "Game Resumed" << std::endl;
                }
                else if (key == GLFW_KEY_ESCAPE) {
                    // Return to main menu
                    currentState = GameState::MAIN_MENU;
                    menuSelection = 0;
                    sigma.stop();
                    isPaused = false;
                }
                break;
                
            case GameState::GAME_OVER:
                if (key == GLFW_KEY_ENTER) {
                    currentState = GameState::GAMEPLAY;
                    resetGame();
                    fadeInEffect = true;
                    fadeAlpha = 1.0f;
                }
                else if (key == GLFW_KEY_ESCAPE) {
                    currentState = GameState::MAIN_MENU;
                    menuSelection = 0;
                }
                break;
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

// Create new particle
void spawnParticles(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count && particles.size() < MAX_PARTICLES; i++) {
        // Random angle and speed
        float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        float speed = 0.005f + ((float)rand() / RAND_MAX) * 0.01f;
        
        Particle p;
        p.x = x;
        p.y = y;
        p.vx = cosf(angle) * speed;
        p.vy = sinf(angle) * speed;
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 1.0f;  // Start fully opaque
        p.lifetime = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;  // Lifetime between 0.5-1.0 seconds
        p.size = 0.02f + ((float)rand() / RAND_MAX) * 0.02f;    // Size between 0.02-0.04
        
        particles.push_back(p);
    }
}

// Draw particles
void drawParticles() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (const auto& p : particles) {
        // Transparency with alpha value
        glColor4f(p.r, p.g, p.b, p.a);
        
        // Draw particle (small square)
        glBegin(GL_QUADS);
            glVertex2f(p.x - p.size/2, p.y + p.size/2);
            glVertex2f(p.x + p.size/2, p.y + p.size/2);
            glVertex2f(p.x + p.size/2, p.y - p.size/2);
            glVertex2f(p.x - p.size/2, p.y - p.size/2);
        glEnd();
    }
    
    glDisable(GL_BLEND);
}

// Update particles
void updateParticles(float deltaTime) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->x += it->vx * deltaTime * 60.0f;  // Velocity * deltaTime
        it->y += it->vy * deltaTime * 60.0f;
        it->lifetime -= deltaTime;
        it->a = it->lifetime;  // Becomes more transparent as lifetime decreases
        
        if (it->lifetime <= 0) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

// Draw light effect
void drawLightEffect(float x, float y, float radius, float r, float g, float b, float intensity) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending mode
    
    // Size for light gradient
    const int segments = 20;
    const float fullAngle = 2.0f * 3.14159f;
    
    // Draw light ring
    glBegin(GL_TRIANGLE_FAN);
        // Center point - full color
        glColor4f(r, g, b, intensity);
        glVertex2f(x, y);
        
        // Outer edge - transparent
        glColor4f(r, g, b, 0.0f);
        for (int i = 0; i <= segments; i++) {
            float angle = i * fullAngle / segments;
            float px = x + cosf(angle) * radius;
            float py = y + sinf(angle) * radius;
            glVertex2f(px, py);
        }
    glEnd();
    
    glDisable(GL_BLEND);
}

// Create a function to draw text (basic approach with rectangles)
void drawText(const std::string& text, float x, float y, float size, float r, float g, float b) {
    // Simple visual text representation with rectangles
    float spacing = size * 0.6f;
    float startX = x;
    
    for (char c : text) {
        // Skip spaces
        if (c == ' ') {
            x += spacing;
            continue;
        }
        
        // Simplified letter shapes using rectangles
        switch (c) {
            case 'A':
                drawRectangle(x, y, size/8, size, r, g, b);  // Left line
                drawRectangle(x + size/8*7, y, size/8, size, r, g, b);  // Right line
                drawRectangle(x, y, size, size/8, r, g, b);  // Top line
                drawRectangle(x, y - size/2, size, size/8, r, g, b);  // Middle line
                break;
            // Add more letters as needed
            // This is just a placeholder for actual text rendering
            default:
                drawRectangle(x, y, size/2, size, r, g, b);
                break;
        }
        x += spacing;
    }
}

// Draw a button with text
void drawButton(const std::string& text, float x, float y, float width, float height, 
                bool selected, float r, float g, float b) {
    if (selected) {
        // Draw highlighted button
        drawRectangle(x - 0.01f, y + 0.01f, width + 0.02f, height + 0.02f, 1.0f, 1.0f, 1.0f);
    }
    
    // Draw button background
    drawRectangle(x, y, width, height, r, g, b);
    
    // Display button label with individual letters
    float fontSize = height * 0.6f;
    float textX = x + width/2 - text.length() * fontSize * 0.3f;
    float textY = y - height/4;
    
    for (size_t i = 0; i < text.length(); i++) {
        // Draw each letter as a small colored rectangle
        float letterX = textX + i * fontSize * 0.6f;
        drawRectangle(letterX, textY, fontSize * 0.4f, fontSize * 0.8f, 1.0f, 1.0f, 1.0f);
    }
}

// Function to draw the main menu
void drawMainMenu() {
    // Draw title
    float titleSize = 0.2f;
    
    // Draw "AVOIDANCE GAME" title
    drawRectangle(-0.6f, 0.7f, 1.2f, titleSize, 0.2f, 0.5f, 0.8f);
    
    // Draw color accent lines under title
    for (int i = 0; i < 10; i++) {
        float hue = (float)i / 10.0f;
        // Create rainbow effect
        float r = sin(hue * 6.28f) * 0.5f + 0.5f;
        float g = sin((hue + 0.33f) * 6.28f) * 0.5f + 0.5f;
        float b = sin((hue + 0.67f) * 6.28f) * 0.5f + 0.5f;
        
        drawRectangle(-0.5f + i * 0.1f, 0.5f, 0.08f, 0.03f, r, g, b);
    }
    
    // Draw menu buttons
    float buttonWidth = 0.6f;
    float buttonHeight = 0.1f;
    float buttonX = -buttonWidth/2;
    float buttonY = 0.2f;
    float spacing = 0.15f;
    
    // Play button
    drawButton("PLAY", buttonX, buttonY, buttonWidth, buttonHeight, 
              menuSelection == 0, 0.1f, 0.5f, 0.1f);
    
    // Controls button
    drawButton("CONTROLS", buttonX, buttonY - spacing, buttonWidth, buttonHeight, 
              menuSelection == 1, 0.1f, 0.1f, 0.5f);
    
    // Exit button
    drawButton("EXIT", buttonX, buttonY - spacing*2, buttonWidth, buttonHeight, 
              menuSelection == 2, 0.5f, 0.1f, 0.1f);
    
    // Draw instructions at bottom
    drawRectangle(-0.7f, -0.7f, 1.4f, 0.08f, 0.8f, 0.8f, 0.8f);
    
    // Draw small arrow indicators near active button
    if (menuSelection >= 0 && menuSelection <= 2) {
        float arrowY = buttonY - menuSelection * spacing;
        drawRectangle(buttonX - 0.1f, arrowY - 0.02f, 0.06f, 0.06f, 1.0f, 1.0f, 0.0f);
    }
}

// Function to draw control instructions
void drawControlsScreen() {
    // Draw title
    drawRectangle(-0.5f, 0.7f, 1.0f, 0.15f, 0.1f, 0.1f, 0.6f);
    
    // Draw controls
    float y = 0.4f;
    float lineHeight = 0.08f;
    float spacing = 0.15f;
    
    // Arrow keys control
    drawRectangle(-0.5f, y, 0.1f, 0.1f, 1.0f, 1.0f, 1.0f);  // Left arrow
    drawRectangle(-0.3f, y, 0.1f, 0.1f, 1.0f, 1.0f, 1.0f);  // Right arrow
    drawRectangle(0.0f, y - 0.04f, 0.6f, lineHeight, 0.7f, 0.7f, 0.7f); // "Move player"
    
    // P key
    drawRectangle(-0.5f, y - spacing*1, 0.1f, 0.1f, 0.2f, 0.8f, 0.2f);  // P key
    drawRectangle(0.0f, y - spacing*1 - 0.04f, 0.6f, lineHeight, 0.7f, 0.7f, 0.7f); // "Pause"
    
    // M key
    drawRectangle(-0.5f, y - spacing*2, 0.1f, 0.1f, 0.8f, 0.2f, 0.2f);  // M key
    drawRectangle(0.0f, y - spacing*2 - 0.04f, 0.6f, lineHeight, 0.7f, 0.7f, 0.7f); // "Mute"
    
    // ESC key
    drawRectangle(-0.5f, y - spacing*3, 0.1f, 0.1f, 0.8f, 0.8f, 0.2f);  // ESC key
    drawRectangle(0.0f, y - spacing*3 - 0.04f, 0.6f, lineHeight, 0.7f, 0.7f, 0.7f); // "Exit"
    
    // Back instruction
    drawRectangle(-0.5f, -0.7f, 1.0f, lineHeight, 0.8f, 0.8f, 0.0f); // "Press ESC to return"
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
        std::cerr << "- collision.wav\n- pickup.wav\n- levelup.wav\n- gameover.wav\n- background.wav" << std::endl;
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
        return -1;  // Exit on error
    } else {
        powerUpSound.setBuffer(powerUpBuffer);
    }

    if (!levelUpBuffer.loadFromFile((soundPath / "levelup.wav").string())) {
        std::cerr << "Level-up sound failed to load: " << (soundPath / "levelup.wav").string() << std::endl;
        return -1;  // Exit on error
    } else {
        levelUpSound.setBuffer(levelUpBuffer);
    }

    if (!gameOverBuffer.loadFromFile((soundPath / "gameover.wav").string())) {
        std::cerr << "Game-over sound failed to load: " << (soundPath / "gameover.wav").string() << std::endl;
        return -1;  // Exit on error
    } else {
        gameOverSound.setBuffer(gameOverBuffer);
    }

    // Load background music
    if (!sigma.openFromFile((soundPath / "sigma.wav").string())) {
        std::cerr << "Background music failed to load: " << (soundPath / "sigma.wav").string() << std::endl;
        return -1;  // Exit on error
    } else {
        sigma.setVolume(30.0f);
        // We'll handle the loop control ourselves
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

    resetGame();

    // In main game loop
    while (!glfwWindowShouldClose(window)) {
        // Music control - restart music if it ends
        if (currentState == GameState::GAMEPLAY && !isPaused && !gameOver &&
            sigma.getStatus() != sf::Music::Status::Playing) {
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
            backgroundColor * 0.2f,        // Dark blue shade
            backgroundColor * 0.1f,        // Slight green
            0.3f + backgroundColor * 0.2f, // Base blue color
            1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);

        updateWindowTitle(window);

        // Render based on current state
        switch (currentState) {
            case GameState::MAIN_MENU:
                if (menuSelection >= 0) {
                    drawMainMenu();
                } else {
                    drawControlsScreen();
                }
                break;
                
            case GameState::GAMEPLAY:
                if (!gameOver) {
                    // Existing gameplay code
                    // Add light effect before drawing the player
                    drawLightEffect(playerX + 0.05f, -0.8f, 0.2f, 0.0f, 0.8f, 0.0f, 0.3f);  // Green glow

                    // Draw player (semi-transparent if invisible)
                    if (!isInvisible) {
                        drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f);
                    } else {
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f);
                        glDisable(GL_BLEND);
                    }

                    // Create power-up
                    if (rand() % 500 == 0) {
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

                        // Add light effect around power-ups
                        for (const auto& powerUp : powerUps) {
                            float r = 0.0f, g = 0.0f, b = 0.0f;
                            switch (powerUp.type) {
                                case 1: g = 0.8f; break;
                                case 2: b = 0.8f; break;
                                case 3: r = g = 0.6f; break;
                            }
                            drawLightEffect(powerUp.x + 0.04f, powerUp.y, 0.15f, r, g, b, 0.4f);
                        }

                        // When power-up is collected
                        if (it->y < -0.7f && it->y > -0.9f && it->x < playerX + 0.1f && it->x + 0.1f > playerX) {
                            powerUpSound.play();  // Power-up sound
                            
                            // Color based on power-up type
                            float r = 0.0f, g = 0.0f, b = 0.0f;
                            switch (it->type) {
                                case 1: g = 1.0f; break;     // Speed: green
                                case 2: b = 1.0f; break;     // Reset: blue
                                case 3: r = g = 1.0f; break; // Invisibility: yellow
                            }
                            
                            // Power-up particles
                            spawnParticles(it->x + 0.04f, it->y, r, g, b, 15);
                            
                            switch (it->type) {
                                case 1: // Speed
                                    hasSpeedBoost = true;
                                    speedBoostTimer = 20.0f;
                                    playerSpeed = originalPlayerSpeed + 0.1f;  // Increased speed boost
                                    std::cout << "Speed Boost Activated for 20 seconds!" << std::endl;
                                    break;
                                case 2: // Block reset
                                    hasBlockReset = true;
                                    blockResetTimer = 20.0f;
                                    blocks.clear();
                                    blocks.push_back({(rand() % 200 - 100) / 100.0f});
                                    std::cout << "Blocks Reset Active for 20 seconds!" << std::endl;
                                    break;
                                case 3: // Invisibility (existing code)
                                    isInvisible = true;
                                    invisibilityTimer = 20.0f;
                                    std::cout << "Invisibility Activated for 20 seconds!" << std::endl;
                                    break;
                            }
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
                            std::cout << "Speed Boost Ended!" << std::endl;
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
                            std::cout << "Block Reset Ended!" << std::endl;
                        }
                    }

                    // Existing invisibility timer update
                    if (isInvisible) {
                        invisibilityTimer -= 0.016f;
                        if (invisibilityTimer <= 0) {
                            isInvisible = false;
                            std::cout << "Invisibility Ended!" << std::endl;
                        }
                    }

                    for (auto& block : blocks) {
                        block.y -= blockSpeed;
                        drawRectangle(block.x, block.y, 0.1f, 0.1f, 1.0f, 0.0f, 0.0f);

                        if (block.y < -1.0f) {
                            block.x = (rand() % 200 - 100) / 100.0f;
                            block.y = 1.0f;
                            score++;
                            
                            // Small brightness effect every 5 points
                            if (score % 5 == 0) {
                                spawnParticles(0.0f, 0.9f, 0.0f, 1.0f, 0.5f, 5);  // Green particles at top of screen
                            }

                            // Level system
                            // Set a maximum number of blocks
                            const int MAX_BLOCKS = 10;

                            // Special effect for each level up
                            if (score % 10 == 0) {
                                levelUpSound.play();
                                level++;
                                blockSpeed += 0.0005f;
                                
                                // Level up glow - momentary flash on screen
                                float intensity = 0.4f;
                                glEnable(GL_BLEND);
                                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                                
                                glColor4f(1.0f, 1.0f, 0.8f, intensity);
                                glBegin(GL_QUADS);
                                    glVertex2f(-1.0f, 1.0f);
                                    glVertex2f(1.0f, 1.0f);
                                    glVertex2f(1.0f, -1.0f);
                                    glVertex2f(-1.0f, -1.0f);
                                glEnd();
                                
                                glDisable(GL_BLEND);
                                
                                // Level up particles - distributed across screen
                                for (int i = 0; i < 5; i++) {
                                    float randomX = -0.9f + ((float)rand() / RAND_MAX) * 1.8f;
                                    float randomY = -0.9f + ((float)rand() / RAND_MAX) * 1.8f;
                                    spawnParticles(randomX, randomY, 1.0f, 1.0f, 0.8f, 10);
                                }
                                
                                // Add new block only if under the maximum
                                if (blocks.size() < MAX_BLOCKS) {
                                    blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
                                }
                                
                                std::cout << "New Level: " << level << " | Block Count: " << blocks.size() 
                                         << " | Block Speed: " << blockSpeed << std::endl;
                            } else {
                                blockSpeed += 0.00005f; 
                            }
                            
                            std::cout << "Score: " << score << std::endl;
                        }

                        // On collision
                        if (block.y < -0.7f && block.y > -0.9f && block.x < playerX + 0.1f && block.x + 0.1f > playerX) {
                            if (!isInvisible) {  // Only take damage if not invisible
                                health--;
                                collisionSound.play();  // Collision sound
                                // Collision particles (red)
                                spawnParticles(playerX + 0.05f, -0.8f, 1.0f, 0.3f, 0.2f, 20);
                                // On game over
                                if (health <= 0) {
                                    gameOverSound.play();  // Game over sound
                                    sigma.stop();  // Stop background music
                                    gameOver = true;
                                    fadeOutEffect = true;  // Close screen
                                    fadeAlpha = 0.0f;      // Start transparent
                                } else {
                                    std::cout << "Remaining Health: " << health << std::endl;
                                }
                            }
                            // Reset block position regardless of invisibility
                            block.y = 1.0f;  
                            block.x = (rand() % 200 - 100) / 100.0f;
                        }
                    }
                } else {
                    currentState = GameState::GAME_OVER;
                }
                break;
                
            case GameState::PAUSED:
                // Draw paused overlay
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
                
                // Draw "PAUSED" text
                drawRectangle(-0.3f, 0.2f, 0.6f, 0.2f, 1.0f, 1.0f, 1.0f);
                break;
                
            case GameState::GAME_OVER:
                // Draw game over overlay
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
                
                // Draw "GAME OVER" text
                drawRectangle(-0.5f, 0.2f, 1.0f, 0.2f, 1.0f, 0.2f, 0.2f);
                
                // Draw score display
                std::string scoreStr = "SCORE: " + std::to_string(score);
                for (size_t i = 0; i < scoreStr.length(); i++) {
                    drawRectangle(-0.4f + i * 0.08f, -0.1f, 0.06f, 0.1f, 1.0f, 1.0f, 1.0f);
                }
                
                // Draw restart instruction
                drawRectangle(-0.5f, -0.3f, 1.0f, 0.08f, 0.8f, 0.8f, 0.8f);
                
                // Draw press ENTER instruction
                drawRectangle(-0.4f, -0.5f, 0.8f, 0.06f, 0.6f, 0.6f, 0.6f);
                break;
        }

        // Update and draw particles always
        updateParticles(0.016f);
        drawParticles();

        // Update and draw fade effect
        if (fadeInEffect) {
            fadeAlpha -= 0.01f;  // Become more transparent each frame
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
            fadeAlpha += 0.01f;  // Become more opaque each frame
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

    // Before exiting, cleanup SFML and GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}