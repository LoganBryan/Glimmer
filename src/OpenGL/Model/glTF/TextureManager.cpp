#include "TextureManager.h"
#include <stb_image.h>

GLuint TextureManager::CreatePlaceholder()
{
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    unsigned char data[4] = { 255, 0, 255, 255 };
    glTextureStorage2D(tex, 1, GL_RGBA8, 1, 1);
    glTextureSubImage2D(tex, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

    return tex;
}

void TextureManager::AsyncLoad(GLuint textureID, const std::string& path)
{
    std::thread([textureID, path]()
        {
            int width, height, nChannels;
            unsigned char* data = stbi_load(path.c_str(), &width, &height, &nChannels, 4);
            if (!data)
            {
                std::cerr << "Failed to load texture: " << path << std::endl;
                return;
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
        }).detach();
}
