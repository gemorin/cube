#include <cmath>
#include <cstring>
#include <array>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;


#define GLFW_NO_GLU 1
#define GLFW_INCLUDE_GLCOREARB 1
extern "C" {
#include "GLFW/glfw3.h"
}

constexpr float PI =  3.14159265f;

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

struct __attribute__((packed)) MyMatrix {
    GLfloat buf[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};

    MyMatrix() = default;

    void print()
    {
        printf("[ %3.2f %3.2f %3.2f %3.2f ]\n"
               "[ %3.2f %3.2f %3.2f %3.2f ]\n"
               "[ %3.2f %3.2f %3.2f %3.2f ]\n"
               "[ %3.2f %3.2f %3.2f %3.2f ]\n",
               get(0, 0), get(0, 1), get(0, 2), get(0, 3),
               get(1, 0), get(1, 1), get(1, 2), get(1, 3),
               get(2, 0), get(2, 1), get(2, 2), get(2, 3),
               get(3, 0), get(3, 1), get(3, 2), get(3, 3));
    }

    // column major ordering ... confusing
    float get(unsigned char row, unsigned char col) const {
        return buf[4*col + row];
    }
    void set(unsigned char row, unsigned char col, const float f) {
        buf[4*col + row] = f;
    }
    void reset() {
        MyMatrix n;
        memcpy(buf,n.buf, sizeof(buf));
    }
    MyMatrix& rotateX(const double angleInRad) {
        set(1, 1, cosf(angleInRad));
        set(1, 2, -sinf(angleInRad));
        set(2, 2, get(1, 1));
        set(2, 1, -get(1, 2));
        return *this;
    }

    MyMatrix& rotateZ(const double angleInRad) {
        set(0, 0, cosf(angleInRad));
        set(0, 1, -sinf(angleInRad));
        set(1, 1, get(0, 0));
        set(1, 0, -get(0, 1));
        return *this;
    }
    MyMatrix& rotateY(const double angleInRad) {
        set(0, 0, cosf(angleInRad));
        set(0, 2, sinf(angleInRad));
        set(2, 0, -get(0, 2));
        set(2, 2, get(0, 0));
        return *this;
    }
    MyMatrix operator*(const MyMatrix rhs) const {
        MyMatrix ret;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                ret.set(i, j, 0.0f);
                for (int k = 0; k < 4; ++k) {
                    ret.set(i, j, ret.get(i,j) + get(i, k) * rhs.get(k, j));
                }
            }
        }
        return ret;
    }
    MyMatrix transpose() const {
        MyMatrix ret;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                ret.set(i, j, get(j, i));
            }
        }
        return ret;
    }
};

struct __attribute__((packed)) MyPoint {
    GLfloat x,y,z;

    MyPoint() : x(0.0), y(0.0), z(0.0) {}
    constexpr MyPoint(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    MyPoint operator+(const MyPoint &rhs) const {
        MyPoint ret = *this;
        ret.x += rhs.x;
        ret.y += rhs.y;
        ret.z += rhs.z;

        return ret;
    }
    MyPoint operator-(const MyPoint &rhs) const {
        MyPoint ret = *this;
        ret.x -= rhs.x;
        ret.y -= rhs.y;
        ret.z -= rhs.z;

        return ret;
    }
    MyPoint& operator+=(const MyPoint &rhs) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;

        return *this;
    }
    void print() const {
        printf("%.3f %.3f %.3f\n", x, y, z);
    }

    MyPoint transform(const MyMatrix& m) const {
        MyPoint ret;
        ret.x = x*m.get(0,0)+y*m.get(0, 1)+z*m.get(0,2)+m.get(0,3);
        ret.y = x*m.get(1,0)+y*m.get(1, 1)+z*m.get(1,2)+m.get(1,3);
        ret.z = x*m.get(2,0)+y*m.get(2, 1)+z*m.get(2,2)+m.get(2,3);
        //print();
        //ret.print();
        return ret;
    }
    GLfloat dot(const MyPoint& rhs) const {
        return x*rhs.x+y*rhs.y+z*rhs.z;
    }
    MyPoint cross(const MyPoint& rhs) const {
        MyPoint ret;
        ret.x = y*rhs.z - z * rhs.y;
        ret.y = z*rhs.x - x * rhs.z;
        ret.z = x*rhs.y - y * rhs.x;
        return ret;
    }

