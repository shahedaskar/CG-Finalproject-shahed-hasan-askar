#include <iostream>
#include <cmath>
#define GLEW_STATIC 
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- إعدادات النافذة ---
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// --- متغيرات الكاميرا والشخصية ---
glm::vec3 cameraPos = glm::vec3(0.0f, 3.5f, 8.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 0.5f, 5.0f);

// --- متغيرات الفأرة والزوايا ---
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

// --- متغيرات الوقت ---
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 cubePos = glm::vec3(0.0f, 0.0f,0.0f);
//متغير السرعة
float speed = 5.0f;
const int obstacleCount = 20;
glm::vec3 obstacles[20];
//تحديد عرض الطريق
float roadHalfWidth = 4.8f; // نص عرض الطريق
float cubeHalf = 0.3f;      //
float distanceTravelled = 0.001f;
float cycleLength = 150.0f; // كل 50 وحدة مسافة = دورة كاملة
//متغيرات العملات
const int coinCount = 5;
glm::vec3 coins[coinCount];
bool coinActive[coinCount];
int score=0;

// --- نصوص المظلات (Shaders) المعدلة ---
// Vertex Shader: الآن يستقبل اللون كمدخل ثاني (location = 1) ويمرره
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

// Fragment Shader: الآن يستقبل اللون من Vertex Shader ويعرضه
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0f);\n"
"}\n\0";

// --- التصاريح المسبقة للدوال ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
//void mouse_callback(GLFWwindow* window, double xpos, double ypos);//
void processInput(GLFWwindow* window);

void resetGame()
{
    // رجّع اللاعب
    cubePos = glm::vec3(0.0f, 0.0f, 0.0f);

    // رجّع السرعة
    speed = 5.0f;

    // رجّع الحواجز
    for (int i = 0; i < obstacleCount; i++)
    {
        float z = -(i * 8.0f + 10.0f);

        int lane = rand() % 3;

        float x;
        if (lane == 0) x = -2.0f;
        else if (lane == 1) x = 0.0f;
        else x = 2.0f;

        obstacles[i] = glm::vec3(x, 0.0f, z);
    }
}


