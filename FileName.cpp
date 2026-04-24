// =======================
// =======================
#include <iostream>
#include <cmath>
#include <fstream>
#include <cstdlib>
#include <ctime>

// مكتبات OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// مكتبات الرياضيات (GLM)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =======================
// 🪟 إعدادات النافذة
// =======================
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// =======================
//  اللاعب
// =======================
glm::vec3 cubePos(0.0f, 0.0f, 0.0f); // موقع المكعب

// =======================
//  الوقت
// =======================
float deltaTime = 0.0f; // الفرق بين الفريمات
float lastFrame = 0.0f;

// =======================
//  السرعة والطريق
// =======================
float speed = 5.0f;
float textureOffset = 0.0f; // لتحريك الطريق

// =======================
// حالة اللعبة
// =======================
bool gameOver = false;

// =======================
// 🪂 القفز
// =======================
bool isJumping = false;     // هل اللاعب يقفز
float jumpVelocity = 0.0f;  // سرعة القفز
float gravity = -18.0f;     // الجاذبية

// =======================
//  الحواجز
// =======================
const int obstacleCount = 20;
glm::vec3 obstacles[obstacleCount];

// =======================
// 🪙 العملات
// =======================
const int coinCount = 5;
glm::vec3 coins[coinCount];
bool coinActive[coinCount];
int score = 0;

// =======================
//  حدود الطريق
// =======================
float roadHalfWidth = 4.5f;
float cubeHalf = 0.2f;

// =======================
//  Shader الأساسي
// =======================
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"layout (location = 2) in vec2 aTexCoord;\n"
"out vec3 ourColor;\n"
"out vec2 TexCoord;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"uniform float offset;\n"
"void main()\n"
"{\n"
"    // تحويل الإحداثيات لعالم 3D\n"
"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"    ourColor = aColor;\n"
"\n"
"    // تحريك التكستشر (الطريق)\n"
"    TexCoord = vec2(aTexCoord.x, aTexCoord.y + offset);\n"
"}\n";

// =======================
//  Fragment Shader
// =======================
const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"in vec2 TexCoord;\n"
"uniform sampler2D texture1;\n"
"uniform bool useTexture;\n"
"void main()\n"
"{\n"
"    // إذا في texture نستخدمه\n"
"    if(useTexture)\n"
"        FragColor = texture(texture1, TexCoord);\n"
"    else\n"
"        FragColor = vec4(ourColor, 1.0);\n"
"}\n";

// =======================
//  Shader للـ HUD
// =======================
const char* hudVertexShader =
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"void main()\n"
"{\n"
"    // رسم مباشر بدون كاميرا\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"}\n";

const char* hudFragmentShader =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec3 hudColor;\n"
"void main()\n"
"{\n"
"    // لون الشريط (أخضر/أحمر)\n"
"    FragColor = vec4(hudColor, 1.0);\n"
"}\n";

// =======================
//  تحميل Texture BMP
// =======================
unsigned int loadBMPTexture(const char* filename)
{
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int width;
    unsigned int height;
    unsigned int imageSize;

    std::ifstream file(filename, std::ios::binary);

    if (!file)
    {
        std::cout << "Failed to open texture file\n";
        return 0;
    }

    // قراءة الهيدر
    file.read((char*)header, 54);

    // استخراج معلومات الصورة
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);

    if (imageSize == 0)
        imageSize = width * height * 3;

    if (dataPos == 0)
        dataPos = 54;

    unsigned char* data = new unsigned char[imageSize];

    file.seekg(dataPos);
    file.read((char*)data, imageSize);
    file.close();

    // إنشاء Texture
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
        GL_BGR, GL_UNSIGNED_BYTE, data);

    glGenerateMipmap(GL_TEXTURE_2D);

    delete[] data;

    return textureID;
}
// =======================
// دوال التصريح
// =======================
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void resetGame();

void resetGame()
{
    cubePos = glm::vec3(0.0f, 0.0f, 0.0f);
    speed = 5.0f;
    textureOffset = 0.0f;
    gameOver = false;
    isJumping = false;
    jumpVelocity = 0.0f;
    score = 0;

    // توزيع الحواجز عشوائياً على 3 مسارات
    for (int i = 0; i < obstacleCount; i++)
    {
        int lane = rand() % 3;
        obstacles[i] = glm::vec3((lane - 1) * 2.0f, 0.0f, -(i * 8.0f + 10.0f));
    }

    // توزيع العملات عشوائياً على الطريق
    for (int i = 0; i < coinCount; i++)
    {
        int lane = rand() % 3;
        coins[i] = glm::vec3((lane - 1) * 2.0f, 0.4f, -(rand() % 50 + 20));
        coinActive[i] = true;
    }
}