    float length() const {
        return sqrt(x*x+y*y+z*z);
    }

    void normalize() {
        float l = length();
        if (l != 0.0) {
            x /= l;
            y /= l;
            z /= l;
        }
    }
};

struct MyCube {
    MyPoint vertices[36];

    enum {
        FRONT = 0,
        RIGHT = 1,
        LEFT = 2,
        BACK = 3,
        BOTTOM = 4,
        TOP = 5
    };

    void addZ(GLfloat f)
    {
        for (int i = 0; i < 36; ++i) {
            vertices[i].z += f;
        }
    }

    void setFace(int face, MyPoint p)
    {
        for (int i = 0; i < 6; ++i) {
            vertices[face * 6 + i] = p;
        }
    }

    void addToFace(int face, MyPoint p)
    {
        for (int i = 0; i < 6; ++i) {
            vertices[face * 6 + i] += p;
        }
    }

    void set(const MyPoint& center, GLfloat radius)
    {
        const GLfloat half = radius / 2.0f;
        {
            // front face low triangle
            vertices[0].x = center.x - half;
            vertices[0].y = center.y - half;
            vertices[0].z = center.z + half;
            vertices[1] = vertices[0] + MyPoint(radius, 0.0, 0.0);
            vertices[2] = vertices[0] + MyPoint(radius, radius, 0.0);

            vertices[3] = vertices[0];
            vertices[4] = vertices[0] + MyPoint(radius, radius, 0.0);
            vertices[5] = vertices[0] + MyPoint(0.0, radius, 0.0);
        }
        {
            // right face
            vertices[6].x = center.x + half;
            vertices[6].y = center.y - half;
            vertices[6].z = center.z + half;
            vertices[7] = vertices[6] + MyPoint(0.0, 0.0, -radius);
            vertices[8] = vertices[6] + MyPoint(0.0, radius, -radius);

            vertices[9] = vertices[6];
            vertices[10] = vertices[6] + MyPoint(0.0, radius, -radius);
            vertices[11] = vertices[6] + MyPoint(0.0, radius, 0.0);
        }
        {
            // left face
            vertices[12].x = center.x - half;
            vertices[12].y = center.y - half;
            vertices[12].z = center.z - half;
            vertices[13] = vertices[12] + MyPoint(0.0, 0.0, radius);
            vertices[14] = vertices[12] + MyPoint(0.0, radius, radius);

            vertices[15] = vertices[12];
            vertices[16] = vertices[12] + MyPoint(0.0, radius, radius);
            vertices[17] = vertices[12] + MyPoint(0.0, radius, 0.0);
        }
        {
            // back face
            vertices[18].x = center.x + half;
            vertices[18].y = center.y - half;
            vertices[18].z = center.z - half;
            vertices[19] = vertices[18] + MyPoint(-radius, 0.0, 0.0);
            vertices[20] = vertices[18] + MyPoint(-radius, radius, 0.0);

            vertices[21] = vertices[18];
            vertices[22] = vertices[18] + MyPoint(-radius, radius, 0.0);
            vertices[23] = vertices[18] + MyPoint(0.0, radius, 0.0);
        }
        {
            // bottom face
            vertices[24].x = center.x - half;
            vertices[24].y = center.y - half;
            vertices[24].z = center.z - half;
            vertices[25] = vertices[24] + MyPoint(radius, 0.0, 0.0);
            vertices[26] = vertices[24] + MyPoint(radius, 0.0, radius);

            vertices[27] = vertices[24];
            vertices[28] = vertices[24] + MyPoint(radius, 0.0, radius);
            vertices[29] = vertices[24] + MyPoint(0.0, 0.0, radius);
        }
        {
            // top face
            vertices[30].x = center.x - half;
            vertices[30].y = center.y + half;
            vertices[30].z = center.z + half;
            vertices[31] = vertices[30] + MyPoint(radius, 0.0, 0.0);
            vertices[32] = vertices[30] + MyPoint(radius, 0.0, -radius);

            vertices[33] = vertices[30];
            vertices[34] = vertices[30] + MyPoint(radius, 0.0, -radius);
            vertices[35] = vertices[30] + MyPoint(0.0, 0.0, -radius);
        }
    }

