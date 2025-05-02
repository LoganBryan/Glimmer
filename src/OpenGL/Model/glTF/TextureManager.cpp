#include "TextureManager.h"
#include <stb_image.h>
#include <stb_image_resize.h>

TextureManager::TextureManager()
{
	CreatePlaceholderTexture();
}

TextureManager::~TextureManager()
{
	// Unload texture (make them non-resident) and delete
	for (auto& pair : mTextureCache)
	{
		if (pair.second.handle != 0 && pair.second.isResident)
			glMakeTextureHandleNonResidentARB(pair.second.handle);
		if (pair.second.textureID != 0)
			glDeleteTextures(1, &pair.second.textureID);
	}

	// Cleanup placeholders
	if (mPlaceholderTexture.handle != 0 && mPlaceholderTexture.isResident)
		glMakeTextureHandleNonResidentARB(mPlaceholderTexture.handle);
	if (mPlaceholderTexture.textureID != 0)
		glDeleteTextures(1, &mPlaceholderTexture.textureID);
}

uint64_t TextureManager::RequestTexture(const std::filesystem::path& imagePath, bool srgb, ResourceCache& cache)
{
	std::string key = imagePath.generic_string();

	auto it = mTextureCache.find(key);
	if (it != mTextureCache.end())
	{
		if (it->second.handle != 0)
			return it->second.handle; // It's already loaded and has a handle
		else if (it->second.isLoading)
			return mPlaceholderTexture.handle; // Currently being loaded
		else
		{
			std::cerr << "[TextureManager] Warning! Entry for texture exists but there is no handle/ loading state for " << key << "\n";
			return mPlaceholderTexture.handle;
		}
	}
	else
	{
		std::cout << "[TextureManager] Info! Queuing texture load for " << key << "\n";
		mTextureCache[key] = TextureInfo{ .isLoading = true };
		QueueTextureLoad(imagePath, srgb, cache);
		return mPlaceholderTexture.handle;
	}
}

std::vector<std::string> TextureManager::ProcessUploadQueue()
{
	std::vector<QueuedUpload> toProcess;
	mCompletedLoads.clear();

	{
		std::lock_guard<std::mutex> lock(mQueueMutex);
		if (mUploadQueue.empty()) return mCompletedLoads;
		toProcess.swap(mUploadQueue);
	}

	if (toProcess.empty()) return mCompletedLoads;

	for (const auto& upload : toProcess)
	{
		auto it = mTextureCache.find(upload.path);
		if (it == mTextureCache.end())
		{
			std::cerr << "[TextureManager] Error! Upload completed for an unknown texture " << upload.path << "\n";
			//if (upload.resizedData)
			//	free(upload.resizedData.get());
			continue;
		}

		if (!upload.resizedData)
		{
			std::cerr << "[TextureManager] Error! Upload queue item has no data for " << upload.path << "\n";
			it->second.isLoading = false;
			continue;
		}

		UploadTextureData(upload, it->second);

		if (it->second.handle != 0)
			mCompletedLoads.push_back(upload.path);
		else
			it->second.isLoading = false;

		//free(upload.resizedData.get());
	}
	return mCompletedLoads;
}

uint64_t TextureManager::GetTextureHandle(const std::string& path) const
{
	auto it = mTextureCache.find(path);
	if (it != mTextureCache.end() && it->second.handle != 0)
		return it->second.handle;

	return mPlaceholderTexture.handle;
}

void TextureManager::QueueLoadedTexture(const std::string& identifier, int width, int height, int channels, unsigned char* loadedPixelData, bool srgb)
{
	size_t bufferSize = static_cast<size_t>(targetWidth) * targetHeight * 4;
	std::unique_ptr<unsigned char[]> resizedImagePtr;
	
	try
	{
		resizedImagePtr = std::make_unique<unsigned char[]>(bufferSize);
	}
	catch (const std::bad_alloc& e)
	{
		std::cerr << "[TextureManager] Error! QueueLoadedTexture failed to allocate memory for resize. \nReason: " << e.what() << "\n";
		stbi_image_free(loadedPixelData);

		auto it = mTextureCache.find(identifier);
		if (it != mTextureCache.end())
			it->second.isLoading = false;
		return;
	}

	int flags = 0;
	if (srgb)
		flags |= STBIR_FLAG_ALPHA_PREMULTIPLIED;

	int success = stbir_resize_uint8_generic(loadedPixelData, width, height, 0,
		resizedImagePtr.get(), targetWidth, targetHeight, 0,
		4, 3, flags, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, (srgb ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR), nullptr);
	stbi_image_free(loadedPixelData);

	if (!success)
	{
		std::cerr << "[TextureManager] Error! QueueLoadedTexture failed to resize texture: " << identifier << "\n";

		auto it = mTextureCache.find(identifier);
		if (it != mTextureCache.end())
			it->second.isLoading = false;
		return;
 	}

	QueuedUpload out;
	out.path = identifier;
	out.srgb = srgb;
	out.width;
	out.height;
	out.nChannels = channels;
	out.resizedData = std::move(resizedImagePtr);

	{
		std::lock_guard<std::mutex> lock(mQueueMutex);
		mUploadQueue.push_back(std::move(out));
	}
}

