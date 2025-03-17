#define SFML_DEBUG  // Debug mesajları için ekle

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <filesystem> // Eklendi
#include <optional>   // Eklendi

// Ses buffer nesnelerini tanımla
sf::SoundBuffer collisionBuffer;
sf::SoundBuffer powerUpBuffer;
sf::SoundBuffer levelUpBuffer;
sf::SoundBuffer gameOverBuffer;

// Ses nesnelerini ayrı tanımla
sf::Sound collisionSound;
sf::Sound powerUpSound;
sf::Sound levelUpSound;
sf::Sound gameOverSound;
sf::Music sigma;

float playerX = 0.0f;  
float playerSpeed = 0.05f;  
float blockSpeed = 0.01f;  // 0.02f'den 0.01f'e düşürüldü
int score = 0;  
int health = 3;  
int level = 1;  // Yeni eklenen seviye değişkeni
bool gameOver = false;  
bool gameStarted = false;  
float backgroundColor = 0.0f;
bool colorIncreasing = true;

struct Block {
    float x, y;
};

struct PowerUp {
    float x, y;
    int type;  // 1 = hız, 2 = blok sıfırlama, 3 = görünmezlik
    float duration;  // Güçlendirme süresi
};

std::vector<Block> blocks;
std::vector<PowerUp> powerUps;
bool isInvisible = false;
float invisibilityTimer = 0.0f;

void resetGame() {
    playerX = 0.0f;
    score = 0;
    health = 3;
    level = 1;  // Seviyeyi resetle
    blockSpeed = 0.01f;  // Burada da başlangıç hızını güncelle
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

    std::cout << "Oyun Sıfırlandı! Yeni oyun başladı!" << std::endl;
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
        case 1: // Hız - Yeşil
            g = 1.0f;
            break;
        case 2: // Blok sıfırlama - Mavi
            b = 1.0f;
            break;
        case 3: // Görünmezlik - Sarı
            r = 1.0f;
            g = 1.0f;
            break;
    }
    drawRectangle(powerUp.x, powerUp.y, 0.08f, 0.08f, r, g, b);
}

void updateWindowTitle(GLFWwindow* window) {
    std::ostringstream title;
    if (!gameStarted) {
        title << "Oyuna Hos Geldiniz! ENTER'a Basiniz.";
    } else if (gameOver) {
        title << "Oyun Bitti! Skor: " << score << " | ENTER ile Yeniden Başlat";
    } else {
        title << "Kaçınma Oyunu | Seviye: " << level << " | Skor: " << score << " | Can: " << health;
    }
    glfwSetWindowTitle(window, title.str().c_str());
}