    void transform(const MyMatrix& m) {
        for (int i = 0; i < 36; ++i) {
            vertices[i] = vertices[i].transform(m);
        }
    }

};

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

struct MyRubik {
    MyCube cubes[27];
    MyCube colors[27];
    MyCube normals[27];
    MyMatrix transforms[28];
    int pos[27];
    MyPoint faceNormal[6];

    constexpr static MyPoint red{186.0f/255.0f, 12.0f/255.0f, 47.0f/255.0f};
    constexpr static MyPoint green{0.0f/255.0f, 154.0f/255.0f, 68.0f/255.0f};
    constexpr static MyPoint blue{0.0f/255.0f, 61.0f/255.0f, 165.0f/255.0f};
    constexpr static MyPoint orange{254.0f/255.0f, 80.0f/255.0f, 0.0f/255.0f};
    constexpr static MyPoint yellow{255.0f/255.0f, 215.0f/255.0f, 0.0f/255.0f};
    constexpr static MyPoint white{1.0f, 1.0f, 1.0f};
    constexpr static MyPoint inside//{233.0f/255.0f,84.0f/255.0f,133.0f/255.0f};
        {45.0f/255.0f,45.0f/255.0f,45.0f/255.0f};//{0.0f, 0.0f, 0.0f};

    MyRubik() {}

    constexpr static int srcIndices[6][9] = {
        {  0,  1,  2,  3,  4,  5,  6,  7, 8  } // front
       ,{  2, 11, 20,  5, 14, 23,  8, 17, 26 } // right
       ,{  0,  3,  6,  9, 12, 15, 18, 21, 24 } // left
       ,{ 18, 19, 20, 21, 22, 23, 24, 25, 26 } // back
       ,{  0,  1,  2,  9, 10, 11, 18, 19, 20 } // down
       ,{  6,  7,  8, 15, 16, 17, 24, 25, 26 } // up
    };

    constexpr static int dstIndices[6][9] = {
        {  6,  3,  0,  7,  4,  1,  8,  5,  2 } // front
       ,{  8,  5,  2, 17, 14, 11, 26, 23, 20 } // right
       ,{ 18,  9,  0, 21, 12,  3, 24, 15,  6 } // left
       ,{ 20, 23, 26, 19, 22, 25, 18, 21, 24 } // back
       ,{  2, 11, 20,  1, 10, 19,  0,  9, 18 } // down
       ,{ 24, 15,  6, 25, 16,  7, 26, 17,  8 } // up
    };

    void endRot(int type, bool inv = false)
    {
        if (inv) {
            // Inverse rotation is equivalent to 3 regular rots.
            endRot(type);
            endRot(type);
        }
        int tmpBuf[9];
        for (int i = 0; i < 9; ++i) {
            tmpBuf[i] = pos[srcIndices[type][i]];
        }

        for (int i = 0; i < 9; ++i) {
            pos[dstIndices[type][i]] = tmpBuf[i];
        }
    }

    void doIncRot(int type, const MyMatrix& m ) {
        for (int i = 0; i < 9; ++i) {
            transforms[pos[srcIndices[type][i]]]
                        = m * transforms[pos[srcIndices[type][i]]];
        }
    }

