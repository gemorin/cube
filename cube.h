#pragma once

#include <cmath>

struct __attribute__((packed)) MyMatrix {
    // column major layout
    float buf[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};

    MyMatrix() = default;

    void print();
    float get(unsigned char row, unsigned char col) const;
    void set(unsigned char row, unsigned char col, const float f);
    void reset();
    MyMatrix& rotateX(const double angleInRad);
    MyMatrix& rotateZ(const double angleInRad);
    MyMatrix& rotateY(const double angleInRad);
    MyMatrix operator*(const MyMatrix rhs) const;
    MyMatrix transpose() const;
};

struct __attribute__((packed)) MyPoint {
    float x,y,z;

    MyPoint() : x(0.0), y(0.0), z(0.0) {}
    constexpr MyPoint(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    MyPoint operator+(const MyPoint &rhs) const;
    MyPoint operator-(const MyPoint &rhs) const;
    MyPoint& operator+=(const MyPoint &rhs);
    MyPoint operator*(float m) const;
    MyPoint opposite() const;
    void print() const;
    MyPoint transform(const MyMatrix& m) const;
    float dot(const MyPoint& rhs) const;
    MyPoint cross(const MyPoint& rhs) const;
    float length() const;
    void normalize();
};

struct MyQuaternion {
    float w = 1.0;
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;

    MyQuaternion() = default;
    MyQuaternion(float _w, float _x, float _y, float _z)
        : w(_w), x(_x), y(_y), z(_z) {}

    void setRotation(float angle, MyPoint axis);
    void rotateX(float angle);
    void rotateY(float angle);
    void rotateZ(float angle);

    float magnitude() const;
    void toOppositeAxis();

    void normalize();

    MyQuaternion operator*(float mult) const;
    MyQuaternion& operator*=(float mult);
    MyQuaternion operator-(const MyQuaternion& rhs) const;
    MyQuaternion operator+(const MyQuaternion& rhs) const;
    MyQuaternion operator*(const MyQuaternion& rhs) const;

    float dot(const MyQuaternion& rhs) const;

    MyMatrix toMatrix() const;

    static MyQuaternion slerp(const MyQuaternion& start, MyQuaternion end,
                              float t);
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

    void addZ(float f);
    void setFace(int face, MyPoint p);
    void addToFace(int face, MyPoint p);

    void set(const MyPoint& center, float radius);
    void transform(const MyMatrix& m);
};

struct MyRubik {
    MyCube cubes[27];
    MyCube colors[27];
    MyCube normals[27];
    MyQuaternion qTransforms[27];
    MyMatrix mTransforms[27];
    int pos[27];
    MyPoint faceNormal[6];
    MyQuaternion faceRotationEnd[9];

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

    MyQuaternion rotTypeToQuat(int type);

    void startRot(int type, bool inv);
    void doIncRot(int type, float t);
    void endRot(int type, bool inv = false);

    constexpr float radius() const { return 0.40f; }

    void initialize();
};

inline
float MyMatrix::get(unsigned char row, unsigned char col) const
{
    return buf[4*col + row];
}

inline
void MyMatrix::set(unsigned char row, unsigned char col, const float f)
{
    buf[4*col + row] = f;
}

inline
MyPoint MyPoint::operator+(const MyPoint &rhs) const
{
    MyPoint ret = *this;
    ret.x += rhs.x;
    ret.y += rhs.y;
    ret.z += rhs.z;

    return ret;
}

inline
MyPoint MyPoint::operator-(const MyPoint &rhs) const
{
    MyPoint ret = *this;
    ret.x -= rhs.x;
    ret.y -= rhs.y;
    ret.z -= rhs.z;

    return ret;
}

inline
MyPoint& MyPoint::operator+=(const MyPoint &rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;

    return *this;
}

inline
MyPoint MyPoint::operator*(float m) const
{
    return MyPoint(x * m, y * m, z * m);
}

inline
MyPoint MyPoint::opposite() const
{
    return MyPoint(-x, -y, -z);
}

inline
MyPoint MyPoint::cross(const MyPoint& rhs) const
{
    MyPoint ret;
    ret.x = y*rhs.z - z * rhs.y;
    ret.y = z*rhs.x - x * rhs.z;
    ret.z = x*rhs.y - y * rhs.x;
    return ret;
}

inline
float MyPoint::dot(const MyPoint& rhs) const
{
    return x*rhs.x+y*rhs.y+z*rhs.z;
}

inline
float MyPoint::length() const
{
    return sqrtf(x*x+y*y+z*z);
}

inline
void MyPoint::normalize()
{
    float l = length();
    if (l != 0.0) {
        x /= l;
        y /= l;
        z /= l;
    }
}

inline
void MyQuaternion::rotateX(float angle)
{
    setRotation(angle, MyPoint{1.0f, 0.0f, 0.0f});
}

inline
void MyQuaternion::rotateY(float angle)
{
    MyQuaternion::setRotation(angle, MyPoint{0.0f, 1.0f, 0.0f});
}

inline
void MyQuaternion::rotateZ(float angle)
{
    MyQuaternion::setRotation(angle, MyPoint{0.0f, 0.0f, 1.0f});
}

inline
float MyQuaternion::magnitude() const
{
    return sqrtf(w*w + x*x + y*y + z*z);
}

inline
void MyQuaternion::toOppositeAxis()
{
    x = -x;
    y = -y;
    z = -z;
}

inline
void MyQuaternion::normalize()
{
    const float l = magnitude();
    w /= l;
    x /= l;
    y /= l;
    z /= l;
}

inline
MyQuaternion MyQuaternion::operator*(float mult) const
{
    return MyQuaternion{w*mult, x*mult, y*mult, z*mult};
}

inline
MyQuaternion& MyQuaternion::operator*=(float mult)
{
    w *= mult;
    x *= mult;
    y *= mult;
    z *= mult;
    return *this;
}

inline
MyQuaternion MyQuaternion::operator-(const MyQuaternion& rhs) const
{
    return MyQuaternion{w-rhs.w,x-rhs.x,y-rhs.y,z-rhs.z};
}

inline
MyQuaternion MyQuaternion::operator+(const MyQuaternion& rhs) const
{
    return MyQuaternion{w+rhs.w,x+rhs.x,y+rhs.y,z+rhs.z};
}

inline
MyQuaternion MyQuaternion::operator*(const MyQuaternion& rhs) const
{
    return MyQuaternion{
        -x * rhs.x - y * rhs.y - z * rhs.z + w * rhs.w,
        x * rhs.w + y * rhs.z - z * rhs.y + w * rhs.x,
        -x * rhs.z + y * rhs.w + z * rhs.x + w * rhs.y,
        x * rhs.y - y * rhs.x + z * rhs.w + w * rhs.z
    };
}

inline
float MyQuaternion::dot(const MyQuaternion& rhs) const
{
    return w * rhs.w + x * rhs.x + y * rhs.y + z * rhs.z;
}