int main()
{
    // تهيئة مكتبة GLFW وإعداد النافذة
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "مشروع الكاميرا والمكعب - ملون", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "فشل في إنشاء نافذة GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // ربط دوال الاستدعاء (Callbacks)
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetCursorPosCallback(window, mouse_callback);//

    // إخفاء مؤشر الفأرة والتقاطه داخل النافذة
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);//

    // تهيئة مكتبة GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "فشل في تهيئة مكتبة GLEW" << std::endl;
        return -1;
    }

    // تفعيل اختبار العمق (Depth Testing)
    glEnable(GL_DEPTH_TEST);

    // --- بناء المظلات (Compile Shaders) ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- إعداد نقاط المكعب والألوان الجديدة ---
    // كل سطر يمثل نقطة: 3 إحداثيات (x, y, z) + 3 ألوان (r, g, b)
    float vertices[] = {
        // الوجه الخلفي (أحمر)
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,

        // الوجه الأمامي (أخضر)
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,

        // الوجه الأيسر (أزرق)
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,

        // الوجه الأيمن (أصفر)
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

         // الوجه السفلي (سماوي)
         -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
         -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,

         // الوجه العلوي (بنفسجي)
         -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f
    };
    // --- بيانات الطريق: مستطيل طويل ---
    float roadVertices[] = {
        // x, y, z, r, g, b
        -5.0f, -0.5f, 50.0f,  0.3f, 0.3f, 0.3f,  // أعلى يسار
         5.0f, -0.5f, 50.0f,  0.3f, 0.3f, 0.3f,  // أعلى يمين
         5.0f, -0.5f,-50.0f,  0.3f, 0.3f, 0.3f,  // أسفل يمين

         5.0f, -0.5f,-50.0f,  0.3f, 0.3f, 0.3f,  // أسفل يمين
        -5.0f, -0.5f,-50.0f,  0.3f, 0.3f, 0.3f,  // أسفل يسار
        -5.0f, -0.5f, 50.0f,  0.3f, 0.3f, 0.3f   // أعلى يسار
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 1. مؤشر الإحداثيات (الموقع)
    // الخطوة (Stride) أصبحت 6 الآن بدلاً من 3 (3 إحداثيات + 3 ألوان)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. مؤشر الألوان الجديد
    // يبدأ من الإزاحة (Offset) رقم 3
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    unsigned int roadVAO, roadVBO;
    glGenVertexArrays(1, &roadVAO);
    glGenBuffers(1, &roadVBO);

    glBindVertexArray(roadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roadVertices), roadVertices, GL_STATIC_DRAW);

    // موقع الإحداثيات
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // موقع اللون
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glEnable(GL_DEPTH_TEST);

    // --- حلقة الرسم الأساسية (Render Loop) ---
    glm::vec3 cubePosition = glm::vec3(0.0f, 0.0f, 0.0f); // x, y, z
    float moveSpeed = 2.5f; // سرعة الحركة
    unsigned int modelLoc;
    

    for (int i = 0; i < obstacleCount; i++)
{
    float z = - (i * 6.0f + 2.0f); // مسافة بين كل حاجز

    int lane = rand() % 3; // 0 أو 1 أو 2

    float x;

    if (lane == 0) x = -2.0f; // يسار
    else if (lane == 1) x = 0.0f; // وسط
    else x = 2.0f; // يمين

    obstacles[i]= glm::vec3(x, 0.0f, z);
}
    //تهيئة العملات
    for (int i = 0; i < coinCount; i++)
    {
        coins[i].z = -(rand() % 50 + 20); // بعيد قدام
        coins[i].y = 0.0f;

        int lane = rand() % 3;

        if (lane == 0) coins[i].x = -2.0f;
        else if (lane == 1) coins[i].x = 0.0f;
        else coins[i].x = 2.0f;

        coinActive[i] = true;
    }



    float cycleLength = 40.0f; // طول الدورة (ليل + نهار)
  
    while (!glfwWindowShouldClose(window))
    {//حساب الوقت
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        speed += 0.5f * deltaTime;
        lastFrame = currentFrame;
        distanceTravelled += speed * deltaTime;
        
            for (int i = 0; i < coinCount; i++)
            {
                coins[i].z += speed * deltaTime;
            }
        //تحريك المكعب
        processInput(window);

        for (int i = 0; i < coinCount; i++)
        {
            coins[i].z += speed * deltaTime;

            // إذا وصلت عند اللاعب → رجعها
            if (coins[i].z > 2.0f)
            {
                coins[i].z = -(rand() % 50 + 20);

                int lane = rand() % 3;

                if (lane == 0) coins[i].x = -2.0f;
                else if (lane == 1) coins[i].x = 0.0f;
                else coins[i].x = 2.0f;

                coinActive[i] = true;
            }
            float dx = abs(cubePos.x - coins[i].x);
            float dz = abs(cubePos.z - coins[i].z);

            if (coinActive[i] && dx < 0.5f && dz < 0.5f)
            {
                coinActive[i] = false; // تختفي
                score++;               // يزيد العداد
            }

        }


        //التصادم
        for (int i = 0; i < obstacleCount; i++)
        {
            float cubeHalf = 0.3f;
            float obstacleHalf = 0.3f;

            float dx = abs(cubePos.x - obstacles[i].x);
            float dz = abs(cubePos.z - obstacles[i].z);

            if (dx < (cubeHalf + obstacleHalf + 0.3f) &&   // هامش زيادة
                dz < (cubeHalf + obstacleHalf))
            {
               resetGame(); // 🔥 هون الريستارت
        break;       // مهم جداً
            }
        }if (cubePos.x > roadHalfWidth - cubeHalf ||
            cubePos.x < -roadHalfWidth + cubeHalf)
        {
            resetGame();
        }
        
      
        
         // من 0 إلى 1
        float cycleLength = 300.0f;   // طول الدورة كاملة (ليل + نهار)
        float transition = 30.0f;     // مدة الانتقال (سرعة التحول)

        float cycle = fmod(distanceTravelled, cycleLength);

        // 🌙 ليل
        float r, g, b;

        float rNight = 0.05f, gNight = 0.05f, bNight = 0.1f;
        float rDay = 0.5f, gDay = 0.8f, bDay = 1.0f;

        if (cycle < (cycleLength / 2 - transition / 2))
        {
            // 🌙 ليل ثابت
            r = rNight; g = gNight; b = bNight;
        }
        else if (cycle < (cycleLength / 2 + transition / 2))
        {
            // 🔄 انتقال ليل → نهار
            float t = (cycle - (cycleLength / 2 - transition / 2)) / transition;

            r = rNight * (1 - t) + rDay * t;
            g = gNight * (1 - t) + gDay * t;
            b = bNight * (1 - t) + bDay * t;
        }
        else if (cycle < (cycleLength - transition / 2))
        {
            // ☀️ نهار ثابت
            r = rDay; g = gDay; b = bDay;
        }
        else
        {
            // 🔄 انتقال نهار → ليل
            float t = (cycle - (cycleLength - transition / 2)) / transition;

            r = rDay * (1 - t) + rNight * t;
            g = gDay * (1 - t) + gNight * t;
            b = bDay * (1 - t) + bNight * t;
        }

        glClearColor(r, g, b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       
        glUseProgram(shaderProgram);

        glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 5.0f); // ارتفاع + مسافة خلف المكعب
        glm::vec3 cameraPos = cubePos + glm::vec3(0.0f, 4.0f, 12.0f);
        glm::vec3 cameraTarget = cubePos + glm::vec3(0.0f, 0.0f, 0.0f);
        
        
        // الكاميرا دائماً تتجه للمكعب
        glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
        // glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);//


        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        // --- رسم الطريق ---
        glm::mat4 roadModel = glm::mat4(1.0f); // الطريق ثابت
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(roadModel));

        glBindVertexArray(roadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // 6 vertices للطريق
        // ===== رسم اللاعب =====
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cubePos);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
       
        for (int i = 0; i < obstacleCount; i++)
        {
            obstacles[i].z += speed * deltaTime; // سرعة الحركة
            if (obstacles[i].z > 2.0f) // وصل للاعب
            {
                obstacles[i].z = -100.0f; // رجع لبعيد

                // غير المسار (يمين/يسار/وسط)
                int lane = rand() % 3;

                if (lane == 0) obstacles[i].x = -2.0f;
                else if (lane == 1) obstacles[i].x = 0.0f;
                else obstacles[i].x = 2.0f;
            }
            glm::mat4 obstacleModel = glm::mat4(1.0f);

            obstacleModel = glm::translate(obstacleModel, obstacles[i]);

            // خليها أعمدة
            obstacleModel = glm::scale(obstacleModel, glm::vec3(0.5f, 2.0f, 0.5f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(obstacleModel));

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        for (int i = 0; i < coinCount; i++)
        {
            if (coinActive[i])
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, coins[i]);
                model = glm::scale(model, glm::vec3(0.3f));

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                // 🟡 لون العملة
               

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }


       

        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // تنظيف الذاكرة قبل الإغلاق
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// --- معالجة مدخلات لوحة المفاتيح (الأسهم) ---
void processInput(GLFWwindow* window)
{  // if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
     //   glfwSetWindowShouldClose(window, true);

    //float cameraSpeed = 2.5f * deltaTime;

    //if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)                         
        //cameraPos += cameraSpeed * cameraFront;
    //if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        //cameraPos -= cameraSpeed * cameraFront;
    //if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        //cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    //if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
      //  cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    float moveSpeed = 30.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cubePos.z -= moveSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cubePos.z += moveSpeed * deltaTime;

    moveSpeed = 0.03f;

    // يمين (D)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cubePos.x += moveSpeed;

    // يسار (A)
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cubePos.x -= moveSpeed;








}

// --- معالجة حركة الفأرة (الالتفاف الحر) ---
//void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)//
//{
    //float xpos = static_cast<float>(xposIn);
    //float ypos = static_cast<float>(yposIn);

    //if (firstMouse)
    //{
       // lastX = xpos;
        //lastY = ypos;
      //  firstMouse = false;
    //}

    //float xoffset = xpos - lastX;
    //float yoffset = lastY - ypos;
    //lastX = xpos;
    //lastY = ypos;

    //float sensitivity = 0.1f;
    //xoffset *= sensitivity;
    //yoffset *= sensitivity;

    //yaw += xoffset;
    //pitch += yoffset;

    //if (pitch > 89.0f)
      //  pitch = 89.0f;
    //if (pitch < -89.0f)
    //    pitch = -89.0f;

    //glm::vec3 front;
    //front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    //front.y = sin(glm::radians(pitch));
    //front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

   // cameraFront = glm::normalize(front);//
//}

// --- معالجة تغير حجم النافذة ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}