    void set() {
        faceNormal[0].z = 1;
        faceNormal[1].x = 1;
        faceNormal[2].x = -1;
        faceNormal[3].z = -1;
        faceNormal[4].y = -1;
        faceNormal[5].y = 1;

        constexpr float cubeRadius = 0.39f;

        for (int i = 0; i < 27; ++i) {
            for (int j = 0; j < 36; ++j) {
                colors[i].vertices[j] = inside;
            }
        }

        cubes[0].set(MyPoint(-0.4f, -0.4f, 0.4f), cubeRadius);
        colors[0].setFace(MyCube::LEFT, blue);
        colors[0].setFace(MyCube::FRONT, red);
        colors[0].setFace(MyCube::BOTTOM, white);

        cubes[1].set(MyPoint(0.0f, -0.4f, 0.4f), cubeRadius);
        colors[1].setFace(MyCube::FRONT, red);
        colors[1].setFace(MyCube::BOTTOM, white);

        cubes[2].set(MyPoint(0.4f, -0.4f, 0.4f), cubeRadius);
        colors[2].setFace(MyCube::FRONT, red);
        colors[2].setFace(MyCube::BOTTOM, white);
        colors[2].setFace(MyCube::RIGHT, green);

        cubes[3].set(MyPoint(-0.4f, 0.0f, 0.4f), cubeRadius);
        colors[3].setFace(MyCube::LEFT, blue);
        colors[3].setFace(MyCube::FRONT, red);

        cubes[4].set(MyPoint(0.0f, 0.0f, 0.4f), cubeRadius);
        colors[4].setFace(MyCube::FRONT, red);

        cubes[5].set(MyPoint(0.4f, 0.0f, 0.4f), cubeRadius);
        colors[5].setFace(MyCube::FRONT, red);
        colors[5].setFace(MyCube::RIGHT, green);

        cubes[6].set(MyPoint(-0.4f, 0.4f, 0.4f), cubeRadius);
        colors[6].setFace(MyCube::LEFT, blue);
        colors[6].setFace(MyCube::FRONT, red);
        colors[6].setFace(MyCube::TOP, yellow);

        cubes[7].set(MyPoint(0.0f, 0.4f, 0.4f), cubeRadius);
        colors[7].setFace(MyCube::FRONT, red);
        colors[7].setFace(MyCube::TOP, yellow);

        cubes[8].set(MyPoint(0.4f, 0.4f, 0.4f), cubeRadius);
        colors[8].setFace(MyCube::FRONT, red);
        colors[8].setFace(MyCube::RIGHT, green);
        colors[8].setFace(MyCube::TOP, yellow);

        for (int i = 0; i < 9; ++i) {
            cubes[i+9] = cubes[i];
            cubes[i+9].addZ(-0.4f);
            colors[i+9] = colors[i];
            colors[i+9].setFace(MyCube::FRONT, inside);
        }

        for (int i = 0; i < 9; ++i) {
            cubes[i+18] = cubes[i];
            cubes[i+18].addZ(-0.4f*2);
            colors[i+18] = colors[i];
            colors[i+18].setFace(MyCube::FRONT, inside);
            colors[i+18].setFace(MyCube::BACK, orange);
        }

        for (int i = 0; i < 27; ++i) {
            normals[i].setFace(MyCube::FRONT, MyPoint(0.0f, 0.0f, 1.0f));
            normals[i].setFace(MyCube::BACK, MyPoint(0.0f, 0.0f, -1.0f));
            normals[i].setFace(MyCube::TOP, MyPoint(0.0f, 1.0f, 0.0f));
            normals[i].setFace(MyCube::BOTTOM, MyPoint(0.0f, -1.0f, 0.0f));
            normals[i].setFace(MyCube::LEFT, MyPoint(-1.0f, 0.0f, 0.0f));
            normals[i].setFace(MyCube::RIGHT, MyPoint(1.0f, 0.0f, 0.0f));
        }
        for (int i = 0; i < 27; ++i) {
            pos[i] = i;
        }
    }
};

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

        window = glfwCreateWindow(800, 600, "CubeSolver",
                                  NULL, nullptr);
        if (!window) {
            puts("window creation failed");
            return;
        }

        glfwMakeContextCurrent(window);
        glfwSetWindowSizeCallback(window, glfw_onResize);
        glfwSetKeyCallback(window, glfw_onKey);
        glfwSetCursorPosCallback(window, glfw_onMouseMove);

        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

        startup();

        do
        {
            render(glfwGetTime());

            glfwPollEvents();
            glfwSwapBuffers(window);

            running &= (glfwGetKey(window, GLFW_KEY_ESCAPE ) == GLFW_RELEASE);
            running &= !glfwWindowShouldClose(window);
        } while(running);

        shutdown();

        glfwTerminate();
    }
    static MyApp *app;
    static void glfw_onResize(GLFWwindow *window, int w, int h)
    {
        app->onResize(w, h);
    }

    static void glfw_onKey(GLFWwindow *window, int key, int scancode,
                           int action, int mods)
    {
        app->onKey(key, action);
    }

    static void glfw_onMouseMove(GLFWwindow *window, double x, double y)
    {
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

        rubik.set();

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
        constexpr float groundYBase = -1.0f;
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
                           (const GLfloat *) rubik.transforms);

        glUseProgram(shadowProgram);
        glUniformMatrix4fv(shadowVertexTransformLoc, 28, GL_FALSE,
                           (const GLfloat *) rubik.transforms);
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
                          float n, float f)
    {
        MyMatrix result;
        result.set(0, 0, 2.0f / (right - left));
        result.set(1, 1, 2.0f / (top - bottom));
        result.set(2, 2, 2.0f / (n - f));
        result.set(0, 3, (left + right) / (left - right));
        result.set(1, 3, (bottom + top) / (bottom - top));
        result.set(2, 3, (n + f) / (f - n));
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

    static constexpr int initxRot = 0;
    static constexpr int inityRot = 45;
    int xRot;
    int yRot;
    int oldxRot;
    int oldyRot;

    bool inRot = false;
    bool inViewRot = false;
    double rotStartTime;
    double rotLastFrame;
    struct RotType {
        int rotType = -1;
        bool inverse = false;
    };
    array<RotType, 4> queueRotType;
    RotType curRot;

    void startRot(RotType r)
    {
        if (inRot == true || inViewRot) {
            for (int i = 0; i < 4; ++i) {
                if (queueRotType[i].rotType == -1) {
                    queueRotType[i] = r;
                    return;
                }
            }
            return;
        }
        curRot = r;
        inRot = true;
        rotStartTime = glfwGetTime();
        rotLastFrame = rotStartTime;
    }

    int keyPress = -1;

    void processMoveKey(int key)
    {
        if (key == GLFW_KEY_SPACE) {
            resetState();
            return ;
        }

        oldxRot = xRot;
        oldyRot = yRot;
        rotStartTime = glfwGetTime();
        rotLastFrame = rotStartTime;
        inViewRot = true;

        constexpr int inc = 15;
        switch (key) {
          case GLFW_KEY_UP: xRot -= inc; break;
          case GLFW_KEY_DOWN: xRot += inc; break;
          case GLFW_KEY_LEFT: yRot -= inc; break;
          case GLFW_KEY_RIGHT: yRot += inc; break;
        }
        if (xRot > 360) {
            xRot -= 360;
            oldxRot -= 360;
        }
        else if (xRot < 360) {
            xRot += 360;
            oldxRot += 360;
        }
        if (yRot > 360) {
            yRot -= 360;
            oldyRot -= 360;
        }
        else if (yRot < 360) {
            yRot += 360;
            oldyRot += 360;
        }
    }

    void updateCamera()
    {
        cameraTransform = lookAt(eye, eyeDir, MyPoint(0, 1, 0));
    }

    MyPoint eye;
    MyPoint eyeDir;
    void resetState()
    {
        yRot = inityRot;
        xRot = initxRot;

        eye.x = 0.0f;
        eye.y = 0.0f;
        eye.z = 5.0f;

        eyeDir = MyPoint();

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
            if (inRot) {
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
        if (action != GLFW_PRESS) {
            return;
        }

        if (key == GLFW_KEY_S
         || key == GLFW_KEY_X
         || key == GLFW_KEY_Z
         || key == GLFW_KEY_C) {
            constexpr float adjust = 0.1f;
            switch (key) {
              case GLFW_KEY_C: eye.x += adjust; eyeDir.x += adjust; break;
              case GLFW_KEY_Z: eye.x -= adjust; eyeDir.x -= adjust; break;
              case GLFW_KEY_X: eye.y -= adjust; eyeDir.y -= adjust; break;
              case GLFW_KEY_S: eye.y += adjust; eyeDir.y += adjust; break;
            }
            updateCamera();
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

        MyMatrix mv;
        mv.rotateX(xRot / 180.0f * PI - 0.1f);
        mv = mv * MyMatrix().rotateY(yRot / 180.0f * PI - 0.1f);

        float max;
        RotType r;
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

    static constexpr double totRotTime = 0.4;
    static constexpr double totRotTime2 = 0.2;

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

        if (inRot) {
            double totalDiff = currentTime - rotStartTime;
            double diff = currentTime - rotLastFrame;

            if (totalDiff > totRotTime) {
                diff -= (totalDiff - totRotTime);
            }

            diff /= totRotTime;

            MyMatrix t;
            float angle = PI/2.0f*float(diff);
            if (curRot.inverse) {
                angle *= -1.0f;
            }
            if (curRot.rotType == MyCube::TOP) {
                t.rotateY(-angle);
            }
            else if (curRot.rotType == MyCube::BOTTOM) {
                t.rotateY(angle);
            }
            else if (curRot.rotType == MyCube::FRONT) {
                t.rotateZ(-angle);
            }
            else if (curRot.rotType == MyCube::BACK) {
                t.rotateZ(angle);
            }
            else if (curRot.rotType == MyCube::RIGHT) {
                t.rotateX(-angle);
            }
            else if (curRot.rotType == MyCube::LEFT) {
                t.rotateX(angle);
            }
            rubik.doIncRot(curRot.rotType, t);
            glUseProgram(program);
            glCall(glUniformMatrix4fv(vertexTransformLocation, 28, GL_FALSE,
                               (const GLfloat *) rubik.transforms));
            glUseProgram(shadowProgram);
            glCall(glUniformMatrix4fv(shadowVertexTransformLoc, 28, GL_FALSE,
                               (const GLfloat *) rubik.transforms));
            if (totalDiff >= totRotTime) {
                inRot = false;
                //puts("end rot");
                rubik.endRot((int) curRot.rotType, curRot.inverse);
                if (queueRotType[0].rotType != -1) {
                    RotType next = queueRotType[0];
                    queueRotType[0] = queueRotType[1];
                    queueRotType[1] = queueRotType[2];
                    queueRotType[2] = queueRotType[3];
                    queueRotType[3].rotType = -1;
                    startRot(next);
                }
            }
            rotLastFrame = currentTime;
        }
        MyMatrix cubeRot;
        if (inViewRot) {
            double totalDiff = currentTime - rotStartTime;
            double diff = currentTime - rotLastFrame;

            if (totalDiff > totRotTime2) {
                diff -= (totalDiff - totRotTime2);
            }

            diff /= totRotTime2;

            MyMatrix rx,ry;
            rx.rotateX((xRot * diff + oldxRot*(1.0f - diff)) / 180.0f * PI);
            ry.rotateY((yRot * diff + oldyRot*(1.0f - diff)) / 180.0f * PI);
            cubeRot = rx * ry;
            if (totalDiff >= totRotTime2) {
                inViewRot = false;
                if (queueRotType[0].rotType != -1) {
                    RotType next = queueRotType[0];
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
        }
        else {
            MyMatrix rx,ry;
            rx.rotateX(xRot / 180.0f * PI);
            ry.rotateY(yRot / 180.0f * PI);
            cubeRot = rx * ry;
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

        MyPoint lightInvDir(1.0f,3.0f,1.0f);
        MyMatrix p = ortho(-5, 5, -5, 5, -10, 0);
        MyMatrix l = lookAt(lightInvDir, MyPoint(), MyPoint(0,1,0));

        glUniformMatrix4fv(shadowPerspectiveLoc, 1, GL_FALSE, p.buf);
        MyMatrix tmp = l * cubeRot;
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
        MyMatrix cubeMv = fullMv * cubeRot;
        tmp = l * cubeRot;
        tmp = p * tmp;
        glUniformMatrix4fv(shadowMvpLoc, 1, GL_FALSE, tmp.buf);
        glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, cubeMv.buf);
        MyPoint lightPos(2.0f, 1.0f, 0.0f);
        lightPos = lightPos.transform(fullMv);
        glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);

        glDrawArrays(GL_TRIANGLES, 0, 36*27);

        // To debug the shadow
#if 0
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
    MyApp::app = &app;
    app.run();
}

MyApp *MyApp::app;
constexpr MyPoint MyRubik::red;
constexpr MyPoint MyRubik::green;
constexpr MyPoint MyRubik::blue;
constexpr MyPoint MyRubik::yellow;
constexpr MyPoint MyRubik::orange;
constexpr MyPoint MyRubik::white;
constexpr MyPoint MyRubik::inside;
constexpr int MyRubik::srcIndices[6][9];
constexpr int MyRubik::dstIndices[6][9];
