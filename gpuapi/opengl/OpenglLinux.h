/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_GPUAPI_OPENGL_LINUX_H
#define COMS_GPUAPI_OPENGL_LINUX_H

typedef void type_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
typedef void type_glBindFramebuffer(GLenum target, GLuint framebuffer);
typedef void type_glGenFramebuffers(GLsizei n, GLuint *framebuffer);
typedef void type_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum type_glCheckFramebufferStatus(GLenum target);
typedef void type_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void type_glAttachShader(GLuint program, GLuint shader);
typedef void type_glCompileShader(GLuint shader);

// @todo continue

#endif