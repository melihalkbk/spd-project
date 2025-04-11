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

// Define sound objects separately
sf::Sound collisionSound;
sf::Sound powerUpSound;
sf::Sound levelUpSound;
sf::Sound gameOverSound;
sf::Music sigma;

float playerX = 0.0f; //Player position(Yatay)
float playerSpeed = 0.05f;
float blockSpeed = 0.01f; // Initial block speed(Seviye arttıkça artar)
int score = 0;
int health = 3;
int level = 1;  
bool gameOver = false;
bool gameStarted = false;
float backgroundColor = 0.0f;
bool colorIncreasing = true; // Background color animation direction(Açık/Koyu Tonlarda Değişmesini Sağlar)

// Add after other global variables
bool isMuted = false; // Mute toggle(Başlangıçta ses kapalı)
float previousVolume = 30.0f;  // Store previous volume for unmuting(Default volume)

// Add after other global variables
bool isPaused = false;

struct Block {
    float x, y;
};

struct PowerUp {
    float x, y;
    int type;  // 1 = speed, 2 = block reset, 3 = invisibility
    float duration;  // Power-up duration(Etkin Kalma Süresi)
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
        // Add pause toggle
        if (key == GLFW_KEY_P && gameStarted && !gameOver) {
            isPaused = !isPaused;
            if (isPaused) {
                sigma.pause();  // Pause background music
                std::cout << "Game Paused" << std::endl;
            } else {
                sigma.play();   // Resume background music
                std::cout << "Game Resumed" << std::endl;
            }
        }

        // Add mute toggle
        if (key == GLFW_KEY_M) {
            isMuted = !isMuted;
            if (isMuted) {
                // Store current volume and mute all sounds
                previousVolume = sigma.getVolume();
                sigma.setVolume(0.0f);
                collisionSound.setVolume(0.0f);
                powerUpSound.setVolume(0.0f);
                levelUpSound.setVolume(0.0f);
                gameOverSound.setVolume(0.0f);
                std::cout << "Sound muted" << std::endl;
            } else {
                // Restore previous volume
                sigma.setVolume(previousVolume);
                collisionSound.setVolume(100.0f);
                powerUpSound.setVolume(100.0f);
                levelUpSound.setVolume(100.0f);
                gameOverSound.setVolume(100.0f);
                std::cout << "Sound unmuted" << std::endl;
            }
        }

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
    } else if (isPaused) {
        title << "PAUSED | Press P to Resume";
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

        updateWindowTitle(window); // **Display score and game status in window title**

        if (!gameStarted) {
            std::cout << "Welcome to the Game! Press ENTER." << std::endl;
        }
        else if (!gameOver) {
            if (!isPaused) {  // Only update game state if not paused
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
                        
                        // Level system
                        // Set a maximum number of blocks
                        const int MAX_BLOCKS = 10;

                        if (score % 10 == 0) {
                            levelUpSound.play();
                            level++;
                            blockSpeed += 0.0005f;
                            
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
                            // On game over
                            if (health <= 0) {
                                gameOverSound.play();  // Game over sound
                                sigma.stop();  // Stop background music
                                gameOver = true;
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
                // Draw "PAUSED" text or pause screen
                updateWindowTitle(window);  // Update window title to show pause state
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