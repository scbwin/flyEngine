#ifndef GLSHADERINTERFACE_H
#define GLSHADERINTERFACE_H

#include <GL/glew.h>
#include <opengl/OpenGLUtils.h>
#include <math/FlyMath.h>
#include <vector>

namespace fly
{
  static inline void setScalar(int loc, int s) { GL_CHECK(glUniform1i(loc, s)); }
  static inline void setScalar(int loc, float s) { GL_CHECK(glUniform1f(loc, s)); }
  static inline void setVector(int loc, const Vec2f& v) { GL_CHECK(glUniform2f(loc, v[0], v[1])); }
  static inline void setVector(int loc, const Vec3f& v) { GL_CHECK(glUniform3f(loc, v[0], v[1], v[2])); }
  static inline void setVector(int loc, const Vec4f& v) { GL_CHECK(glUniform4f(loc, v[0], v[1], v[2], v[3])); }
  static inline void setMatrix(int loc, const Mat2f& m) { GL_CHECK(glUniformMatrix2fv(loc, 1, false, m.ptr())); }
  static inline void setMatrix(int loc, const Mat3f& m) { GL_CHECK(glUniformMatrix3fv(loc, 1, false, m.ptr())); }
  static inline void setMatrix(int loc, const Mat4f& m) { GL_CHECK(glUniformMatrix4fv(loc, 1, false, m.ptr())); }
  static inline void setMatrixTranspose(int loc, const Mat2f& m) { GL_CHECK(glUniformMatrix2fv(loc, 1, true, m.ptr())); }
  static inline void setMatrixTranspose(int loc, const Mat3f& m) { GL_CHECK(glUniformMatrix3fv(loc, 1, true, m.ptr())); }
  static inline void setMatrixTranspose(int loc, const Mat4f& m) { GL_CHECK(glUniformMatrix4fv(loc, 1, true, m.ptr())); }
  static inline void setScalarArray(int loc, const float& s, unsigned count) { GL_CHECK(glUniform1fv(loc, count, &s)); }
  static inline void setMatrixArray(int loc, const Mat4f& first, unsigned count) { GL_CHECK(glUniformMatrix4fv(loc, count, false, first.ptr())); }
}

#endif