// =======================


// =======================
// البرنامج الرئيسي
// =======================
int main()
{
    srand((unsigned int)time(0));

    glfwInit();

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Game HUD Coins Jump", NULL, NULL);
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);

    // =======================
    // إنشاء shader اللعبة
    // =======================
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);

    // =======================
    // إنشاء shader  HUD الـ
    // =======================
    unsigned int hvs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(hvs, 1, &hudVertexShader, NULL);
    glCompileShader(hvs);

    unsigned int hfs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(hfs, 1, &hudFragmentShader, NULL);
    glCompileShader(hfs);

    unsigned int hudProgram = glCreateProgram();
    glAttachShader(hudProgram, hvs);
    glAttachShader(hudProgram, hfs);
    glLinkProgram(hudProgram);

    // =======================
    // بيانات المكعب للاعب والحواجز
    // =======================
    float cube[] = {
        -0.5,-0.5,-0.5, 1,0,0,  0.5,-0.5,-0.5, 1,0,0,  0.5,0.5,-0.5, 1,0,0,
         0.5,0.5,-0.5, 1,0,0, -0.5,0.5,-0.5, 1,0,0, -0.5,-0.5,-0.5, 1,0,0,

        -0.5,-0.5,0.5, 0,1,0,  0.5,-0.5,0.5, 0,1,0,  0.5,0.5,0.5, 0,1,0,
         0.5,0.5,0.5, 0,1,0, -0.5,0.5,0.5, 0,1,0, -0.5,-0.5,0.5, 0,1,0,

        -0.5,0.5,0.5, 0,0,1, -0.5,0.5,-0.5, 0,0,1, -0.5,-0.5,-0.5, 0,0,1,
        -0.5,-0.5,-0.5, 0,0,1, -0.5,-0.5,0.5, 0,0,1, -0.5,0.5,0.5, 0,0,1,

         0.5,0.5,0.5, 1,1,0,  0.5,0.5,-0.5, 1,1,0,  0.5,-0.5,-0.5, 1,1,0,
         0.5,-0.5,-0.5, 1,1,0,  0.5,-0.5,0.5, 1,1,0,  0.5,0.5,0.5, 1,1,0,

        -0.5,-0.5,-0.5, 0,1,1,  0.5,-0.5,-0.5, 0,1,1,  0.5,-0.5,0.5, 0,1,1,
         0.5,-0.5,0.5, 0,1,1, -0.5,-0.5,0.5, 0,1,1, -0.5,-0.5,-0.5, 0,1,1,

        -0.5,0.5,-0.5, 1,0,1,  0.5,0.5,-0.5, 1,0,1,  0.5,0.5,0.5, 1,0,1,
         0.5,0.5,0.5, 1,0,1, -0.5,0.5,0.5, 1,0,1, -0.5,0.5,-0.5, 1,0,1
    };

    // =======================
    // بيانات الطريق
    // =======================
    float road[] = {
        -5,-0.5, 50, 1,1,1, 0,10,
         5,-0.5, 50, 1,1,1, 1,10,
         5,-0.5,-50, 1,1,1, 1,0,

         5,-0.5,-50, 1,1,1, 1,0,
        -5,-0.5,-50, 1,1,1, 0,0,
        -5,-0.5, 50, 1,1,1, 0,10
    };

    // =======================
    // مستطيل العملة
    // =======================
    float quad[] = {
        -0.5f,  0.5f, 0.0f, 1,1,1, 0,1,
         0.5f,  0.5f, 0.0f, 1,1,1, 1,1,
         0.5f, -0.5f, 0.0f, 1,1,1, 1,0,

         0.5f, -0.5f, 0.0f, 1,1,1, 1,0,
        -0.5f, -0.5f, 0.0f, 1,1,1, 0,0,
        -0.5f,  0.5f, 0.0f, 1,1,1, 0,1
    };

    // =======================
    // مستطيل الHUD 
    // =======================
    float hud[] = {
        -1.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, 0.9f,

         1.0f, 0.9f,
        -1.0f, 0.9f,
        -1.0f, 1.0f
    };

    // تجهيز VAO/VBO للمكعب
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // تجهيز VAO/VBO للطريق
    unsigned int roadVAO, roadVBO;
    glGenVertexArrays(1, &roadVAO);
    glGenBuffers(1, &roadVBO);

    glBindVertexArray(roadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(road), road, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // تجهيز VAO/VBO للعملات
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // تجهيز VAO/VBO للـ HUD
    unsigned int hudVAO, hudVBO;
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);

    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(hud), hud, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // تحميل الصور
    unsigned int roadTexture = loadBMPTexture("road.bmp");
    unsigned int coinTexture = loadBMPTexture("coin.bmp");

    resetGame();

    // =======================
    // حلقة اللعبة
    // =======================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        if (!gameOver)
        {
            speed += 0.5f * deltaTime;
            textureOffset += speed * deltaTime * 0.5f;

            // تطبيق القفز والجاذبية
            if (isJumping)
            {
                cubePos.y += jumpVelocity * deltaTime;
                jumpVelocity += gravity * deltaTime;

                if (cubePos.y <= 0.0f)
                {
                    cubePos.y = 0.0f;
                    isJumping = false;
                    jumpVelocity = 0.0f;
                }
            }

            // تحريك الحواجز وفحص التصادم
            for (int i = 0; i < obstacleCount; i++)
            {
                obstacles[i].z += speed * deltaTime;

                if (obstacles[i].z > 2.0f)
                {
                    int lane = rand() % 3;
                    obstacles[i].x = (lane - 1) * 2.0f;
                    obstacles[i].z = -100.0f;
                }

                float dx = fabs(cubePos.x - obstacles[i].x);
                float dy = fabs(cubePos.y - obstacles[i].y);
                float dz = fabs(cubePos.z - obstacles[i].z);

                if (dx < 0.8f && dy < 1.5f && dz < 0.6f)
                    gameOver = true;
            }

            // تحريك العملات وفحص جمعها
            for (int i = 0; i < coinCount; i++)
            {
                coins[i].z += speed * deltaTime;

                if (coins[i].z > 2.0f)
                {
                    int lane = rand() % 3;
                    coins[i].x = (lane - 1) * 2.0f;
                    coins[i].y = 0.4f;
                    coins[i].z = -(rand() % 50 + 20);
                    coinActive[i] = true;
                }

                float dx = fabs(cubePos.x - coins[i].x);
                float dy = fabs(cubePos.y - coins[i].y);
                float dz = fabs(cubePos.z - coins[i].z);

                if (coinActive[i] && dx < 0.7f && dy < 1.0f && dz < 0.7f)
                {
                    coinActive[i] = false;
                    score++;
                    std::cout << "Score: " << score << std::endl;
                }
            }

            // خسارة إذا خرج اللاعب من الطريق
            if (cubePos.x > roadHalfWidth - cubeHalf ||
                cubePos.x < -roadHalfWidth + cubeHalf)
            {
                gameOver = true;
            }
        }

        // لون الـ HUD حسب حالة اللعبة
        glm::vec3 hudColor = gameOver ?
            glm::vec3(1.0f, 0.0f, 0.0f) :
            glm::vec3(0.0f, 1.0f, 0.0f);

        // تنظيف الشاشة ولون السماء
        glClearColor(0.4f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // الكاميرا تتبع اللاعب
        glm::vec3 cameraPos = cubePos + glm::vec3(0.0f, 4.0f, 12.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cubePos, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)SCR_WIDTH / SCR_HEIGHT,
            0.1f,
            100.0f
        );

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform1f(glGetUniformLocation(shaderProgram, "offset"), textureOffset);

        // رسم الطريق
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), true);
        glBindTexture(GL_TEXTURE_2D, roadTexture);

        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(roadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // رسم اللاعب
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), false);

        model = glm::translate(glm::mat4(1.0f), cubePos);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // رسم الحواجز
        for (int i = 0; i < obstacleCount; i++)
        {
            model = glm::translate(glm::mat4(1.0f), obstacles[i]);
            model = glm::scale(model, glm::vec3(0.5f, 2.0f, 0.5f));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // رسم العملات باستخدام coin.bmp
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), true);
        glBindTexture(GL_TEXTURE_2D, coinTexture);
        glUniform1f(glGetUniformLocation(shaderProgram, "offset"), 0.0f);

        for (int i = 0; i < coinCount; i++)
        {
            if (coinActive[i])
            {
                model = glm::translate(glm::mat4(1.0f), coins[i]);
                model = glm::scale(model, glm::vec3(0.7f));

                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        // رسم HUD فوق كل شيء
        glUseProgram(hudProgram);
        glUniform3fv(glGetUniformLocation(hudProgram, "hudColor"), 1, glm::value_ptr(hudColor));

        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
     // التحكم بالكيبورد
     // =======================
   void processInput(GLFWwindow* window)
    {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // إعادة اللعبة
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        resetGame();

    if (!gameOver)
    {
        float sideSpeed = 3.0f * deltaTime;

        // حركة يمين ويسار
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cubePos.x -= sideSpeed;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cubePos.x += sideSpeed;

    }
}
// تحديث حجم العرض عند تغيير حجم النافذة
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
