#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <filesystem>
#include <mutex>
#include <cstdint>
#include <vector>

class ResourceCache;

struct TextureInfo
{
	GLuint textureID = 0;
	GLuint64 handle = 0;
	bool isResident = false;
	bool isLoading = false;
};

enum class TextureType
{
	Albedo,
	Normal,
	MetallicRoughness,
	Occlusion,
	Emissive
};

class TextureManager
{
public:
	// Target dim for texture resize
	static constexpr int targetWidth = 2048;
	static constexpr int targetHeight = 2048;
	// Max textures per array

	TextureManager();
	~TextureManager();

	uint64_t RequestTexture(const std::filesystem::path& imagePath, bool srgb, ResourceCache& cache);

	std::vector<std::string> ProcessUploadQueue();

	uint64_t GetTextureHandle(const std::string& path) const;

	uint64_t GetPlaceholderHandle() const { return mPlaceholderTexture.handle; }

	void QueueLoadedTexture(const std::string& identifier, int width, int height, int channels, unsigned char* loadedPixelData, bool srgb);

	std::unordered_map<std::string, TextureInfo> mTextureCache;

private:
	struct QueuedUpload
	{
		TextureType type{};
		std::string path{};
		bool srgb = false;
		GLuint targetArrayID = 0;
		uint32_t targetLayerIndex = 0;
		int width = 0, height = 0, nChannels = 0;
		std::unique_ptr<unsigned char[]> resizedData;
	};

	TextureInfo mPlaceholderTexture;

	std::vector<QueuedUpload> mUploadQueue;
	std::mutex mQueueMutex;
	std::vector<std::string> mCompletedLoads;

private:
	void CreatePlaceholderTexture();
	void QueueTextureLoad(const std::filesystem::path& path, bool srgb, ResourceCache& cache);
	void UploadTextureData(const QueuedUpload& upload, TextureInfo& texInfo);

	static void LoadAndResizeWorker(std::string path, bool srgb, std::vector<QueuedUpload>* queue, std::mutex* queueMtx);
};
