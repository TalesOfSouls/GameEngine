/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_WIN32_H
#define COMS_GPUAPI_OPENGL_WIN32_H

#include <windows.h>
#include "../../platform/win32/Window.h"
#include "../../stdlib/Types.h"
#include "OpenglDefines.h"

#pragma comment(lib, "OpenGL32.Lib")

typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

extern "C" {
    WINGDIAPI void APIENTRY glAccum (GLenum op, GLfloat value);
    WINGDIAPI void APIENTRY glAlphaFunc (GLenum func, GLclampf ref);
    WINGDIAPI GLboolean APIENTRY glAreTexturesResident (GLsizei n, const GLuint *textures, GLboolean *residences);
    WINGDIAPI void APIENTRY glArrayElement (GLint i);
    WINGDIAPI void APIENTRY glBegin (GLenum mode);
    WINGDIAPI void APIENTRY glBindTexture (GLenum target, GLuint texture);
    WINGDIAPI void APIENTRY glBitmap (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
    WINGDIAPI void APIENTRY glBlendFunc (GLenum sfactor, GLenum dfactor);
    WINGDIAPI void APIENTRY glCallList (GLuint list);
    WINGDIAPI void APIENTRY glCallLists (GLsizei n, GLenum type, const GLvoid *lists);
    WINGDIAPI void APIENTRY glClear (GLbitfield mask);
    WINGDIAPI void APIENTRY glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    WINGDIAPI void APIENTRY glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    WINGDIAPI void APIENTRY glClearDepth (GLclampd depth);
    WINGDIAPI void APIENTRY glClearIndex (GLfloat c);
    WINGDIAPI void APIENTRY glClearStencil (GLint s);
    WINGDIAPI void APIENTRY glClipPlane (GLenum plane, const GLdouble *equation);
    WINGDIAPI void APIENTRY glColor3b (GLbyte red, GLbyte green, GLbyte blue);
    WINGDIAPI void APIENTRY glColor3bv (const GLbyte *v);
    WINGDIAPI void APIENTRY glColor3d (GLdouble red, GLdouble green, GLdouble blue);
    WINGDIAPI void APIENTRY glColor3dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glColor3f (GLfloat red, GLfloat green, GLfloat blue);
    WINGDIAPI void APIENTRY glColor3fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glColor3i (GLint red, GLint green, GLint blue);
    WINGDIAPI void APIENTRY glColor3iv (const GLint *v);
    WINGDIAPI void APIENTRY glColor3s (GLshort red, GLshort green, GLshort blue);
    WINGDIAPI void APIENTRY glColor3sv (const GLshort *v);
    WINGDIAPI void APIENTRY glColor3ub (GLubyte red, GLubyte green, GLubyte blue);
    WINGDIAPI void APIENTRY glColor3ubv (const GLubyte *v);
    WINGDIAPI void APIENTRY glColor3ui (GLuint red, GLuint green, GLuint blue);
    WINGDIAPI void APIENTRY glColor3uiv (const GLuint *v);
    WINGDIAPI void APIENTRY glColor3us (GLushort red, GLushort green, GLushort blue);
    WINGDIAPI void APIENTRY glColor3usv (const GLushort *v);
    WINGDIAPI void APIENTRY glColor4b (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
    WINGDIAPI void APIENTRY glColor4bv (const GLbyte *v);
    WINGDIAPI void APIENTRY glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
    WINGDIAPI void APIENTRY glColor4dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glColor4f (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
    WINGDIAPI void APIENTRY glColor4fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glColor4i (GLint red, GLint green, GLint blue, GLint alpha);
    WINGDIAPI void APIENTRY glColor4iv (const GLint *v);
    WINGDIAPI void APIENTRY glColor4s (GLshort red, GLshort green, GLshort blue, GLshort alpha);
    WINGDIAPI void APIENTRY glColor4sv (const GLshort *v);
    WINGDIAPI void APIENTRY glColor4ub (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
    WINGDIAPI void APIENTRY glColor4ubv (const GLubyte *v);
    WINGDIAPI void APIENTRY glColor4ui (GLuint red, GLuint green, GLuint blue, GLuint alpha);
    WINGDIAPI void APIENTRY glColor4uiv (const GLuint *v);
    WINGDIAPI void APIENTRY glColor4us (GLushort red, GLushort green, GLushort blue, GLushort alpha);
    WINGDIAPI void APIENTRY glColor4usv (const GLushort *v);
    WINGDIAPI void APIENTRY glColorMask (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    WINGDIAPI void APIENTRY glColorMaterial (GLenum face, GLenum mode);
    WINGDIAPI void APIENTRY glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glCopyPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
    WINGDIAPI void APIENTRY glCopyTexImage1D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
    WINGDIAPI void APIENTRY glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    WINGDIAPI void APIENTRY glCopyTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
    WINGDIAPI void APIENTRY glCopyTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    WINGDIAPI void APIENTRY glCullFace (GLenum mode);
    WINGDIAPI void APIENTRY glDeleteLists (GLuint list, GLsizei range);
    WINGDIAPI void APIENTRY glDeleteTextures (GLsizei n, const GLuint *textures);
    WINGDIAPI void APIENTRY glDepthFunc (GLenum func);
    WINGDIAPI void APIENTRY glDepthMask (GLboolean flag);
    WINGDIAPI void APIENTRY glDepthRange (GLclampd zNear, GLclampd zFar);
    WINGDIAPI void APIENTRY glDisable (GLenum cap);
    WINGDIAPI void APIENTRY glDisableClientState (GLenum array);
    WINGDIAPI void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count);
    WINGDIAPI void APIENTRY glDrawBuffer (GLenum mode);
    WINGDIAPI void APIENTRY glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    WINGDIAPI void APIENTRY glDrawPixels (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    WINGDIAPI void APIENTRY glEdgeFlag (GLboolean flag);
    WINGDIAPI void APIENTRY glEdgeFlagPointer (GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glEdgeFlagv (const GLboolean *flag);
    WINGDIAPI void APIENTRY glEnable (GLenum cap);
    WINGDIAPI void APIENTRY glEnableClientState (GLenum array);
    WINGDIAPI void APIENTRY glEnd (void);
    WINGDIAPI void APIENTRY glEndList (void);
    WINGDIAPI void APIENTRY glEvalCoord1d (GLdouble u);
    WINGDIAPI void APIENTRY glEvalCoord1dv (const GLdouble *u);
    WINGDIAPI void APIENTRY glEvalCoord1f (GLfloat u);
    WINGDIAPI void APIENTRY glEvalCoord1fv (const GLfloat *u);
    WINGDIAPI void APIENTRY glEvalCoord2d (GLdouble u, GLdouble v);
    WINGDIAPI void APIENTRY glEvalCoord2dv (const GLdouble *u);
    WINGDIAPI void APIENTRY glEvalCoord2f (GLfloat u, GLfloat v);
    WINGDIAPI void APIENTRY glEvalCoord2fv (const GLfloat *u);
    WINGDIAPI void APIENTRY glEvalMesh1 (GLenum mode, GLint i1, GLint i2);
    WINGDIAPI void APIENTRY glEvalMesh2 (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
    WINGDIAPI void APIENTRY glEvalPoint1 (GLint i);
    WINGDIAPI void APIENTRY glEvalPoint2 (GLint i, GLint j);
    WINGDIAPI void APIENTRY glFeedbackBuffer (GLsizei size, GLenum type, GLfloat *buffer);
    WINGDIAPI void APIENTRY glFinish (void);
    WINGDIAPI void APIENTRY glFlush (void);
    WINGDIAPI void APIENTRY glFogf (GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glFogfv (GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glFogi (GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glFogiv (GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glFrontFace (GLenum mode);
    WINGDIAPI void APIENTRY glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    WINGDIAPI GLuint APIENTRY glGenLists (GLsizei range);
    WINGDIAPI void APIENTRY glGenTextures (GLsizei n, GLuint *textures);
    WINGDIAPI void APIENTRY glGetBooleanv (GLenum pname, GLboolean *params);
    WINGDIAPI void APIENTRY glGetClipPlane (GLenum plane, GLdouble *equation);
    WINGDIAPI void APIENTRY glGetDoublev (GLenum pname, GLdouble *params);
    WINGDIAPI GLenum APIENTRY glGetError (void);
    WINGDIAPI void APIENTRY glGetFloatv (GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetIntegerv (GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetLightfv (GLenum light, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetLightiv (GLenum light, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetMapdv (GLenum target, GLenum query, GLdouble *v);
    WINGDIAPI void APIENTRY glGetMapfv (GLenum target, GLenum query, GLfloat *v);
    WINGDIAPI void APIENTRY glGetMapiv (GLenum target, GLenum query, GLint *v);
    WINGDIAPI void APIENTRY glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetMaterialiv (GLenum face, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetPixelMapfv (GLenum map, GLfloat* values);
    WINGDIAPI void APIENTRY glGetPixelMapuiv (GLenum map, GLuint *values);
    WINGDIAPI void APIENTRY glGetPixelMapusv (GLenum map, GLushort *values);
    WINGDIAPI void APIENTRY glGetPointerv (GLenum pname, GLvoid* *params);
    WINGDIAPI void APIENTRY glGetPolygonStipple (GLubyte *mask);
    WINGDIAPI const GLubyte * APIENTRY glGetString (GLenum name);
    WINGDIAPI void APIENTRY glGetTexEnvfv (GLenum target, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetTexEnviv (GLenum target, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetTexGendv (GLenum coord, GLenum pname, GLdouble *params);
    WINGDIAPI void APIENTRY glGetTexGenfv (GLenum coord, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetTexGeniv (GLenum coord, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetTexImage (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
    WINGDIAPI void APIENTRY glGetTexLevelParameterfv (GLenum target, GLint level, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetTexLevelParameteriv (GLenum target, GLint level, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glGetTexParameterfv (GLenum target, GLenum pname, GLfloat *params);
    WINGDIAPI void APIENTRY glGetTexParameteriv (GLenum target, GLenum pname, GLint *params);
    WINGDIAPI void APIENTRY glHint (GLenum target, GLenum mode);
    WINGDIAPI void APIENTRY glIndexMask (GLuint mask);
    WINGDIAPI void APIENTRY glIndexPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glIndexd (GLdouble c);
    WINGDIAPI void APIENTRY glIndexdv (const GLdouble *c);
    WINGDIAPI void APIENTRY glIndexf (GLfloat c);
    WINGDIAPI void APIENTRY glIndexfv (const GLfloat *c);
    WINGDIAPI void APIENTRY glIndexi (GLint c);
    WINGDIAPI void APIENTRY glIndexiv (const GLint *c);
    WINGDIAPI void APIENTRY glIndexs (GLshort c);
    WINGDIAPI void APIENTRY glIndexsv (const GLshort *c);
    WINGDIAPI void APIENTRY glIndexub (GLubyte c);
    WINGDIAPI void APIENTRY glIndexubv (const GLubyte *c);
    WINGDIAPI void APIENTRY glInitNames (void);
    WINGDIAPI void APIENTRY glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI GLboolean APIENTRY glIsEnabled (GLenum cap);
    WINGDIAPI GLboolean APIENTRY glIsList (GLuint list);
    WINGDIAPI GLboolean APIENTRY glIsTexture (GLuint texture);
    WINGDIAPI void APIENTRY glLightModelf (GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glLightModelfv (GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glLightModeli (GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glLightModeliv (GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glLightf (GLenum light, GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glLightfv (GLenum light, GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glLighti (GLenum light, GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glLightiv (GLenum light, GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glLineStipple (GLint factor, GLushort pattern);
    WINGDIAPI void APIENTRY glLineWidth (GLfloat width);
    WINGDIAPI void APIENTRY glListBase (GLuint base);
    WINGDIAPI void APIENTRY glLoadIdentity (void);
    WINGDIAPI void APIENTRY glLoadMatrixd (const GLdouble *m);
    WINGDIAPI void APIENTRY glLoadMatrixf (const GLfloat *m);
    WINGDIAPI void APIENTRY glLoadName (GLuint name);
    WINGDIAPI void APIENTRY glLogicOp (GLenum opcode);
    WINGDIAPI void APIENTRY glMap1d (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
    WINGDIAPI void APIENTRY glMap1f (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
    WINGDIAPI void APIENTRY glMap2d (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
    WINGDIAPI void APIENTRY glMap2f (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
    WINGDIAPI void APIENTRY glMapGrid1d (GLint un, GLdouble u1, GLdouble u2);
    WINGDIAPI void APIENTRY glMapGrid1f (GLint un, GLfloat u1, GLfloat u2);
    WINGDIAPI void APIENTRY glMapGrid2d (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
    WINGDIAPI void APIENTRY glMapGrid2f (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
    WINGDIAPI void APIENTRY glMaterialf (GLenum face, GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glMaterialfv (GLenum face, GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glMateriali (GLenum face, GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glMaterialiv (GLenum face, GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glMatrixMode (GLenum mode);
    WINGDIAPI void APIENTRY glMultMatrixd (const GLdouble *m);
    WINGDIAPI void APIENTRY glMultMatrixf (const GLfloat *m);
    WINGDIAPI void APIENTRY glNewList (GLuint list, GLenum mode);
    WINGDIAPI void APIENTRY glNormal3b (GLbyte nx, GLbyte ny, GLbyte nz);
    WINGDIAPI void APIENTRY glNormal3bv (const GLbyte *v);
    WINGDIAPI void APIENTRY glNormal3d (GLdouble nx, GLdouble ny, GLdouble nz);
    WINGDIAPI void APIENTRY glNormal3dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz);
    WINGDIAPI void APIENTRY glNormal3fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glNormal3i (GLint nx, GLint ny, GLint nz);
    WINGDIAPI void APIENTRY glNormal3iv (const GLint *v);
    WINGDIAPI void APIENTRY glNormal3s (GLshort nx, GLshort ny, GLshort nz);
    WINGDIAPI void APIENTRY glNormal3sv (const GLshort *v);
    WINGDIAPI void APIENTRY glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glOrtho (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
    WINGDIAPI void APIENTRY glPassThrough (GLfloat token);
    WINGDIAPI void APIENTRY glPixelMapfv (GLenum map, GLsizei mapsize, const GLfloat* values);
    WINGDIAPI void APIENTRY glPixelMapuiv (GLenum map, GLsizei mapsize, const GLuint *values);
    WINGDIAPI void APIENTRY glPixelMapusv (GLenum map, GLsizei mapsize, const GLushort *values);
    WINGDIAPI void APIENTRY glPixelStoref (GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glPixelStorei (GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glPixelTransferf (GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glPixelTransferi (GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glPixelZoom (GLfloat xfactor, GLfloat yfactor);
    WINGDIAPI void APIENTRY glPointSize (GLfloat size);
    WINGDIAPI void APIENTRY glPolygonMode (GLenum face, GLenum mode);
    WINGDIAPI void APIENTRY glPolygonOffset (GLfloat factor, GLfloat units);
    WINGDIAPI void APIENTRY glPolygonStipple (const GLubyte *mask);
    WINGDIAPI void APIENTRY glPopAttrib (void);
    WINGDIAPI void APIENTRY glPopClientAttrib (void);
    WINGDIAPI void APIENTRY glPopMatrix (void);
    WINGDIAPI void APIENTRY glPopName (void);
    WINGDIAPI void APIENTRY glPrioritizeTextures (GLsizei n, const GLuint *textures, const GLclampf *priorities);
    WINGDIAPI void APIENTRY glPushAttrib (GLbitfield mask);
    WINGDIAPI void APIENTRY glPushClientAttrib (GLbitfield mask);
    WINGDIAPI void APIENTRY glPushMatrix (void);
    WINGDIAPI void APIENTRY glPushName (GLuint name);
    WINGDIAPI void APIENTRY glRasterPos2d (GLdouble x, GLdouble y);
    WINGDIAPI void APIENTRY glRasterPos2dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glRasterPos2f (GLfloat x, GLfloat y);
    WINGDIAPI void APIENTRY glRasterPos2fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glRasterPos2i (GLint x, GLint y);
    WINGDIAPI void APIENTRY glRasterPos2iv (const GLint *v);
    WINGDIAPI void APIENTRY glRasterPos2s (GLshort x, GLshort y);
    WINGDIAPI void APIENTRY glRasterPos2sv (const GLshort *v);
    WINGDIAPI void APIENTRY glRasterPos3d (GLdouble x, GLdouble y, GLdouble z);
    WINGDIAPI void APIENTRY glRasterPos3dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glRasterPos3f (GLfloat x, GLfloat y, GLfloat z);
    WINGDIAPI void APIENTRY glRasterPos3fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glRasterPos3i (GLint x, GLint y, GLint z);
    WINGDIAPI void APIENTRY glRasterPos3iv (const GLint *v);
    WINGDIAPI void APIENTRY glRasterPos3s (GLshort x, GLshort y, GLshort z);
    WINGDIAPI void APIENTRY glRasterPos3sv (const GLshort *v);
    WINGDIAPI void APIENTRY glRasterPos4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    WINGDIAPI void APIENTRY glRasterPos4dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glRasterPos4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    WINGDIAPI void APIENTRY glRasterPos4fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glRasterPos4i (GLint x, GLint y, GLint z, GLint w);
    WINGDIAPI void APIENTRY glRasterPos4iv (const GLint *v);
    WINGDIAPI void APIENTRY glRasterPos4s (GLshort x, GLshort y, GLshort z, GLshort w);
    WINGDIAPI void APIENTRY glRasterPos4sv (const GLshort *v);
    WINGDIAPI void APIENTRY glReadBuffer (GLenum mode);
    WINGDIAPI void APIENTRY glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
    WINGDIAPI void APIENTRY glRectd (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
    WINGDIAPI void APIENTRY glRectdv (const GLdouble *v1, const GLdouble *v2);
    WINGDIAPI void APIENTRY glRectf (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
    WINGDIAPI void APIENTRY glRectfv (const GLfloat *v1, const GLfloat *v2);
    WINGDIAPI void APIENTRY glRecti (GLint x1, GLint y1, GLint x2, GLint y2);
    WINGDIAPI void APIENTRY glRectiv (const GLint *v1, const GLint *v2);
    WINGDIAPI void APIENTRY glRects (GLshort x1, GLshort y1, GLshort x2, GLshort y2);
    WINGDIAPI void APIENTRY glRectsv (const GLshort *v1, const GLshort *v2);
    WINGDIAPI GLint APIENTRY glRenderMode (GLenum mode);
    WINGDIAPI void APIENTRY glRotated (GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
    WINGDIAPI void APIENTRY glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    WINGDIAPI void APIENTRY glScaled (GLdouble x, GLdouble y, GLdouble z);
    WINGDIAPI void APIENTRY glScalef (GLfloat x, GLfloat y, GLfloat z);
    WINGDIAPI void APIENTRY glScissor (GLint x, GLint y, GLsizei width, GLsizei height);
    WINGDIAPI void APIENTRY glSelectBuffer (GLsizei size, GLuint *buffer);
    WINGDIAPI void APIENTRY glShadeModel (GLenum mode);
    WINGDIAPI void APIENTRY glStencilFunc (GLenum func, GLint ref, GLuint mask);
    WINGDIAPI void APIENTRY glStencilMask (GLuint mask);
    WINGDIAPI void APIENTRY glStencilOp (GLenum fail, GLenum zfail, GLenum zpass);
    WINGDIAPI void APIENTRY glTexCoord1d (GLdouble s);
    WINGDIAPI void APIENTRY glTexCoord1dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glTexCoord1f (GLfloat s);
    WINGDIAPI void APIENTRY glTexCoord1fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glTexCoord1i (GLint s);
    WINGDIAPI void APIENTRY glTexCoord1iv (const GLint *v);
    WINGDIAPI void APIENTRY glTexCoord1s (GLshort s);
    WINGDIAPI void APIENTRY glTexCoord1sv (const GLshort *v);
    WINGDIAPI void APIENTRY glTexCoord2d (GLdouble s, GLdouble t);
    WINGDIAPI void APIENTRY glTexCoord2dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glTexCoord2f (GLfloat s, GLfloat t);
    WINGDIAPI void APIENTRY glTexCoord2fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glTexCoord2i (GLint s, GLint t);
    WINGDIAPI void APIENTRY glTexCoord2iv (const GLint *v);
    WINGDIAPI void APIENTRY glTexCoord2s (GLshort s, GLshort t);
    WINGDIAPI void APIENTRY glTexCoord2sv (const GLshort *v);
    WINGDIAPI void APIENTRY glTexCoord3d (GLdouble s, GLdouble t, GLdouble r);
    WINGDIAPI void APIENTRY glTexCoord3dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glTexCoord3f (GLfloat s, GLfloat t, GLfloat r);
    WINGDIAPI void APIENTRY glTexCoord3fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glTexCoord3i (GLint s, GLint t, GLint r);
    WINGDIAPI void APIENTRY glTexCoord3iv (const GLint *v);
    WINGDIAPI void APIENTRY glTexCoord3s (GLshort s, GLshort t, GLshort r);
    WINGDIAPI void APIENTRY glTexCoord3sv (const GLshort *v);
    WINGDIAPI void APIENTRY glTexCoord4d (GLdouble s, GLdouble t, GLdouble r, GLdouble q);
    WINGDIAPI void APIENTRY glTexCoord4dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glTexCoord4f (GLfloat s, GLfloat t, GLfloat r, GLfloat q);
    WINGDIAPI void APIENTRY glTexCoord4fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glTexCoord4i (GLint s, GLint t, GLint r, GLint q);
    WINGDIAPI void APIENTRY glTexCoord4iv (const GLint *v);
    WINGDIAPI void APIENTRY glTexCoord4s (GLshort s, GLshort t, GLshort r, GLshort q);
    WINGDIAPI void APIENTRY glTexCoord4sv (const GLshort *v);
    WINGDIAPI void APIENTRY glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glTexEnvf (GLenum target, GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glTexEnvi (GLenum target, GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glTexEnviv (GLenum target, GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glTexGend (GLenum coord, GLenum pname, GLdouble param);
    WINGDIAPI void APIENTRY glTexGendv (GLenum coord, GLenum pname, const GLdouble *params);
    WINGDIAPI void APIENTRY glTexGenf (GLenum coord, GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glTexGenfv (GLenum coord, GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glTexGeni (GLenum coord, GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glTexGeniv (GLenum coord, GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glTexImage1D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    WINGDIAPI void APIENTRY glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    WINGDIAPI void APIENTRY glTexParameterf (GLenum target, GLenum pname, GLfloat param);
    WINGDIAPI void APIENTRY glTexParameterfv (GLenum target, GLenum pname, const GLfloat *params);
    WINGDIAPI void APIENTRY glTexParameteri (GLenum target, GLenum pname, GLint param);
    WINGDIAPI void APIENTRY glTexParameteriv (GLenum target, GLenum pname, const GLint *params);
    WINGDIAPI void APIENTRY glTexSubImage1D (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
    WINGDIAPI void APIENTRY glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    WINGDIAPI void APIENTRY glTranslated (GLdouble x, GLdouble y, GLdouble z);
    WINGDIAPI void APIENTRY glTranslatef (GLfloat x, GLfloat y, GLfloat z);
    WINGDIAPI void APIENTRY glVertex2d (GLdouble x, GLdouble y);
    WINGDIAPI void APIENTRY glVertex2dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glVertex2f (GLfloat x, GLfloat y);
    WINGDIAPI void APIENTRY glVertex2fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glVertex2i (GLint x, GLint y);
    WINGDIAPI void APIENTRY glVertex2iv (const GLint *v);
    WINGDIAPI void APIENTRY glVertex2s (GLshort x, GLshort y);
    WINGDIAPI void APIENTRY glVertex2sv (const GLshort *v);
    WINGDIAPI void APIENTRY glVertex3d (GLdouble x, GLdouble y, GLdouble z);
    WINGDIAPI void APIENTRY glVertex3dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glVertex3f (GLfloat x, GLfloat y, GLfloat z);
    WINGDIAPI void APIENTRY glVertex3fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glVertex3i (GLint x, GLint y, GLint z);
    WINGDIAPI void APIENTRY glVertex3iv (const GLint *v);
    WINGDIAPI void APIENTRY glVertex3s (GLshort x, GLshort y, GLshort z);
    WINGDIAPI void APIENTRY glVertex3sv (const GLshort *v);
    WINGDIAPI void APIENTRY glVertex4d (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
    WINGDIAPI void APIENTRY glVertex4dv (const GLdouble *v);
    WINGDIAPI void APIENTRY glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    WINGDIAPI void APIENTRY glVertex4fv (const GLfloat *v);
    WINGDIAPI void APIENTRY glVertex4i (GLint x, GLint y, GLint z, GLint w);
    WINGDIAPI void APIENTRY glVertex4iv (const GLint *v);
    WINGDIAPI void APIENTRY glVertex4s (GLshort x, GLshort y, GLshort z, GLshort w);
    WINGDIAPI void APIENTRY glVertex4sv (const GLshort *v);
    WINGDIAPI void APIENTRY glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
    WINGDIAPI void APIENTRY glViewport (GLint x, GLint y, GLsizei width, GLsizei height);

    typedef void (APIENTRY * PFNGLARRAYELEMENTEXTPROC) (GLint i);
    typedef void (APIENTRY * PFNGLDRAWARRAYSEXTPROC) (GLenum mode, GLint first, GLsizei count);
    typedef void (APIENTRY * PFNGLVERTEXPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
    typedef void (APIENTRY * PFNGLNORMALPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
    typedef void (APIENTRY * PFNGLCOLORPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
    typedef void (APIENTRY * PFNGLINDEXPOINTEREXTPROC) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
    typedef void (APIENTRY * PFNGLTEXCOORDPOINTEREXTPROC) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
    typedef void (APIENTRY * PFNGLEDGEFLAGPOINTEREXTPROC) (GLsizei stride, GLsizei count, const GLboolean *pointer);
    typedef void (APIENTRY * PFNGLGETPOINTERVEXTPROC) (GLenum pname, GLvoid* *params);
    typedef void (APIENTRY * PFNGLARRAYELEMENTARRAYEXTPROC)(GLenum mode, GLsizei count, const GLvoid* pi);

    typedef void (APIENTRY * PFNGLDRAWRANGEELEMENTSWINPROC) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

    typedef void (APIENTRY * PFNGLADDSWAPHINTRECTWINPROC)  (GLint x, GLint y, GLsizei width, GLsizei height);

    typedef void (APIENTRY * PFNGLCOLORTABLEEXTPROC)
        (GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
        GLenum type, const GLvoid *data);
    typedef void (APIENTRY * PFNGLCOLORSUBTABLEEXTPROC)
        (GLenum target, GLsizei start, GLsizei count, GLenum format,
        GLenum type, const GLvoid *data);
    typedef void (APIENTRY * PFNGLGETCOLORTABLEEXTPROC)
        (GLenum target, GLenum format, GLenum type, GLvoid *data);
    typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERIVEXTPROC)
        (GLenum target, GLenum pname, GLint *params);
    typedef void (APIENTRY * PFNGLGETCOLORTABLEPARAMETERFVEXTPROC)
        (GLenum target, GLenum pname, GLfloat *params);
}

typedef void WINAPI type_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
static type_glTexImage2DMultisample* glTexImage2DMultisample;

typedef GLsync WINAPI type_glFenceSync(GLenum condition, GLbitfield flags);
static type_glFenceSync* glFenceSync;

typedef GLenum WINAPI type_glClientWaitSync(GLsync GLsync, GLbitfield flags, GLuint64 timeout);
static type_glClientWaitSync* glClientWaitSync;

typedef void WINAPI type_glDeleteSync(GLsync GLsync);
static type_glDeleteSync* glDeleteSync;

typedef void WINAPI type_glBindFramebuffer(GLenum target, GLuint framebuffer);
static type_glBindFramebuffer* glBindFramebuffer;

typedef void WINAPI type_glGenFramebuffers(GLsizei n, GLuint *framebuffers);
static type_glGenFramebuffers* glGenFramebuffers;

typedef void WINAPI type_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
static type_glFramebufferTexture2D* glFramebufferTexture2D;

typedef GLenum WINAPI type_glCheckFramebufferStatus(GLenum target);
static type_glCheckFramebufferStatus* glCheckFramebufferStatus;

typedef void WINAPI type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
static type_glBlitFramebuffer* glBlitFramebuffer;

typedef void WINAPI type_glAttachShader(GLuint program, GLuint shader);
static type_glAttachShader* glAttachShader;

typedef void WINAPI type_glCompileShader(GLuint shader);
static type_glCompileShader* glCompileShader;

typedef GLuint WINAPI type_glCreateProgram(void);
static type_glCreateProgram* glCreateProgram;

typedef GLuint WINAPI type_glCreateShader(GLenum type);
static type_glCreateShader* glCreateShader;

typedef void WINAPI type_glLinkProgram(GLuint program);
static type_glLinkProgram* glLinkProgram;

typedef void WINAPI type_glShaderSource(GLuint shader, GLsizei count, GLchar **string, GLint *length);
static type_glShaderSource* glShaderSource;

typedef void WINAPI type_glUseProgram(GLuint program);
static type_glUseProgram* glUseProgram;

typedef void WINAPI type_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static type_glGetProgramInfoLog* glGetProgramInfoLog;

typedef void WINAPI type_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
static type_glGetShaderInfoLog* glGetShaderInfoLog;

typedef void WINAPI type_glValidateProgram(GLuint program);
static type_glValidateProgram* glValidateProgram;

typedef void WINAPI type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
static type_glGetProgramiv* glGetProgramiv;

typedef GLint WINAPI type_glGetUniformLocation(GLuint program, const GLchar *name);
static type_glGetUniformLocation* glGetUniformLocation;

typedef void WINAPI type_glUniform4fv(GLint location, GLsizei count, const GLfloat* value);
static type_glUniform4fv* glUniform4fv;

typedef void WINAPI type_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
static type_glUniform4i* glUniform4i;

typedef void WINAPI type_glUniform4ui(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
static type_glUniform4ui* glUniform4ui;

typedef void WINAPI type_glUniform1i(GLint location, GLint v0);
static type_glUniform1i* glUniform1i;

typedef void WINAPI type_glUniform1iv(GLint location, GLsizei count, const GLint* value);
static type_glUniform1iv* glUniform1iv;

typedef void WINAPI type_glUniform1f(GLint location, GLfloat v0);
static type_glUniform1f* glUniform1f;

typedef void WINAPI type_glUniform1fv(GLint location, GLsizei count, const GLfloat* value);
static type_glUniform1fv* glUniform1fv;

typedef void WINAPI type_glUniform2fv(GLint location, GLsizei count, const GLfloat* value);
static type_glUniform2fv* glUniform2fv;

typedef void WINAPI type_glUniform3fv(GLint location, GLsizei count, const GLfloat* value);
static type_glUniform3fv* glUniform3fv;

typedef void WINAPI type_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
static type_glUniform3f* glUniform3f;

typedef void WINAPI type_glUniform3iv(GLint location, GLsizei count, const GLint* value);
static type_glUniform3iv* glUniform3iv;

typedef void WINAPI type_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2);
static type_glUniform3i* glUniform3i;

typedef void WINAPI type_glEnableVertexAttribArray(GLuint index);
static type_glEnableVertexAttribArray* glEnableVertexAttribArray;

typedef void WINAPI type_glDisableVertexAttribArray(GLuint index);
static type_glDisableVertexAttribArray* glDisableVertexAttribArray;

typedef GLint WINAPI type_glGetAttribLocation(GLuint program, const GLchar *name);
static type_glGetAttribLocation* glGetAttribLocation;

typedef void WINAPI type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
static type_glVertexAttribPointer* glVertexAttribPointer;

typedef void WINAPI type_glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
static type_glVertexAttribIPointer* glVertexAttribIPointer;

typedef void WINAPI type_glBindVertexArray(GLuint array);
static type_glBindVertexArray* glBindVertexArray;

typedef void WINAPI type_glGenVertexArrays(GLsizei n, GLuint *arrays);
static type_glGenVertexArrays* glGenVertexArrays;

typedef void WINAPI type_glBindBuffer(GLenum target, GLuint buffer);
static type_glBindBuffer* glBindBuffer;

typedef void WINAPI type_glBindBufferBase(GLenum target, GLuint index, GLuint buffer);
static type_glBindBufferBase* glBindBufferBase;

typedef void WINAPI type_glBufferSubData(GLenum target, GLuint offset, GLsizeiptr size, const void* data);
static type_glBufferSubData* glBufferSubData;

typedef void WINAPI type_glGenBuffers(GLsizei n, GLuint *buffers);
static type_glGenBuffers* glGenBuffers;

typedef void WINAPI type_glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
static type_glGenRenderbuffers* glGenRenderbuffers;

typedef void WINAPI type_glBindRenderbuffer(GLenum target, GLuint renderbuffer);
static type_glBindRenderbuffer* glBindRenderbuffer;

typedef void WINAPI type_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
static type_glRenderbufferStorage* glRenderbufferStorage;

typedef void WINAPI type_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
static type_glRenderbufferStorageMultisample* glRenderbufferStorageMultisample;

typedef void WINAPI type_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
static type_glFramebufferRenderbuffer* glFramebufferRenderbuffer;

typedef void WINAPI type_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
static type_glBufferData* glBufferData;

typedef void WINAPI type_glActiveTexture(GLenum texture);
static type_glActiveTexture* glActiveTexture;

typedef void WINAPI type_glDeleteProgram(GLuint program);
static type_glDeleteProgram* glDeleteProgram;

typedef void WINAPI type_glDeleteVertexArrays(GLsizei n, const GLuint* arrays);
static type_glDeleteVertexArrays* glDeleteVertexArrays;

typedef void WINAPI type_glDeleteShader(GLuint shader);
static type_glDeleteShader* glDeleteShader;

typedef void WINAPI type_glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers);
static type_glDeleteFramebuffers* glDeleteFramebuffers;

typedef void WINAPI type_glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
static type_glDeleteRenderbuffers* glDeleteRenderbuffers;

typedef void WINAPI type_glDrawBuffers(GLsizei n, const GLenum *bufs);
static type_glDrawBuffers* glDrawBuffers;

typedef void WINAPI type_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
static type_glTexImage3D* glTexImage3D;

typedef void WINAPI type_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
static type_glTexSubImage3D* glTexSubImage3D;

typedef void WINAPI type_glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
static type_glDrawElementsBaseVertex* glDrawElementsBaseVertex;

typedef void WINAPI type_glGenerateMipmap(GLenum target);
static type_glGenerateMipmap* glGenerateMipmap;

typedef void WINAPI type_glDetachShader(GLuint program, GLuint shader);
static type_glDetachShader* glDetachShader;

typedef void WINAPI type_glDeleteBuffers(GLsizei n, const GLuint* buffers);
static type_glDeleteBuffers* glDeleteBuffers;

typedef void WINAPI type_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
static type_glUniformMatrix2fv* glUniformMatrix2fv;

typedef void WINAPI type_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
static type_glUniformMatrix3fv* glUniformMatrix3fv;

typedef void WINAPI type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
static type_glUniformMatrix4fv* glUniformMatrix4fv;

typedef void WINAPI type_glGetShaderiv(GLuint shader, GLenum pname, GLint* param);
static type_glGetShaderiv* glGetShaderiv;

typedef void WINAPI type_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
static type_glDrawArraysInstanced* glDrawArraysInstanced;

typedef void WINAPI type_glDrawElementsInstanced(GLenum mode, GLint count, GLenum type, const void* indices, GLsizei instancecount);
static type_glDrawElementsInstanced* glDrawElementsInstanced;

typedef void WINAPI type_glProgramParameteri(GLuint program, GLenum pname, GLint value);
static type_glProgramParameteri* glProgramParameteri;

typedef void WINAPI gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
typedef void WINAPI type_glDebugMessageCallback(gl_debug_callback* callback, const void* userParam);
static type_glDebugMessageCallback* glDebugMessageCallback;

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013

#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_FULL_ACCELERATION_ARB 0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9

#define WGL_RED_BITS_ARB 0x2015
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_DEPTH_BITS_ARB 0x2022

#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext, const int *attribList);
static wgl_create_context_attribs_arb* wglCreateContextAttribsARB;

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
static wgl_get_pixel_format_attrib_iv_arb* wglGetPixelFormatAttribivARB;

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
static wgl_get_pixel_format_attrib_fv_arb* wglGetPixelFormatAttribfvARB;

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
static wgl_choose_pixel_format_arb* wglChoosePixelFormatARB;

typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
static wgl_swap_interval_ext* wglSwapIntervalEXT;

typedef const char* WINAPI wgl_get_extensions_string_ext(void);
static wgl_get_extensions_string_ext* wglGetExtensionsStringEXT;

void set_pixel_format(HDC hdc, int32 multisampling = 0)
{
    int32 suggested_pixel_format_idx = 0;
    uint32 extended_pick = 0;

    if (wglChoosePixelFormatARB) {
        int32 attr_list[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            multisampling > 0 ? WGL_SAMPLE_BUFFERS_ARB : 0, (int32) (multisampling > 0),
            WGL_SAMPLES_ARB, multisampling, // MSAA
            0,
        };

        wglChoosePixelFormatARB(hdc, attr_list, 0, 1, &suggested_pixel_format_idx, &extended_pick);
    }

    if(!extended_pick) {
        PIXELFORMATDESCRIPTOR desired_pixel_format = {};
        desired_pixel_format.nSize = sizeof(desired_pixel_format);
        desired_pixel_format.nVersion = 1;
        desired_pixel_format.iPixelType = PFD_TYPE_RGBA;
        desired_pixel_format.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        desired_pixel_format.cColorBits = 32;
        desired_pixel_format.cAlphaBits = 8;
        desired_pixel_format.cDepthBits = 24;
        desired_pixel_format.iLayerType = PFD_MAIN_PLANE;

        suggested_pixel_format_idx = ChoosePixelFormat(hdc, &desired_pixel_format);
    }

    PIXELFORMATDESCRIPTOR suggested_pixel_format;
    DescribePixelFormat(hdc, suggested_pixel_format_idx, sizeof(suggested_pixel_format), &suggested_pixel_format);
    SetPixelFormat(hdc, suggested_pixel_format_idx, &suggested_pixel_format);
}

bool gl_extensions_load()
{
    WNDCLASSA wc = {};

    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "WGLLoader";

    if (!RegisterClassA(&wc)) {
        return false;
    }

    HWND window = CreateWindowExA(
        0,
        wc.lpszClassName,
        "ExtensionLoader",
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        wc.hInstance,
        0
    );

    HDC hdc = GetDC(window);
    set_pixel_format(hdc);

    HGLRC openGLRC = wglCreateContext(hdc);

    if (!wglMakeCurrent(hdc, openGLRC) || !wglGetExtensionsStringEXT) {
        return false;
    }

    char *extension = (char *) wglGetExtensionsStringEXT();
    char *pos = extension;

    while(*pos) {
        while(*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') {
            ++pos;
        }

        char *end = pos;
        while(*end && !(*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
            ++end;
        }

        // umm count = end - pos;
        // OpenGL->SupportsSRGBFramebuffer = str_compare(count, pos, "WGL_EXT_framebuffer_sRGB") == 0 || str_compare(count, pos, "WGL_ARB_framebuffer_sRGB") == 0;

        pos = end;
    }

    wglMakeCurrent(NULL, NULL);

    wglDeleteContext(openGLRC);
    ReleaseDC(window, hdc);
    DestroyWindow(window);

    return true;
}

const int win32_opengl_attribs[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 6,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
    0,
};

void opengl_init_wgl()
{
    wglChoosePixelFormatARB = (wgl_choose_pixel_format_arb *) wglGetProcAddress("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (wgl_create_context_attribs_arb *) wglGetProcAddress("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (wgl_swap_interval_ext *) wglGetProcAddress("wglSwapIntervalEXT");
    wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *) wglGetProcAddress("wglGetExtensionsStringEXT");
}

void opengl_init_gl()
{
    glTexImage2DMultisample = (type_glTexImage2DMultisample *) wglGetProcAddress("glTexImage2DMultisample");
    glFenceSync = (type_glFenceSync *) wglGetProcAddress("glFenceSync");
    glClientWaitSync = (type_glClientWaitSync *) wglGetProcAddress("glClientWaitSync");
    glDeleteSync = (type_glDeleteSync *) wglGetProcAddress("glDeleteSync");
    glBindFramebuffer = (type_glBindFramebuffer *) wglGetProcAddress("glBindFramebuffer");
    glGenFramebuffers = (type_glGenFramebuffers *) wglGetProcAddress("glGenFramebuffers");
    glFramebufferTexture2D = (type_glFramebufferTexture2D *) wglGetProcAddress("glFramebufferTexture2D");
    glCheckFramebufferStatus = (type_glCheckFramebufferStatus *) wglGetProcAddress("glCheckFramebufferStatus");
    glBlitFramebuffer = (type_glBlitFramebuffer *) wglGetProcAddress("glBlitFramebuffer");
    glAttachShader = (type_glAttachShader *) wglGetProcAddress("glAttachShader");
    glCompileShader = (type_glCompileShader *) wglGetProcAddress("glCompileShader");
    glCreateProgram = (type_glCreateProgram *) wglGetProcAddress("glCreateProgram");
    glCreateShader = (type_glCreateShader *) wglGetProcAddress("glCreateShader");
    glLinkProgram = (type_glLinkProgram *) wglGetProcAddress("glLinkProgram");
    glShaderSource = (type_glShaderSource *) wglGetProcAddress("glShaderSource");
    glUseProgram = (type_glUseProgram *) wglGetProcAddress("glUseProgram");
    glGetProgramInfoLog = (type_glGetProgramInfoLog *) wglGetProcAddress("glGetProgramInfoLog");
    glGetShaderInfoLog = (type_glGetShaderInfoLog *) wglGetProcAddress("glGetShaderInfoLog");
    glValidateProgram = (type_glValidateProgram *) wglGetProcAddress("glValidateProgram");
    glGetProgramiv = (type_glGetProgramiv *) wglGetProcAddress("glGetProgramiv");
    glGetUniformLocation = (type_glGetUniformLocation *) wglGetProcAddress("glGetUniformLocation");
    glUniform4fv = (type_glUniform4fv *) wglGetProcAddress("glUniform4fv");
    glUniform4i = (type_glUniform4i *) wglGetProcAddress("glUniform4i");
    glUniform4ui = (type_glUniform4ui *) wglGetProcAddress("glUniform4ui");
    glUniform1i = (type_glUniform1i *) wglGetProcAddress("glUniform1i");
    glUniform1iv = (type_glUniform1iv *) wglGetProcAddress("glUniform1iv");
    glUniform1f = (type_glUniform1f *) wglGetProcAddress("glUniform1f");
    glUniform1fv = (type_glUniform1fv *) wglGetProcAddress("glUniform1fv");
    glUniform2fv = (type_glUniform2fv *) wglGetProcAddress("glUniform2fv");
    glUniform3fv = (type_glUniform3fv *) wglGetProcAddress("glUniform3fv");
    glUniform3iv = (type_glUniform3iv *) wglGetProcAddress("glUniform3iv");
    glUniform3i = (type_glUniform3i *) wglGetProcAddress("glUniform3i");
    glUniform3f = (type_glUniform3f *) wglGetProcAddress("glUniform3f");
    glEnableVertexAttribArray = (type_glEnableVertexAttribArray *) wglGetProcAddress("glEnableVertexAttribArray");
    glDisableVertexAttribArray = (type_glDisableVertexAttribArray *) wglGetProcAddress("glDisableVertexAttribArray");
    glGetAttribLocation = (type_glGetAttribLocation *) wglGetProcAddress("glGetAttribLocation");
    glVertexAttribPointer = (type_glVertexAttribPointer *) wglGetProcAddress("glVertexAttribPointer");
    glVertexAttribIPointer = (type_glVertexAttribIPointer *) wglGetProcAddress("glVertexAttribIPointer");
    glBindVertexArray = (type_glBindVertexArray *) wglGetProcAddress("glBindVertexArray");
    glGenVertexArrays = (type_glGenVertexArrays *) wglGetProcAddress("glGenVertexArrays");
    glBindBuffer = (type_glBindBuffer *) wglGetProcAddress("glBindBuffer");
    glBindBufferBase = (type_glBindBufferBase *) wglGetProcAddress("glBindBufferBase");
    glBufferSubData = (type_glBufferSubData *) wglGetProcAddress("glBufferSubData");
    glGenBuffers = (type_glGenBuffers *) wglGetProcAddress("glGenBuffers");
    glGenRenderbuffers = (type_glGenRenderbuffers *) wglGetProcAddress("glGenRenderbuffers");
    glBindRenderbuffer = (type_glBindRenderbuffer *) wglGetProcAddress("glBindRenderbuffer");
    glRenderbufferStorage = (type_glRenderbufferStorage *) wglGetProcAddress("glRenderbufferStorage");
    glRenderbufferStorageMultisample = (type_glRenderbufferStorageMultisample *) wglGetProcAddress("glRenderbufferStorageMultisample");
    glFramebufferRenderbuffer = (type_glFramebufferRenderbuffer *) wglGetProcAddress("glFramebufferRenderbuffer");
    glBufferData = (type_glBufferData *) wglGetProcAddress("glBufferData");
    glActiveTexture = (type_glActiveTexture *) wglGetProcAddress("glActiveTexture");
    glDeleteProgram = (type_glDeleteProgram *) wglGetProcAddress("glDeleteProgram");
    glDeleteVertexArrays = (type_glDeleteVertexArrays *) wglGetProcAddress("glDeleteVertexArrays");
    glDeleteShader = (type_glDeleteShader *) wglGetProcAddress("glDeleteShader");
    glDeleteFramebuffers = (type_glDeleteFramebuffers *) wglGetProcAddress("glDeleteFramebuffers");
    glDeleteRenderbuffers = (type_glDeleteRenderbuffers *) wglGetProcAddress("glDeleteRenderbuffers");
    glDrawBuffers = (type_glDrawBuffers *) wglGetProcAddress("glDrawBuffers");
    glTexImage3D = (type_glTexImage3D *) wglGetProcAddress("glTexImage3D");
    glTexSubImage3D = (type_glTexSubImage3D *) wglGetProcAddress("glTexSubImage3D");
    glDrawElementsBaseVertex = (type_glDrawElementsBaseVertex *) wglGetProcAddress("glDrawElementsBaseVertex");
    glGenerateMipmap = (type_glGenerateMipmap *) wglGetProcAddress("glGenerateMipmap");
    glDetachShader = (type_glDetachShader *) wglGetProcAddress("glDetachShader");
    glDeleteBuffers = (type_glDeleteBuffers *) wglGetProcAddress("glDeleteBuffers");
    glUniformMatrix2fv = (type_glUniformMatrix2fv *) wglGetProcAddress("glUniformMatrix2fv");
    glUniformMatrix3fv = (type_glUniformMatrix3fv *) wglGetProcAddress("glUniformMatrix3fv");
    glUniformMatrix4fv = (type_glUniformMatrix4fv *) wglGetProcAddress("glUniformMatrix4fv");
    glGetShaderiv = (type_glGetShaderiv *) wglGetProcAddress("glGetShaderiv");
    glDrawArraysInstanced = (type_glDrawArraysInstanced *) wglGetProcAddress("glDrawArraysInstanced");
    glDrawElementsInstanced = (type_glDrawElementsInstanced *) wglGetProcAddress("glDrawElementsInstanced");
    glProgramParameteri = (type_glProgramParameteri *) wglGetProcAddress("glProgramParameteri");
    glDebugMessageCallback = (type_glDebugMessageCallback *) wglGetProcAddress("glDebugMessageCallback");
}

void opengl_destroy(Window* window)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(window->openGLRC);
    ReleaseDC(window->hwnd, window->hdc);
}

void opengl_instance_create(Window* window, int32 multisample = 0)
{
    LOG_1("Load opengl");
    gl_extensions_load();

    opengl_init_wgl();

    // @question Why do we do the GetDC here? Couldn't we do it in UtilsWindows.h
    window->hdc = GetDC(window->hwnd);
    set_pixel_format(window->hdc, multisample);

    window->openGLRC = 0;
    if (wglCreateContextAttribsARB) {
        window->openGLRC = wglCreateContextAttribsARB(window->hdc, 0, win32_opengl_attribs);
    }

    if (!window->openGLRC) {
        window->openGLRC = wglCreateContext(window->hdc);
    }

    if(!wglMakeCurrent(window->hdc, window->openGLRC)) {
        LOG_1("Couldn't load opengl");
        return;
    }

    opengl_init_gl();

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(0);
    }
    LOG_1("Loaded opengl");
}

#endif