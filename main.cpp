#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

float playerX = 0.0f; // Oyuncu X konumu
float playerSpeed = 0.05f; // Oyuncu hareket hızı
float blockSpeed = 0.02f; // Düşen blokların hızı
int score = 0; // Skor

struct Block {
    float x, y;
};

// Rastgele blokları saklamak için vektör
std::vector<Block> blocks;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_LEFT && playerX > -0.9f) playerX -= playerSpeed; // Sol
        if (key == GLFW_KEY_RIGHT && playerX < 0.9f) playerX += playerSpeed; // Sağ
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

int main() {
    srand(time(0));

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

    // Başlangıç blokları oluştur
    for (int i = 0; i < 5; i++) {
        blocks.push_back({(rand() % 200 - 100) / 100.0f, 1.0f});
    }

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Siyah arka plan
        glClear(GL_COLOR_BUFFER_BIT);

        // Oyuncuyu çiz
        drawRectangle(playerX, -0.8f, 0.1f, 0.1f, 0.0f, 1.0f, 0.0f); // Yeşil oyuncu

        // Blokları düşür ve çiz
        for (auto& block : blocks) {
            block.y -= blockSpeed;
            drawRectangle(block.x, block.y, 0.1f, 0.1f, 1.0f, 0.0f, 0.0f); // Kırmızı blok

            // Blok yere ulaştıysa yeniden yukarıda rastgele başlat
            if (block.y < -1.0f) {
                block.x = (rand() % 200 - 100) / 100.0f;
                block.y = 1.0f;
                score++; // Skoru artır
                std::cout << "Skor: " << score << std::endl;
            }

            // Çarpışma kontrolü
            if (block.y < -0.7f && block.y > -0.9f && block.x < playerX + 0.1f && block.x + 0.1f > playerX) {
                std::cout << "OYUN BİTTİ! Skor: " << score << std::endl;
                glfwSetWindowShouldClose(window, true);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
