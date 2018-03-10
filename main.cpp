#include <cmath>
#include <cstring>
#include <array>
#include <string>
#include <fstream>
#include <iostream>

#include "cube.h"

using namespace std;

#define GLFW_NO_GLU 1
#define GLFW_INCLUDE_GLCOREARB 1
#include "GLFW/glfw3.h"

#define STR(s) #s

#define glCall(x) \
do { \
    (x); \
    GLenum ret = glGetError(); \
    if (ret != GL_NO_ERROR) { \
        printf("error: %s returned %d\n", STR(x), int(ret)); \
        exit(1); \
    } \
} while(0)

#define SHADOWMAP_SIZE 4096

namespace {
string getFileAsString(const char *filename) {
    ifstream ifs(filename);
    if (!ifs)
        return string();
    return string((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());
}
}

MyMatrix lookAt(const MyPoint& eye, const MyPoint& center, const MyPoint& up)
{
    MyPoint f = (center - eye);
    f.normalize();

    MyPoint upn(up);
    upn.normalize();

    const MyPoint s = f.cross(upn);
    const MyPoint u = s.cross(f);

    MyMatrix rot;
    rot.set(0, 0, s.x);
    rot.set(0, 1, s.y);
    rot.set(0, 2, s.z);
    rot.set(1, 0, u.x);
    rot.set(1, 1, u.y);
    rot.set(1, 2, u.z);
    rot.set(2, 0, -f.x);
    rot.set(2, 1, -f.y);
    rot.set(2, 2, -f.z);

    MyMatrix trans;
    trans.set(0, 3, -eye.x);
    trans.set(1, 3, -eye.y);
    trans.set(2, 3, -eye.z);
    return rot * trans;
}

struct MyApp
{
    static void errorCallback(int error, const char *desc)
    {
        puts(desc);
    }

  public:
    MyApp() = default;
    MyApp(MyApp&) = delete;

    GLFWwindow *window = nullptr;
    void run()
    {
        bool running = true;

        if (!glfwInit()) {
            fprintf(stderr, "Failed to initialize GLFW\n");
            return;
        }
        glfwSetErrorCallback(errorCallback);


        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "CubeSolver",
                                  NULL, nullptr);
        if (!window) {
            puts("window creation failed");
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowUserPointer(window, this);
        glfwSetWindowSizeCallback(window, glfw_onResize);
        glfwSetKeyCallback(window, glfw_onKey);
        //glfwSetCursorPosCallback(window, glfw_onMouseMove);

        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        startup();

        do
        {
            render(glfwGetTime());

            glfwSwapBuffers(window);
            glfwPollEvents();

            running &= (glfwGetKey(window, GLFW_KEY_ESCAPE ) == GLFW_RELEASE);
            running &= !glfwWindowShouldClose(window);
        } while(running);

        shutdown();

        glfwTerminate();
    }
    static void glfw_onResize(GLFWwindow *window, int w, int h)
    {
        MyApp *app = (MyApp *) glfwGetWindowUserPointer(window);
        app->onResize(w, h);
    }

    static void glfw_onKey(GLFWwindow *window, int key, int scancode,
                           int action, int mods)
    {
        MyApp *app = (MyApp *) glfwGetWindowUserPointer(window);
        app->onKey(key, action);
    }

    static void glfw_onMouseMove(GLFWwindow *window, double x, double y)
    {
        MyApp *app = (MyApp *) glfwGetWindowUserPointer(window);
        app->onMouseMove(x, y);
    }

    string getShaderLog(GLuint shader)
    {
        string str(" ", 4096);
        GLint len;
        glGetShaderInfoLog(shader, 4096, &len, (GLchar *)str.c_str());
        str.resize(len);
        return str;
    }
    static string getProgramLog(GLuint program)
    {
        string str(" ", 4096);
        GLint len;
        glGetProgramInfoLog(program, 4096, &len, (GLchar *)str.c_str());
        str.resize(len);
        return str;
    }

    GLuint compileShader(const char *filename, GLenum shaderType)
    {
        GLuint shader = glCreateShader(shaderType);
        if (shader == 0) {
            printf("glCreateShader failed: %d\n", int(glGetError()));
            return 0;
        }
        string src = getFileAsString(filename);
        if (src.empty()) {
            printf("Could not read %s\n", filename);
            return 0;
        }
        GLchar *srcPtr = (GLchar *) src.c_str();
        glShaderSource(shader, 1, &srcPtr, 0);
        glCompileShader(shader);
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            printf("glCompileShader for %s failed: %s\n", filename,
                   getShaderLog(shader).c_str());
            return 0;
        }
        return shader;
    }

