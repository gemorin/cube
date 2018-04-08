#include "cube.h"

#include <cstdio>
#include <cmath>
#include <cstring>

using namespace std;

void MyMatrix::print()
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

void MyMatrix::reset()
{
    MyMatrix n;
    memcpy(buf,n.buf, sizeof(buf));
}

MyMatrix& MyMatrix::rotateX(const double angleInRad)
{
    set(1, 1, cosf(angleInRad));
    set(1, 2, -sinf(angleInRad));
    set(2, 2, get(1, 1));
    set(2, 1, -get(1, 2));
    return *this;
}

MyMatrix& MyMatrix::rotateZ(const double angleInRad)
{
    set(0, 0, cosf(angleInRad));
    set(0, 1, -sinf(angleInRad));
    set(1, 1, get(0, 0));
    set(1, 0, -get(0, 1));
    return *this;
}

MyMatrix& MyMatrix::rotateY(const double angleInRad)
{
    set(0, 0, cosf(angleInRad));
    set(0, 2, sinf(angleInRad));
    set(2, 0, -get(0, 2));
    set(2, 2, get(0, 0));
    return *this;
}

MyMatrix MyMatrix::operator*(const MyMatrix rhs) const
{
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

MyMatrix MyMatrix::transpose() const
{
    MyMatrix ret;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            ret.set(i, j, get(j, i));
        }
    }
    return ret;
}

void MyPoint::print() const
{
    printf("%.3f %.3f %.3f\n", x, y, z);
}

MyPoint MyPoint::transform(const MyMatrix& m) const
{
    MyPoint ret;
    ret.x = x*m.get(0,0)+y*m.get(0, 1)+z*m.get(0,2)+m.get(0,3);
    ret.y = x*m.get(1,0)+y*m.get(1, 1)+z*m.get(1,2)+m.get(1,3);
    ret.z = x*m.get(2,0)+y*m.get(2, 1)+z*m.get(2,2)+m.get(2,3);
    //print();
    //ret.print();
    return ret;
}

MyPoint MyPoint::transform(const MyQuaternion& q) const
{
    MyQuaternion v(0.0f, x, y, z);
    MyQuaternion conj(q);
    conj.toOppositeAxis();
    v = q * v * conj;
    return MyPoint(v.x, v.y, v.z);
}

void MyQuaternion::setRotation(float angle, MyPoint axis)
{
    w = cosf(angle / 2);
    float s = sinf(angle / 2);
    axis.normalize();
    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
}


MyMatrix MyQuaternion::toMatrix() const
{
    MyQuaternion q = *this;
    q.normalize();

    MyMatrix ret;
    ret.set(0, 0, 1.0f - 2.0f * q.y * q.y - 2.0f * q.z * q.z);
    ret.set(0, 1, 2.0f * q.x * q.y - 2.0f * q.w * q.z);
    ret.set(0, 2, 2.0f * q.x * q.z + 2.0f * q.w * q.y);
    ret.set(1, 0, 2.0f * q.x * q.y + 2.0f * q.w * q.z);
    ret.set(1, 1, 1.0f - 2.0f * q.x * q.x - 2.0f * q.z * q.z);
    ret.set(1, 2, 2.0f * q.y * q.z - 2.0f * q.w * q.x);
    ret.set(2, 0, 2.0f * q.x * q.z - 2.0f * q.w * q.y);
    ret.set(2, 1, 2.0f * q.y * q.z + 2.0f * q.w * q.x);
    ret.set(2, 2, 1.0f - 2.0f * q.x * q.x - 2.0f * q.y * q.y);

    return ret;
}

MyQuaternion
MyQuaternion::slerp(const MyQuaternion& start, MyQuaternion end, float t)
{
    float dot = start.dot(end);
    if (dot < 0) {
        dot = fabs(dot);
        end *= -1.0f;
    }

    if (dot > 0.95) {
        MyQuaternion ret = start + (end - start) * t;
        ret.normalize();
        return ret;
    }
    const float finalAngle = acosf(dot);
    const float angle = finalAngle * t;
    const float fact0 = cosf(angle) - dot * sinf(angle) / sinf(finalAngle);
    const float fact1 = sinf(angle) / sinf(finalAngle);
    return start * fact0 + end * fact1;
}

void MyCube::addZ(float f)
{
    for (int i = 0; i < 36; ++i) {
        vertices[i].z += f;
    }
}

void MyCube::setFace(int face, MyPoint p)
{
    for (int i = 0; i < 6; ++i) {
        vertices[face * 6 + i] = p;
    }
}