void TextureManager::CreatePlaceholderTexture()
{
	glCreateTextures(GL_TEXTURE_2D, 1, &mPlaceholderTexture.textureID);
	const GLenum internalForm = GL_RGBA8;
	const GLsizei mipLevel = 1;
	const int width = 1, height = 1;

	glTextureStorage2D(mPlaceholderTexture.textureID, mipLevel, internalForm, width, height);
	unsigned char colorData[4] = {255, 255, 255, 255};
	glTextureSubImage2D(mPlaceholderTexture.textureID, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, colorData);

	glTextureParameteri(mPlaceholderTexture.textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(mPlaceholderTexture.textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(mPlaceholderTexture.textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(mPlaceholderTexture.textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	mPlaceholderTexture.handle = glGetTextureHandleARB(mPlaceholderTexture.textureID);
	if (mPlaceholderTexture.handle == 0)
	{
		std::cerr << "[TextureManager] Error! Failed to get placeholder texture handle! \n";
		return;
	}
	glMakeTextureHandleResidentARB(mPlaceholderTexture.handle);
	mPlaceholderTexture.isResident = true;
}

void TextureManager::QueueTextureLoad(const std::filesystem::path& path, bool srgb, ResourceCache& cache)
{
	if (!std::filesystem::exists(path))
	{
		std::cerr << "[TextureManager] Error! Texture file not found at " << path << "\n";

		auto it = mTextureCache.find(path.generic_string());
		if (it != mTextureCache.end())
			it->second.isLoading = false;

		return;
	}

	// Launch worker
	std::thread(LoadAndResizeWorker, path.string(), srgb, &mUploadQueue, &mQueueMutex).detach();
}

void TextureManager::UploadTextureData(const QueuedUpload& upload, TextureInfo& texInfo)
{
	GLuint textureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

	GLenum internalForm = upload.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	GLenum format = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;
	GLsizei mipLevels = static_cast<GLsizei>(1 + floor(log2(std::max(targetWidth, targetHeight))));

	glTextureStorage2D(textureID, mipLevels, internalForm, targetWidth, targetHeight);

	glTextureSubImage2D(textureID, 0, 0, 0, targetWidth, targetHeight, format, type, upload.resizedData.get());

	glGenerateTextureMipmap(textureID);

	glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
	glTextureParameterf(textureID, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

	GLuint64 handle = glGetTextureHandleARB(textureID);
	if (handle == 0)
	{
		std::cerr << "[TextureManager] Error! Failed to get texture handle for " << upload.path << "\n";
		glDeleteTextures(1, &textureID);
		texInfo.isLoading = false;
		texInfo.handle = 0;
		texInfo.textureID = 0;
		return;
	}

	glMakeTextureHandleResidentARB(handle);

	// Upload cache entry
	texInfo.textureID = textureID;
	texInfo.handle = handle;
	texInfo.isResident = true;
	texInfo.isLoading = false;
}

void TextureManager::LoadAndResizeWorker(std::string path, bool srgb, std::vector<QueuedUpload>* queue, std::mutex* queueMtx)
{
	int width = 0, height = 0, nChannels = 0;
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 4);

	if (!data)
	{
		std::cerr << "[TextureManager] Error! Worker failed to load texture at " << path << " \n with reason: " << stbi_failure_reason() << "\n";
		return;
	}
	
	size_t bufferSize = static_cast<size_t>(targetWidth) * targetHeight * 4;
	std::unique_ptr<unsigned char[]> resizedImagePtr;
	try
	{
		resizedImagePtr = std::make_unique<unsigned char[]>(bufferSize);
	}
	catch (const std::bad_alloc& e)
	{
		std::cerr << "[TextureManager] Error! Worker failed to allocate memory " << e.what() << "\n";
		stbi_image_free(data);
		return;
	}

	// Determine sRGB state for resize
	int flags = 0;
	if (srgb)
	{
		flags |= STBIR_FLAG_ALPHA_PREMULTIPLIED;
	}

	int success = stbir_resize_uint8_generic(data, width, height, 0, 
		resizedImagePtr.get(), targetWidth, targetHeight, 0,
		4, 3, flags, 
		STBIR_EDGE_CLAMP, 
		STBIR_FILTER_DEFAULT, 
		(srgb ? STBIR_COLORSPACE_SRGB : STBIR_COLORSPACE_LINEAR), 
		nullptr);
	
	stbi_image_free(data);

	if (!success)
	{
		std::cerr << "[TextureManager] Error! Worker failed to resize texture at " << path << "\n";
		return;
	}

	QueuedUpload out;
	out.path = path;
	out.srgb = srgb;
	out.width = width;
	out.height = height;
	out.nChannels = nChannels;
	out.resizedData = std::move(resizedImagePtr);

	{
		std::lock_guard<std::mutex> lock(*queueMtx);
		queue->push_back(std::move(out));
	}
}