int main() {
    srand(time(0));
    
    // SFML hata mesajlarını aktifleştir
    sf::err().rdbuf(std::cerr.rdbuf());

    // Doğru proje dizinini kullan
    std::filesystem::path projectPath = "/Users/melih/GitHub/spd-project";
    std::filesystem::path soundPath = projectPath / "sounds";
    
    // Debug bilgisi yazdır
    std::cout << "Project directory: " << projectPath.string() << std::endl;
    std::cout << "Sound files directory: " << soundPath.string() << std::endl;

    // Dizin kontrolü
    if (!std::filesystem::exists(soundPath)) {
        std::cerr << "Sounds directory not found! Creating..." << std::endl;
        std::filesystem::create_directory(soundPath);
        
        std::cerr << "Please place the following WAV files in " << soundPath.string() << ":" << std::endl;
        std::cerr << "- collision.wav\n- pickup.wav\n- levelup.wav\n- gameover.wav\n- background.wav" << std::endl;
        return -1;
    }

    // Ses dosyalarını yükle
    if (!collisionBuffer.loadFromFile((soundPath / "collision.wav").string())) {
        std::cerr << "Çarpışma sesi yüklenemedi: " << (soundPath / "collision.wav").string() << std::endl;
        std::cerr << "Dosya boyutu: " << std::filesystem::file_size(soundPath / "collision.wav") << " bytes" << std::endl;
    } else {
        std::cout << "Collision sound loaded successfully!" << std::endl;
        std::cout << "Sample rate: " << collisionBuffer.getSampleRate() << std::endl;
        std::cout << "Channel count: " << collisionBuffer.getChannelCount() << std::endl;
        std::cout << "Duration: " << collisionBuffer.getDuration().asSeconds() << " seconds" << std::endl;
        collisionSound.setBuffer(collisionBuffer);
    }

    if (!powerUpBuffer.loadFromFile((soundPath / "pickup.wav").string())) {
        std::cerr << "Power-up sesi yüklenemedi: " << (soundPath / "pickup.wav").string() << std::endl;
        return -1;  // Hata durumunda çık
    } else {
        powerUpSound.setBuffer(powerUpBuffer);
    }

    if (!levelUpBuffer.loadFromFile((soundPath / "levelup.wav").string())) {
        std::cerr << "Level-up sesi yüklenemedi: " << (soundPath / "levelup.wav").string() << std::endl;
        return -1;  // Hata durumunda çık
    } else {
        levelUpSound.setBuffer(levelUpBuffer);
    }

    if (!gameOverBuffer.loadFromFile((soundPath / "gameover.wav").string())) {
        std::cerr << "Game-over sesi yüklenemedi: " << (soundPath / "gameover.wav").string() << std::endl;
        return -1;  // Hata durumunda çık
    } else {
        gameOverSound.setBuffer(gameOverBuffer);
    }

    // Arkaplan müziğini yükle
    if (!sigma.openFromFile((soundPath / "sigma.wav").string())) {
        std::cerr << "Arkaplan müziği yüklenemedi: " << (soundPath / "sigma.wav").string() << std::endl;
        return -1;  // Hata durumunda çık
    } else {
        sigma.setVolume(30.0f);
        sigma.setLoop(true);
        sigma.play();
    }

    if (!glfwInit()) {
        std::cerr << "GLFW başlatılamadı!" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Kaçınma Oyunu", NULL, NULL);
    if (!window) {
        std::cerr << "Pencere oluşturulamadı!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW başlatılamadı!" << std::endl;
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    resetGame();

    // Ana döngüde müzik kontrolü ekle (while döngüsünün başına):
    while (!glfwWindowShouldClose(window)) {
        // Müzik kontrolü
        if (sigma.getStatus() != sf::SoundSource::Playing) {
            sigma.play();
        }
        // Arka plan rengi animasyonu
        if (colorIncreasing) {
            backgroundColor += 0.001f; // Daha yavaş geçiş için 0.005f yerine 0.001f
        } else {
            backgroundColor -= 0.001f;
        }
        
        if (backgroundColor >= 1.0f) {
            colorIncreasing = false;
        }
        if (backgroundColor <= 0.0f) {
            colorIncreasing = true;
        }

        // Dinamik arka plan rengi
        glClearColor(
            backgroundColor * 0.2f,        // Koyu mavi tonu için
            backgroundColor * 0.1f,        // Çok az yeşil
            0.3f + backgroundColor * 0.2f, // Mavi baz renk
            1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);

        updateWindowTitle(window); // **Pencere başlığına skor ve oyun durumu yazdır**

        if (!gameStarted) {
            std::cout << "Oyuna Hos Geldiniz! ENTER'a basiniz." << std::endl;
        }
        else if (!gameOver) {
            // Oyuncuyu çiz (görünmezse yarı saydam)
            if (!isInvisible) {
                drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f);
            } else {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f);
                glDisable(GL_BLEND);
            }

            // Power-up oluştur
            if (rand() % 500 == 0) {
                powerUps.push_back({
                    (rand() % 200 - 100) / 100.0f, 
                    1.0f, 
                    rand() % 3 + 1,
                    5.0f  // 5 saniyelik süre
                });
            }

            // Power-up'ları güncelle ve çiz
            for (auto it = powerUps.begin(); it != powerUps.end();) {
                it->y -= blockSpeed;
                drawPowerUp(*it);

                // Power-up alındığında
                if (it->y < -0.7f && it->y > -0.9f && it->x < playerX + 0.1f && it->x + 0.1f > playerX) {
                    powerUpSound.play();  // Power-up sesi
                    switch (it->type) {
                        case 1: // Hız
                            playerSpeed += 0.02f;
                            std::cout << "Hız Artırıldı!" << std::endl;
                            break;
                        case 2: // Blok sıfırlama
                            blocks.clear();
                            blocks.push_back({(rand() % 200 - 100) / 100.0f});
                            std::cout << "Bloklar Temizlendi!" << std::endl;
                            break;
                        case 3: // Görünmezlik
                            isInvisible = true;
                            invisibilityTimer = 5.0f;
                            std::cout << "Görünmezlik Aktif!" << std::endl;
                            break;
                    }
                    it = powerUps.erase(it);
                } else if (it->y < -1.0f) {
                    it = powerUps.erase(it);
                } else {
                    ++it;
                }
            }

            // Görünmezlik süresini güncelle
            if (isInvisible) {
                invisibilityTimer -= 0.016f; // yaklaşık 60 FPS
                if (invisibilityTimer <= 0) {
                    isInvisible = false;
                    std::cout << "Görünmezlik Sona Erdi!" << std::endl;
                }
            }

            for (auto& block : blocks) {
                block.y -= blockSpeed;
                drawRectangle(block.x, block.y, 0.1f, 0.1f, 1.0f, 0.0f, 0.0f);

                if (block.y < -1.0f) {
                    block.x = (rand() % 200 - 100) / 100.0f;
                    block.y = 1.0f;
                    score++;
                    
                    // Seviye sistemi
                    // Seviye atladığında
                    if (score % 10 == 0) {
                        levelUpSound.play();  // Seviye atlama sesi
                        level++;
                        blockSpeed += 0.0005f;  // 0.001f'den 0.0005f'e düşürüldü
                        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});  // Yeni blok ekle
                        std::cout << "Yeni Seviye: " << level << " | Blok Sayisi: " << blocks.size() 
                                 << " | Blok Hizi: " << blockSpeed << std::endl;
                    } else {
                        blockSpeed += 0.00005f;  // 0.0001f'den 0.00005f'e düşürüldü
                    }
                    
                    std::cout << "Skor: " << score << std::endl;
                }

                // Çarpışma durumunda
                if (block.y < -0.7f && block.y > -0.9f && block.x < playerX + 0.1f && block.x + 0.1f > playerX) {
                    health--;
                    collisionSound.play();  // Çarpışma sesi
                    // Oyun bitti durumunda
                    if (health <= 0) {
                        gameOverSound.play();  // Game over sesi
                        sigma.stop();  // Arkaplan müziğini durdur
                        gameOver = true;
                    } else {
                        std::cout << "Kalan Can: " << health << std::endl;
                        block.y = 1.0f;  // Blok yeniden başlat
                        block.x = (rand() % 200 - 100) / 100.0f;  // Yeni rastgele x pozisyonu
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