void MyCube::set(const MyPoint& center, float radius)
{
    const float half = radius / 2.0f;
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

void MyCube::transform(const MyMatrix& m)
{
    for (int i = 0; i < 36; ++i) {
        vertices[i] = vertices[i].transform(m);
    }
}

MyQuaternion MyRubik::rotTypeToQuat(int type)
{
    MyQuaternion ret;
    switch (type) {
      case MyCube::TOP: ret.rotateY(-M_PI/2.0f); return ret;
      case MyCube::BOTTOM: ret.rotateY(M_PI/2.0f); return ret;
      case MyCube::FRONT: ret.rotateZ(-M_PI/2.0f); return ret;
      case MyCube::BACK: ret.rotateZ(M_PI/2.0f); return ret;
      case MyCube::RIGHT: ret.rotateX(-M_PI/2.0f); return ret;
      case MyCube::LEFT: ret.rotateX(M_PI/2.0f); return ret;
    }
    return ret;
}

void MyRubik::endRot(int type, bool inv)
{
    for (int i = 0; i < 9; ++i) {
        qTransforms[pos[srcIndices[type][i]]] = faceRotationEnd[i];
        mTransforms[pos[srcIndices[type][i]]] =
                                            faceRotationEnd[i].toMatrix();
    }
    int numIterations = inv ? 3 : 1;
    for (int iterations = 0 ; iterations < numIterations; ++iterations) {
        int tmpBuf[9];
        for (int i = 0; i < 9; ++i) {
            tmpBuf[i] = pos[srcIndices[type][i]];
        }

        for (int i = 0; i < 9; ++i) {
            pos[dstIndices[type][i]] = tmpBuf[i];
        }
    }
}

void MyRubik::doIncRot(int type, float t)
{
    for (int i = 0; i < 9; ++i) {
        const MyQuaternion& cur = qTransforms[pos[srcIndices[type][i]]];
        mTransforms[pos[srcIndices[type][i]]] = MyQuaternion::slerp(
                                                    cur,
                                                    faceRotationEnd[i],
                                                    t).toMatrix();
    }
}

void MyRubik::startRot(int type, bool inv)
{
    MyQuaternion endQuat = rotTypeToQuat(type);
    if (inv) {
        endQuat.toOppositeAxis();
    }
    for (int i = 0; i < 9; ++i) {
        const MyQuaternion& c = qTransforms[pos[srcIndices[type][i]]];
        faceRotationEnd[i] = endQuat * c;
        faceRotationEnd[i].normalize();
    }
}

void MyRubik::initialize()
{
    faceNormal[0].z = 1;
    faceNormal[1].x = 1;
    faceNormal[2].x = -1;
    faceNormal[3].z = -1;
    faceNormal[4].y = -1;
    faceNormal[5].y = 1;

    // Give some space between the cubes
    const float indvRadius = radius() - 0.004f;

    for (int i = 0; i < 27; ++i) {
        for (int j = 0; j < 36; ++j) {
            colors[i].vertices[j] = inside;
        }
    }

    cubes[0].set(MyPoint(-radius(), -radius(), radius()), indvRadius);
    colors[0].setFace(MyCube::LEFT, blue);
    colors[0].setFace(MyCube::FRONT, red);
    colors[0].setFace(MyCube::BOTTOM, white);

    cubes[1].set(MyPoint(0.0f, -radius(), radius()), indvRadius);
    colors[1].setFace(MyCube::FRONT, red);
    colors[1].setFace(MyCube::BOTTOM, white);

    cubes[2].set(MyPoint(radius(), -radius(), radius()), indvRadius);
    colors[2].setFace(MyCube::FRONT, red);
    colors[2].setFace(MyCube::BOTTOM, white);
    colors[2].setFace(MyCube::RIGHT, green);

    cubes[3].set(MyPoint(-radius(), 0.0f, radius()), indvRadius);
    colors[3].setFace(MyCube::LEFT, blue);
    colors[3].setFace(MyCube::FRONT, red);

    cubes[4].set(MyPoint(0.0f, 0.0f, radius()), indvRadius);
    colors[4].setFace(MyCube::FRONT, red);

    cubes[5].set(MyPoint(radius(), 0.0f, radius()), indvRadius);
    colors[5].setFace(MyCube::FRONT, red);
    colors[5].setFace(MyCube::RIGHT, green);

    cubes[6].set(MyPoint(-radius(), radius(), radius()), indvRadius);
    colors[6].setFace(MyCube::LEFT, blue);
    colors[6].setFace(MyCube::FRONT, red);
    colors[6].setFace(MyCube::TOP, yellow);

    cubes[7].set(MyPoint(0.0f, radius(), radius()), indvRadius);
    colors[7].setFace(MyCube::FRONT, red);
    colors[7].setFace(MyCube::TOP, yellow);

    cubes[8].set(MyPoint(radius(), radius(), radius()), indvRadius);
    colors[8].setFace(MyCube::FRONT, red);
    colors[8].setFace(MyCube::RIGHT, green);
    colors[8].setFace(MyCube::TOP, yellow);

    for (int i = 0; i < 9; ++i) {
        cubes[i+9] = cubes[i];
        cubes[i+9].addZ(-radius());
        colors[i+9] = colors[i];
        colors[i+9].setFace(MyCube::FRONT, inside);
    }

    for (int i = 0; i < 9; ++i) {
        cubes[i+18] = cubes[i];
        cubes[i+18].addZ(-radius()*2);
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

constexpr MyPoint MyRubik::red;
constexpr MyPoint MyRubik::green;
constexpr MyPoint MyRubik::blue;
constexpr MyPoint MyRubik::yellow;
constexpr MyPoint MyRubik::orange;
constexpr MyPoint MyRubik::white;
constexpr MyPoint MyRubik::inside;
constexpr int MyRubik::srcIndices[6][9];
constexpr int MyRubik::dstIndices[6][9];
