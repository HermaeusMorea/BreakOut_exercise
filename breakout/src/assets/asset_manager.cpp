#include "breakout/assets/asset_manager.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>

#include "breakout/assets/asset_paths.h"
#include "stb_image.h"

// Instantiate static variables
std::map<std::string, Texture2D>    AssetManager::Textures;
std::map<std::string, Shader>       AssetManager::Shaders;


Shader AssetManager::LoadShader(const std::string& vShaderFile, const std::string& fShaderFile, const std::string& gShaderFile, std::string name)
{
    const std::filesystem::path vertexPath = AssetPaths::Resolve(vShaderFile);
    const std::filesystem::path fragmentPath = AssetPaths::Resolve(fShaderFile);
    const std::filesystem::path geometryPath = gShaderFile.empty() ? std::filesystem::path() : AssetPaths::Resolve(gShaderFile);
    Shaders[name] = loadShaderFromFile(vertexPath, fragmentPath, gShaderFile.empty() ? nullptr : &geometryPath);
    return Shaders[name];
}

Shader AssetManager::GetShader(const std::string& name)
{
    return Shaders[name];
}

Texture2D AssetManager::LoadTexture(const std::string& file, bool alpha, std::string name)
{
    Textures[name] = loadTextureFromFile(AssetPaths::Resolve(file), alpha);
    return Textures[name];
}

Texture2D AssetManager::GetTexture(const std::string& name)
{
    return Textures[name];
}

void AssetManager::Clear()
{
    // (properly) delete all shaders	
    for (auto iter : Shaders)
        glDeleteProgram(iter.second.ID);
    // (properly) delete all textures
    for (auto iter : Textures)
        glDeleteTextures(1, &iter.second.ID);
}

Shader AssetManager::loadShaderFromFile(const std::filesystem::path& vShaderFile, const std::filesystem::path& fShaderFile, const std::filesystem::path* gShaderFile)
{
    // 1. retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    try
    {
        // open files
        std::ifstream vertexShaderFile(vShaderFile);
        std::ifstream fragmentShaderFile(fShaderFile);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vertexShaderFile.rdbuf();
        fShaderStream << fragmentShaderFile.rdbuf();
        // close file handlers
        vertexShaderFile.close();
        fragmentShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        // if geometry shader path is present, also load a geometry shader
        if (gShaderFile != nullptr)
        {
            std::ifstream geometryShaderFile(*gShaderFile);
            std::stringstream gShaderStream;
            gShaderStream << geometryShaderFile.rdbuf();
            geometryShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::exception e)
    {
        std::cout << "ERROR::SHADER: Failed to read shader files" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();
    // 2. now create shader object from source code
    Shader shader;
    shader.Compile(vShaderCode, fShaderCode, gShaderFile != nullptr ? gShaderCode : nullptr);
    return shader;
}

Texture2D AssetManager::loadTextureFromFile(const std::filesystem::path& file, bool alpha)
{
    // create texture object
    Texture2D texture;
    if (alpha)
    {
        texture.Internal_Format = GL_RGBA;
        texture.Image_Format = GL_RGBA;
    }
    // load image
    int width, height, nrChannels;
    unsigned char* data = stbi_load(file.string().c_str(), &width, &height, &nrChannels, 0);
    // now generate texture
    texture.Generate(width, height, data);
    // and finally free image data
    stbi_image_free(data);
    return texture;
}
