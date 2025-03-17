#define SFML_DEBUG  // Add for debug messages

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <filesystem> // Added
#include <optional>   // Added

// Define sound buffer objects
sf::SoundBuffer collisionBuffer;
sf::SoundBuffer powerUpBuffer;
sf::SoundBuffer levelUpBuffer;
sf::SoundBuffer gameOverBuffer;

// Define sound objects separately
sf::Sound collisionSound;
sf::Sound powerUpSound;
sf::Sound levelUpSound;
sf::Sound gameOverSound;
sf::Music sigma;

float playerX = 0.0f;
float playerSpeed = 0.05f;
float blockSpeed = 0.01f;  // Reduced from 0.02f to 0.01f
int score = 0;
int health = 3;
int level = 1;  // Newly added level variable
bool gameOver = false;
bool gameStarted = false;
float backgroundColor = 0.0f;
bool colorIncreasing = true;

struct Block {
    float x, y;
};

struct PowerUp {
    float x, y;
    int type;  // 1 = speed, 2 = block reset, 3 = invisibility
    float duration;  // Power-up duration
};

std::vector<Block> blocks;
std::vector<PowerUp> powerUps;
bool isInvisible = false;
float invisibilityTimer = 0.0f;

void resetGame() {
    playerX = 0.0f;
    score = 0;
    health = 3;
    level = 1;  // Reset level
    blockSpeed = 0.01f;  // Update initial speed here as well
    gameOver = false;
    blocks.clear();
    powerUps.clear();
    isInvisible = false;
    invisibilityTimer = 0.0f;
    playerSpeed = 0.05f;
    backgroundColor = 0.0f;
    colorIncreasing = true;

    for (int i = 0; i < 5; i++) {
        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
    }

    std::cout << "Game Reset! New game started!" << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (!gameStarted && key == GLFW_KEY_ENTER) {
            gameStarted = true;
            resetGame();
        }
        else if (gameOver && key == GLFW_KEY_ENTER) {
            resetGame();
        }
        
        if (gameStarted && !gameOver) {
            if (key == GLFW_KEY_LEFT && playerX > -0.9f) playerX -= playerSpeed;
            if (key == GLFW_KEY_RIGHT && playerX < 0.9f) playerX += playerSpeed;
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
    } else {
        title << "Avoidance Game | Level: " << level << " | Score: " << score << " | Health: " << health;
    }
    glfwSetWindowTitle(window, title.str().c_str());
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
        sigma.setLoop(true);
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

    // Add music control in the main loop (at the beginning of the while loop):
    while (!glfwWindowShouldClose(window)) {
        // Music control
        if (sigma.getStatus() != sf::SoundSource::Playing) {
            sigma.play();
        }
        // Background color animation
        if (colorIncreasing) {
            backgroundColor += 0.001f; // Slower transition, changed from 0.005f to 0.001f
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

        updateWindowTitle(window); // **Display score and game status in window title**

        if (!gameStarted) {
            std::cout << "Welcome to the Game! Press ENTER." << std::endl;
        }
        else if (!gameOver) {
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

                // When power-up is collected
                if (it->y < -0.7f && it->y > -0.9f && it->x < playerX + 0.1f && it->x + 0.1f > playerX) {
                    powerUpSound.play();  // Power-up sound
                    switch (it->type) {
                        case 1: // Speed
                            playerSpeed += 0.02f;
                            std::cout << "Speed Increased!" << std::endl;
                            break;
                        case 2: // Block reset
                            blocks.clear();
                            blocks.push_back({(rand() % 200 - 100) / 100.0f});
                            std::cout << "Blocks Cleared!" << std::endl;
                            break;
                        case 3: // Invisibility
                            isInvisible = true;
                            invisibilityTimer = 5.0f;
                            std::cout << "Invisibility Activated!" << std::endl;
                            break;
                    }
                    it = powerUps.erase(it);
                } else if (it->y < -1.0f) {
                    it = powerUps.erase(it);
                } else {
                    ++it;
                }
            }

            // Update invisibility timer
            if (isInvisible) {
                invisibilityTimer -= 0.016f; // approximately 60 FPS
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
                    
                    // Level system
                    // When level up
                    if (score % 10 == 0) {
                        levelUpSound.play();  // Level up sound
                        level++;
                        blockSpeed += 0.0005f;  // Reduced from 0.001f to 0.0005f
                        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});  // Add new block
                        std::cout << "New Level: " << level << " | Block Count: " << blocks.size() 
                                 << " | Block Speed: " << blockSpeed << std::endl;
                    } else {
                        blockSpeed += 0.00005f;  // Reduced from 0.0001f to 0.00005f
                    }
                    
                    std::cout << "Score: " << score << std::endl;
                }

                // On collision
                if (block.y < -0.7f && block.y > -0.9f && block.x < playerX + 0.1f && block.x + 0.1f > playerX) {
                    health--;
                    collisionSound.play();  // Collision sound
                    // On game over
                    if (health <= 0) {
                        gameOverSound.play();  // Game over sound
                        sigma.stop();  // Stop background music
                        gameOver = true;
                    } else {
                        std::cout << "Remaining Health: " << health << std::endl;
                        block.y = 1.0f;  // Reset block
                        block.x = (rand() % 200 - 100) / 100.0f;  // New random x position
                    }
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Before exiting, cleanup SFML and GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}