    GLuint program;
    GLuint shadowProgram;
    GLuint debugProgram;
    GLuint projMatrixLocation = -1;
    GLuint mvMatrixLocation;
    GLuint vertexTransformLocation;
    GLuint passThroughShader;
    GLuint shadowMapID;
    GLuint shadowMvpLoc;

    GLuint shadowMvLoc;
    GLuint shadowPerspectiveLoc;
    GLuint shadowVertexTransformLoc;

    GLuint lightPosLoc;

    GLuint getUniform(GLuint program, const char *name)
    {
        GLint ret = glGetUniformLocation(program, name);
        if (ret < 0) {
            printf("glGetUniformLocation for %s failed %d. glError %d\n",
                   name, ret, int(glGetError()));
            exit(1);
        }
        return ret;
    }

    GLuint debugTexIDLoc;
    bool compileShaders()
    {
        GLuint vertexShader = compileShader("vertex.glsl", GL_VERTEX_SHADER);
        GLuint fragmentShader = compileShader("fragment.glsl",
                                              GL_FRAGMENT_SHADER);

        if (!vertexShader || !fragmentShader) {
            return false;
        }

        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        projMatrixLocation = getUniform(program, "projMatrix");
        mvMatrixLocation = getUniform(program, "mvMatrix");
        vertexTransformLocation = getUniform(program, "vTransform");
        passThroughShader = getUniform(program, "passThroughShader");
        shadowMapID = getUniform(program, "shadowMap");
        shadowMvpLoc = getUniform(program, "shadowMvp");
        lightPosLoc = getUniform(program, "lightPos");

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            printf("link program failed: %s\n", getProgramLog(program).c_str());
            return false;
        }
        //glPointSize(5.0f);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        vertexShader = compileShader("vertex_shadowmap.glsl", GL_VERTEX_SHADER);
        fragmentShader = compileShader("fragment_shadowmap.glsl",
                                       GL_FRAGMENT_SHADER);

        if (!vertexShader || !fragmentShader) {
            return false;
        }

        shadowProgram = glCreateProgram();

        glAttachShader(shadowProgram, vertexShader);
        glAttachShader(shadowProgram, fragmentShader);
        glLinkProgram(shadowProgram);

        shadowPerspectiveLoc = getUniform(shadowProgram, "projMatrix");
        shadowMvLoc = getUniform(shadowProgram, "mvMatrix");
        shadowVertexTransformLoc = getUniform(shadowProgram, "vTransform");

        glGetProgramiv(shadowProgram, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            printf("link program failed: %s\n",
                   getProgramLog(shadowProgram).c_str());
            return false;
        }

        vertexShader = compileShader("vertex_passthrough.glsl",
                                     GL_VERTEX_SHADER);
        fragmentShader = compileShader("fragment_texture.glsl",
                                       GL_FRAGMENT_SHADER);

        if (!vertexShader || !fragmentShader) {
            return false;
        }

        debugProgram = glCreateProgram();

        glAttachShader(debugProgram, vertexShader);
        glAttachShader(debugProgram, fragmentShader);
        glLinkProgram(debugProgram);

        glGetProgramiv(debugProgram, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            printf("link program failed: %s\n",
                   getProgramLog(debugProgram).c_str());
            return false;
        }
        debugTexIDLoc = getUniform(debugProgram, "text");

        glUseProgram(program);

