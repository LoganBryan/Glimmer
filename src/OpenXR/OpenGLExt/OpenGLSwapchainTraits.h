#pragma once
#include <vector>

#define XR_USE_PLATFORM_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <GL/GL.h>
#include <GL/glext.h>
#include <GL/wglext.h>

struct OpenGLSwapchainTraits
{
	PROC GetExtension(const char* functionName) { return wglGetProcAddress(functionName); }
	typedef void (*PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(GLenum target, GLenum attachment, GLuint texture, GLint level,
		GLint baseViewIndex, GLsizei numViews);
	PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)GetExtension("glFramebufferTextureMultiviewOVR");


	struct ImageViewCreateInfo {
		void* image;
		enum class Type : uint8_t {
			RTV,
			DSV,
			SRV,
			UAV
		} type;
		enum class View : uint8_t {
			TYPE_1D,
			TYPE_2D,
			TYPE_3D,
			TYPE_CUBE,
			TYPE_1D_ARRAY,
			TYPE_2D_ARRAY,
			TYPE_CUBE_ARRAY,
		} view;
		int64_t format;
		enum class Aspect : uint8_t {
			COLOR_BIT = 0x01,
			DEPTH_BIT = 0x02,
			STENCIL_BIT = 0x04
		} aspect;
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	inline static const std::vector<int64_t> GetSupportedColorSwapchainFormats()
	{
		// https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/f122f9f1fc729e2dc82e12c3ce73efa875182854/src/tests/hello_xr/graphicsplugin_opengl.cpp#L229-L236
		return {
			GL_RGB10_A2,
			GL_RGBA16F,
			// The two below should only be used as a fallback, as they are linear color formats without enough bits for color
			// depth, thus leading to banding.
			GL_RGBA8,
			GL_RGBA8_SNORM,
		};
	}

	inline static const std::vector<int64_t> GetSupportedDepthSwapchainFormats()
	{
		return {
			GL_DEPTH_COMPONENT32F,
			GL_DEPTH_COMPONENT32,
			GL_DEPTH_COMPONENT24,
			GL_DEPTH_COMPONENT16 };
	}

	inline static void DestroyImageViews(void*& imageView, std::unordered_map<GLuint, ImageViewCreateInfo>& imageViews) {
		GLuint framebuffer = (GLuint)(uint64_t)imageView;
		imageViews.erase(framebuffer);
		glDeleteFramebuffers(1, &framebuffer);
		imageView = nullptr;
	}

	inline void* CreateImageView(const ImageViewCreateInfo& imageViewCI, std::unordered_map<GLuint, ImageViewCreateInfo>& imageViews) {
		GLuint framebuffer = 0;
		glGenFramebuffers(1, &framebuffer);

		GLenum attachment = imageViewCI.aspect == ImageViewCreateInfo::Aspect::COLOR_BIT ? GL_COLOR_ATTACHMENT0 : GL_DEPTH_ATTACHMENT;

		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D_ARRAY) {
			glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, attachment, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel, imageViewCI.baseArrayLayer, imageViewCI.layerCount);
		}
		else if (imageViewCI.view == ImageViewCreateInfo::View::TYPE_2D) {
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, (GLuint)(uint64_t)imageViewCI.image, imageViewCI.baseMipLevel);
		}
		else {
			DEBUG_BREAK;
			std::cout << "ERROR: OPENGL: Unknown ImageView View type." << std::endl;
		}

		GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (result != GL_FRAMEBUFFER_COMPLETE) {
			DEBUG_BREAK;
			std::cout << "ERROR: OPENGL: Framebuffer is not complete." << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		imageViews[framebuffer] = imageViewCI;
		return (void*)(uint64_t)framebuffer;
	}

};