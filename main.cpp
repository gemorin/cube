#include <sb6.h>
#include <vmath.h>

#include <cmath>
#include <array>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

constexpr float PI =  3.14159265f;

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
    void rotateX(const double angleInRad) {
        set(1, 1, cosf(angleInRad));
        set(1, 2, -sinf(angleInRad));
        set(2, 2, get(1, 1));
        set(2, 1, -get(1, 2));
    }

    void rotateZ(const double angleInRad) {
        set(0, 0, cosf(angleInRad));
        set(0, 1, -sinf(angleInRad));
        set(1, 1, get(0, 0));
        set(1, 0, -get(0, 1));
    }
    void rotateY(const double angleInRad) {
        set(0, 0, cosf(angleInRad));
        set(0, 2, sinf(angleInRad));
        set(2, 0, -get(0, 2));
        set(2, 2, get(0, 0));
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
    MyMatrix tranpose() const {
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

struct MyRubik {
    MyCube cubes[27];
    MyCube colors[27];
    MyCube normals[27];
    MyMatrix transforms[27];
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

        float r = 0.39f;

        for (int i = 0; i < 27; ++i) {
            for (int j = 0; j < 36; ++j) {
                colors[i].vertices[j] = inside;
            }
        }

        cubes[0].set(MyPoint(-0.4f, -0.4f, 0.4f), r);
        colors[0].setFace(MyCube::LEFT, blue);
        colors[0].setFace(MyCube::FRONT, red);
        colors[0].setFace(MyCube::BOTTOM, white);

        cubes[1].set(MyPoint(0.0f, -0.4f, 0.4f), r);
        colors[1].setFace(MyCube::FRONT, red);
        colors[1].setFace(MyCube::BOTTOM, white);

        cubes[2].set(MyPoint(0.4f, -0.4f, 0.4f), r);
        colors[2].setFace(MyCube::FRONT, red);
        colors[2].setFace(MyCube::BOTTOM, white);
        colors[2].setFace(MyCube::RIGHT, green);

        cubes[3].set(MyPoint(-0.4f, 0.0f, 0.4f), r);
        colors[3].setFace(MyCube::LEFT, blue);
        colors[3].setFace(MyCube::FRONT, red);

        cubes[4].set(MyPoint(0.0f, 0.0f, 0.4f), r);
        colors[4].setFace(MyCube::FRONT, red);

        cubes[5].set(MyPoint(0.4f, 0.0f, 0.4f), r);
        colors[5].setFace(MyCube::FRONT, red);
        colors[5].setFace(MyCube::RIGHT, green);


        cubes[6].set(MyPoint(-0.4f, 0.4f, 0.4f), r);
        colors[6].setFace(MyCube::LEFT, blue);
        colors[6].setFace(MyCube::FRONT, red);
        colors[6].setFace(MyCube::TOP, yellow);

        cubes[7].set(MyPoint(0.0f, 0.4f, 0.4f), r);
        colors[7].setFace(MyCube::FRONT, red);
        colors[7].setFace(MyCube::TOP, yellow);

        cubes[8].set(MyPoint(0.4f, 0.4f, 0.4f), r);
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

struct MyApp : public sb6::application
{
  public:
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
            puts("glCreateShader failed");
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
    GLuint projMatrixLocation = -1;
    GLuint mvMatrixLocation;
    GLuint vertexTransformLocation;
    GLuint passThroughShader;
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

        projMatrixLocation = glGetUniformLocation(program, "projMatrix");
        mvMatrixLocation = glGetUniformLocation(program, "mvMatrix");
        vertexTransformLocation = glGetUniformLocation(program, "vTransform");
        passThroughShader = glGetUniformLocation(program, "passThroughShader");

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            printf("link program failed: %s\n", getProgramLog(program).c_str());
            return false;
        }
        //glPointSize(5.0f);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glUseProgram(program);

        return true;
    }

    GLuint vao;
    GLuint buffer;
    GLuint bufferColor;
    GLuint normals;

    MyRubik rubik;

    void init()
    {
        sb6::application::init();
        info.samples = 4;
        strcpy(info.title, "CubeSolver");
    }

    GLuint groundVao;
    GLuint groundVecBuf;
    GLuint groundColorBuf;
    GLuint groundNormalBuf;

    MyPoint groundVec[6];
    MyPoint groundColor[6];
    MyPoint groundNormal[6];

    void startup()
    {
        if (!compileShaders())
            exit(1);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        rubik.set();

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rubik.cubes), rubik.cubes,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &bufferColor);
        glBindBuffer(GL_ARRAY_BUFFER, bufferColor);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rubik.colors), rubik.colors,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &normals);
        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glBufferData(GL_ARRAY_BUFFER, sizeof(rubik.normals),
                     rubik.normals,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);

        constexpr float groundBase = 20.f;
        constexpr float groundYBase = -1.2f;
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

        glGenVertexArrays(1, &groundVao);
        glBindVertexArray(groundVao);

        glGenBuffers(1, &groundVecBuf);
        glBindBuffer(GL_ARRAY_BUFFER, groundVecBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(groundVec), groundVec,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        glGenBuffers(1, &groundColorBuf);
        glBindBuffer(GL_ARRAY_BUFFER, groundColorBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(groundColor), groundColor,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);

        glGenBuffers(1, &groundNormalBuf);
        glBindBuffer(GL_ARRAY_BUFFER, groundNormalBuf);
        glBufferData(GL_ARRAY_BUFFER, sizeof(groundNormal), groundNormal,
                     GL_STATIC_DRAW);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);

        glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, projMatrix);
        glUniformMatrix4fv(vertexTransformLocation, 27, GL_FALSE,
                           (const GLfloat *) rubik.transforms);
    }

    void shutdown()
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(program);
    }

    vmath::mat4     projMatrix;
    void onResize(int w, int h)
    {
        sb6::application::onResize(w, h);
        float aspect = (float) info.windowWidth / (float)info.windowHeight;
        projMatrix = vmath::perspective(50.0f, aspect, 0.1f, 1000.0f);
        if (projMatrixLocation != -1) {
            glUniformMatrix4fv(projMatrixLocation, 1, GL_FALSE, projMatrix);
        }
    }

    MyMatrix mv;


    static constexpr int initxRot = 0;
    static constexpr int inityRot = 45;
    int xRot = initxRot;
    int yRot = inityRot;
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
        oldxRot = xRot;
        oldyRot = yRot;
        rotStartTime = glfwGetTime();
        rotLastFrame = rotStartTime;
        if (key != GLFW_KEY_SPACE) {
            inViewRot = true;
        }

        constexpr int inc = 15;
        switch (key) {
          case GLFW_KEY_UP: xRot -= inc; break;
          case GLFW_KEY_DOWN: xRot += inc; break;
          case GLFW_KEY_LEFT: yRot -= inc; break;
          case GLFW_KEY_RIGHT: yRot += inc; break;
          case GLFW_KEY_SPACE: yRot = inityRot; xRot = initxRot; break;
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

    void onKey(int key, int action)
    {
        static bool shiftOn = false;
        if (key == GLFW_KEY_LSHIFT
         || key == GLFW_KEY_RSHIFT) {
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
            else if (action !=  GLFW_PRESS) {
                return;
            }
            processMoveKey(key);
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

        //MyMatrix rx,ry;
        //rx.rotateX(-xRot / 180.0f * PI);
        //ry.rotateY(-yRot / 180.0f * PI);
        //MyMatrix mv = ry * rx;
        MyMatrix rx,ry;
        rx.rotateX(xRot / 180.0f * PI - 0.1f);
        ry.rotateY(yRot / 180.0f * PI - 0.1f);
        MyMatrix mv = rx * ry;
        //mv = mv.tranpose();
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
    void render(double currentTime)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        constexpr GLfloat background[] = { 210.0f/255.0f, 230.0f/255.0f, 255.0f/255.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, background);


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
            glUniformMatrix4fv(vertexTransformLocation, 27, GL_FALSE,
                               (const GLfloat *) rubik.transforms);
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
            mv = rx * ry;
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
            mv = rx * ry;
        }
        glUniform1i(passThroughShader, 1);
        glBindVertexArray(groundVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glUniform1i(passThroughShader, 0);
        mv.set(2, 3, -5.0f);
        glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, mv.buf);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36*27);

        if (frames == 100) {
            frames = 0;
            printf("fps %f\n", 100.0 / (currentTime - start));
        }
    }
};

//constexpr GLfloat MyApp::vertices[];
//constexpr GLfloat MyApp::colors[];
constexpr MyPoint MyRubik::red;
constexpr MyPoint MyRubik::green;
constexpr MyPoint MyRubik::blue;
constexpr MyPoint MyRubik::yellow;
constexpr MyPoint MyRubik::orange;
constexpr MyPoint MyRubik::white;
constexpr MyPoint MyRubik::inside;
constexpr int MyRubik::srcIndices[6][9];
constexpr int MyRubik::dstIndices[6][9];
DECLARE_MAIN(MyApp);