        return true;
    }

    GLuint vao;
    GLuint buffer;
    GLuint bufferColor;
    GLuint normals;

    MyRubik rubik;

    MyPoint groundVec[6];
    MyPoint groundColor[6];
    MyPoint groundNormal[6];

    GLuint frameBuf;
    GLuint depthTexture;

    void initFrameBuf()
    {

        glCall(glGenFramebuffers(1, &frameBuf));
        glCall(glBindFramebuffer(GL_FRAMEBUFFER, frameBuf));

        glCall(glGenTextures(1, &depthTexture));
        glCall(glBindTexture(GL_TEXTURE_2D, depthTexture));
        glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
                            SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0,
                            GL_DEPTH_COMPONENT, GL_FLOAT, 0));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                               GL_LINEAR));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                               GL_LINEAR));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                               GL_CLAMP_TO_EDGE));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                               GL_CLAMP_TO_EDGE));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC,
                               GL_LEQUAL));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                               GL_COMPARE_REF_TO_TEXTURE));

        glCall(glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                    depthTexture, 0));

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        GLenum ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (ret != GL_FRAMEBUFFER_COMPLETE) {
            printf("glCheckFramebufferStatus returned %d\n", int(ret));
            ret = glGetError();
            printf("GLError %d\n", int(ret));
            exit(1);
        }
    }

    GLuint quadVertexBuffer;
    void startup()
    {
        if (!compileShaders())
            exit(1);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        rubik.initialize();

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rubik.cubes) + sizeof(groundVec),
                     NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rubik.cubes), rubik.cubes);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &bufferColor);
        glBindBuffer(GL_ARRAY_BUFFER, bufferColor);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rubik.colors) + sizeof(groundColor)
                     , NULL,
                     GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rubik.colors), rubik.colors);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &normals);
        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glBufferData(GL_ARRAY_BUFFER,
                     sizeof(rubik.normals)+sizeof(groundNormal),
                     NULL,
                     GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(rubik.normals),
                        rubik.normals);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);

        constexpr float groundBase = 50.f;
        // -(Half of the rubik cube diagonal plus some)
        const float groundYBase = -sqrtf(3.0f)*1.5f*rubik.radius()-0.2f;
        constexpr float groundYDisp = 0.0f;

        groundVec[0].x = -groundBase;
        groundVec[0].y = groundYBase;
        groundVec[0].z = groundBase;

        groundVec[1].x = groundBase;
        groundVec[1].y = groundYBase;
        groundVec[1].z = groundBase;

        groundVec[2].x = groundBase;
        groundVec[2].y = groundYBase + groundYDisp;
        groundVec[2].z = -groundBase;

        groundVec[3].x = -groundBase;
        groundVec[3].y = groundYBase;
        groundVec[3].z = groundBase;

        groundVec[4].x = groundBase;
        groundVec[4].y = groundYBase + groundYDisp;
        groundVec[4].z = -groundBase;

        groundVec[5].x = -groundBase;
        groundVec[5].y = groundYBase + groundYDisp;
        groundVec[5].z = -groundBase;

        for (int i = 0; i < 6; ++i) {
            groundColor[i].x = 0.6f;
            groundColor[i].y = 0.7f;
            groundColor[i].z = 0.9f;
        }
        for (int i = 0; i < 6; ++i) {
            groundNormal[i].x = 0.0f;
            groundNormal[i].y = 1.0f;
            groundNormal[i].z = 0.0f;
        }

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(rubik.cubes),
                        sizeof(groundVec), groundVec);

        glBindBuffer(GL_ARRAY_BUFFER, bufferColor);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(rubik.colors),
                        sizeof(groundColor), groundColor);

        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(rubik.normals),
                        sizeof(groundNormal), groundNormal);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_MULTISAMPLE);

        const float aspect = (float) windowWidth / (float)windowHeight;
        projMatrix = perspective(50.0f, aspect, 0.1f, 1000.0f);


        glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE,
                           projMatrix.buf);
        glUniformMatrix4fv(vertexTransformLocation, 28, GL_FALSE,
                           (const GLfloat *) rubik.mTransforms);

        glUseProgram(shadowProgram);
        glUniformMatrix4fv(shadowVertexTransformLoc, 28, GL_FALSE,
                           (const GLfloat *) rubik.mTransforms);
        glUseProgram(program);

        // The quad's FBO. Used only for visualizing the shadowmap.
        constexpr GLfloat quadVertexBufferData[] = {
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        };

        glGenBuffers(1, &quadVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertexBufferData),
                     quadVertexBufferData, GL_STATIC_DRAW);

        initFrameBuf();

        resetState();
        glfwGetCursorPos(window, &curX, &curY);
    }

    void shutdown()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(program);
    }

    MyMatrix projMatrix;
    static MyMatrix perspective(float fovy, float aspect, float n, float f)
    {
        float q = 1.0f / tan((0.5f * fovy) * (M_PI/180.0));
        float A = q / aspect;
        float B = (n + f) / (n - f);
        float C = (2.0f * n * f) / (n - f);

        MyMatrix result;
        result.set(0, 0, A);
        result.set(1, 1, q);
        result.set(2, 2, B);
        result.set(2, 3, C);
        result.set(3, 2, -1.0f);

        return result;
    }

    static MyMatrix ortho(float left, float right, float bottom, float top,
                          float near, float far)
    {
        MyMatrix result;
        result.set(0, 0, 2.0f / (right - left));
        result.set(1, 1, 2.0f / (top - bottom));
        result.set(2, 2, 2.0f / (near - far));
        result.set(0, 3, (left + right) / (left - right));
        result.set(1, 3, (bottom + top) / (bottom - top));
        result.set(2, 3, (near + far) / (near - far));
        return result;
    }
    int windowWidth = 0;
    int windowHeight = 0;
    void onResize(int w, int h)
    {
        printf("resize %d %d\n", w, h);
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        float aspect = (float) windowWidth / (float)windowHeight;
        projMatrix = perspective(50.0f, aspect, 0.1f, 1000.0f);
        if (projMatrixLocation != -1) {
            glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE,
                               projMatrix.buf);
        }
    }

    MyQuaternion cubeRotStart;
    MyQuaternion cubeRotEnd;
    MyQuaternion cubeRot;

    bool inFaceRot = false;
    bool inViewRot = false;
    double rotStartTime;
    double rotLastFrame;
    struct FaceRotationInfo {
        int rotType = -1;
        bool inverse = false;
    };
    array<FaceRotationInfo, 4> queueRotType;
    FaceRotationInfo faceRotation;

    void startRot(const FaceRotationInfo& r)
    {
        if (inFaceRot == true || inViewRot) {
            for (int i = 0; i < 4; ++i) {
                if (queueRotType[i].rotType == -1) {
                    queueRotType[i] = r;
                    return;
                }
            }
            return;
        }
        faceRotation = r;
        inFaceRot = true;
        rotStartTime = glfwGetTime();
        rotLastFrame = rotStartTime;
        rubik.startRot(r.rotType, r.inverse);
    }

    int keyPress = -1;

    void processMoveKey(int key)
    {
        if (key == GLFW_KEY_SPACE) {
            resetState();
            return ;
        }

        rotStartTime = glfwGetTime();
        rotLastFrame = rotStartTime;
        inViewRot = true;

        MyQuaternion newRot;
        constexpr float angle = (M_PI/8.0f); // 22.5deg
        switch (key) {
          case GLFW_KEY_UP: newRot.rotateX(-angle); break;
          case GLFW_KEY_DOWN: newRot.rotateX(angle); break;
          case GLFW_KEY_LEFT: newRot.rotateY(-angle); break;
          case GLFW_KEY_RIGHT: newRot.rotateY(angle); break;
        }

        cubeRotStart = cubeRot;
        cubeRotEnd = newRot * cubeRot;
        cubeRotEnd.normalize();
    }

    void updateCamera()
    {
        cameraTransform = lookAt(eye, eyeDir, MyPoint(0, 1, 0));
    }

    MyPoint eye;
    MyPoint eyeDir;
    void resetState()
    {
        cubeRot.rotateY(M_PI / 4.0f); // 45deg

        eye.x = 0.0f;
        eye.y = 0.0f;
        eye.z = 5.0f;

        eyeDir = MyPoint();
        inCameraMove = false;

        updateCamera();
    }

    double curX, curY;
    float cameraPitch = 0.0;
    float cameraYaw = 0.0;
    void onMouseMove(double x, double y)
    {

        const float diffX = (x - curX) / 100.0f;
        const float diffY = (y - curY) / 100.0f;

        eyeDir.x += diffX;
        eyeDir.y -= diffY;
        updateCamera();

        curX = x;
        curY = y;
    }

    bool inCameraMove = false;
    bool isXCameraMove = false;
    float cameraAdjust = 0.0f;
    int maxCameraMoveIncr = 0;
    int numCameraMoveIncr = 0;
    bool cameraKeyStillPressed = false;

    int currentCameraMoveKey;
    void onKey(int key, int action)
    {
        static bool shiftOn = false;
        if (key == GLFW_KEY_LEFT_SHIFT
         || key == GLFW_KEY_RIGHT_SHIFT) {
            shiftOn = (action == GLFW_PRESS);
        }

        if (GLFW_KEY_UP == key
         || GLFW_KEY_DOWN == key
         || GLFW_KEY_LEFT == key
         || GLFW_KEY_RIGHT == key
         || GLFW_KEY_SPACE == key) {
            if (inFaceRot) {
                return;

            }
            if (action == GLFW_RELEASE) {
                keyPress = -1;
                return;
            }
            if (key != GLFW_KEY_SPACE) {
                keyPress = key;
            }
            else if (action != GLFW_PRESS) {
                return;
            }
            processMoveKey(key);
            return;
        }


        if (key == GLFW_KEY_S
         || key == GLFW_KEY_X
         || key == GLFW_KEY_Z
         || key == GLFW_KEY_C) {
            if (inCameraMove) {
                // If we're already moving, we're just updating if the
                // current key is still pressed or not
                if (key == currentCameraMoveKey) {
                    cameraKeyStillPressed = (action != GLFW_RELEASE);
                }
                return;
            }

            if (action == GLFW_RELEASE) {
                return;
            }

            float adjust = 0.1f / 10.0f;
            maxCameraMoveIncr = 10;
            switch (key) {
              case GLFW_KEY_C: isXCameraMove = true;
                               cameraAdjust = adjust; break;
              case GLFW_KEY_Z: isXCameraMove = true;
                               cameraAdjust = -adjust; break;
              case GLFW_KEY_X: isXCameraMove = false;
                               cameraAdjust = -adjust; break;
              case GLFW_KEY_S: isXCameraMove = false;
                               cameraAdjust = adjust; break;
            }
            numCameraMoveIncr = 0;
            inCameraMove = true;
            cameraKeyStillPressed = true;
            currentCameraMoveKey = key;
            return;
        }

        if (action != GLFW_PRESS) {
            return;
        }

        MyPoint direction;
        switch (key) {
          case 'U': direction.y = 1.0f; break;
          case 'F': direction.z = 1.0f; break;
          case 'R': direction.x = 1.0f; break;
          case 'L': direction.x = -1.0f; break;
          case 'D': direction.y = -1.0f; break;
          case 'B': direction.z = -1.0f; break;
          default: return;
        }

        MyMatrix mv = cubeRot.toMatrix();
        float max;
        FaceRotationInfo r;
        r.inverse = shiftOn;
        for (int i = 0; i < 6; ++i) {
            const MyPoint face = rubik.faceNormal[i].transform(mv);
            const float d = face.dot(direction);
            if (i == 0 || max < d) {
                r.rotType = i;
                max = d;
            }
        }
        startRot(r);
    }

    static constexpr float totRotTime = 0.4f;
    static constexpr float totRotTime2 = 0.3f;

    int frames = 0;
    double start;
    MyMatrix cameraTransform;
    double prevFps = 1.0;
    void render(double currentTime)
    {
        if (frames == 0) {
            start = currentTime;
        }
        ++frames;

        if (inFaceRot) {
            const float t = float(currentTime - rotStartTime) / totRotTime;
            if (t >= 1.0f) {
                inFaceRot = false;
                rubik.endRot((int) faceRotation.rotType,
                             faceRotation.inverse);
                if (queueRotType[0].rotType != -1) {
                    FaceRotationInfo next = queueRotType[0];
                    queueRotType[0] = queueRotType[1];
                    queueRotType[1] = queueRotType[2];
                    queueRotType[2] = queueRotType[3];
                    queueRotType[3].rotType = -1;
                    startRot(next);
                }
            }
            else {
                rubik.doIncRot(faceRotation.rotType, t);
            }
            glUseProgram(program);
            glCall(glUniformMatrix4fv(vertexTransformLocation, 28, GL_FALSE,
                               (const GLfloat *) rubik.mTransforms));
            glUseProgram(shadowProgram);
            glCall(glUniformMatrix4fv(shadowVertexTransformLoc, 28, GL_FALSE,
                               (const GLfloat *) rubik.mTransforms));
        }
        if (inViewRot) {
            const float t = float(currentTime - rotStartTime) / totRotTime2;
            if (t >= 1.0f) {
                inViewRot = false;
                cubeRot = cubeRotEnd;
                if (queueRotType[0].rotType != -1) {
                    FaceRotationInfo next = queueRotType[0];
                    queueRotType[0] = queueRotType[1];
                    queueRotType[1] = queueRotType[2];
                    queueRotType[2] = queueRotType[3];
                    queueRotType[3].rotType = -1;
                    startRot(next);
                }
                else if (keyPress != -1) {
                    processMoveKey(keyPress);
                }
            }
            else {
                cubeRot = MyQuaternion::slerp(cubeRotStart, cubeRotEnd, t);
            }
        }
        MyMatrix mCubeRot = cubeRot.toMatrix();

        if (inCameraMove) {
            if (isXCameraMove) {
                eye.x += cameraAdjust;
                eyeDir.x += cameraAdjust;
            }
            else {
                eye.y += cameraAdjust;
                eyeDir.y += cameraAdjust;
            }
            ++numCameraMoveIncr;
            if (numCameraMoveIncr == maxCameraMoveIncr) {
                if (cameraKeyStillPressed) {
                    numCameraMoveIncr = 0;
                }
                else {
                    inCameraMove = false;
                }
            }
            updateCamera();
        }

        // Render shadow into shadow map
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuf);
        glViewport(0,0,SHADOWMAP_SIZE,SHADOWMAP_SIZE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        constexpr GLfloat b[] = { 0.0f,0.0f,0.0f };
        glClearBufferfv(GL_COLOR, 0, b);
        glUseProgram(shadowProgram);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

#if 0
        // XXX
        MyPoint lightPos(0.0f, 5.0f, 0.0f);
        MyPoint lightInvDir(0.0f,1.0f,0.0f);
        //MyPoint lightPos(2.0f, 1.0f, 0.0f);
        //MyPoint lightInvDir(1.0f,3.0f,1.0f);
        MyMatrix l = lookAt(lightInvDir,
                            MyPoint(),
                            MyPoint(1.0f,0.0f,0.0f));
        MyMatrix p = ortho(-5, 5, -5, 5, 0, 10.0);
#else
        MyPoint lightPos(0.0f,50.0f,5.0f);
        MyPoint lightTarget(0.0f,
                            1.5f*rubik.radius(),
                            1.5f*rubik.radius());
        MyMatrix l = lookAt(lightPos, lightTarget,
                            MyPoint(0,-1.0f,10.0f));
        //MyPoint lightPos = (lightTarget + lightInvDir);
        MyMatrix p = ortho(-5, 5, -5, 5, 0.1, 100.0);
#endif

        glUniformMatrix4fv(shadowPerspectiveLoc, 1, GL_FALSE, p.buf);
        MyMatrix tmp = l * mCubeRot;
        glCall(glUniformMatrix4fv(shadowMvLoc, 1, GL_FALSE, tmp.buf));
        glDrawArrays(GL_TRIANGLES, 0, 36*27);

        // Switch back the program
        glUseProgram(program);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, windowWidth, windowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Bind shadowmap texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(shadowMapID, 0);

        constexpr GLfloat background[] = { 210.0f/255.0f, 230.0f/255.0f,
                                           255.0f/255.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, background);

        // Render the ground with shadow
        MyMatrix fullMv = cameraTransform;
        glUniform1i(passThroughShader, 1);
        glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, fullMv.buf);
        tmp = l;
        tmp = p * tmp;
        glCall(glUniformMatrix4fv(shadowMvpLoc, 1, GL_FALSE, tmp.buf));
        glDrawArrays(GL_TRIANGLES, 36*27, 6);

        // Render the cube with shadow
        glUniform1i(passThroughShader, 0);
        MyMatrix cubeMv = fullMv * mCubeRot;
        tmp = l * mCubeRot;
        tmp = p * tmp;
        glUniformMatrix4fv(shadowMvpLoc, 1, GL_FALSE, tmp.buf);
        glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, cubeMv.buf);
        lightPos = lightPos.transform(fullMv);
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

        glDrawArrays(GL_TRIANGLES, 0, 36*27);

        // To debug the shadow
#if 1
        glUseProgram(debugProgram);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glViewport(0, 0, 256, 256);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(debugTexIDLoc, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, bufferColor);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);
#endif

        if (frames == 100) {
            frames = 0;
            double fps = 100.0 / (currentTime - start);
            double diff = fps / prevFps;
            if (diff > 1.1 || diff < 0.9) {
                printf("fps %.2lf\n", fps);
            }
            prevFps = fps;
        }
    }
};

int main(void) {
    MyApp app;
    app.run();
}

