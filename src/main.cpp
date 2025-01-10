#define TINYGLTF_IMPLEMENTATION

#include <stdio.h>
#include <string>
#include <sstream>
#include <format>
#include <vector>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "tiny_gltf.h"

#include "Shader.h"
#include "Camera.h"
#include "FPSCounter.h"

#define PI 3.14159265358979323846
#define TWO_PI 6.2831855
#define BUFFER_OFFSET(i) ((char*)NULL + (i))

// TODO: General code cleanup

// TEMP
float vertices[] = {
	// Pos          // Normal           // TexCoord
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
};

float skyboxVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

float quadVertices[] = { // vertex for a quad to fill the screen
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};

glm::vec3 cubePositions[] = {
	glm::vec3(0.0f,  0.0f,  0.0f),
	glm::vec3(2.0f,  5.0f, -15.0f),
	glm::vec3(-1.5f, -2.2f, -2.5f),
	glm::vec3(-3.8f, -2.0f, -12.3f),
	glm::vec3(2.4f, -0.4f, -3.5f),
	glm::vec3(-1.7f,  3.0f, -7.5f),
	glm::vec3(1.3f, -2.0f, -2.5f),
	glm::vec3(1.5f,  2.0f, -2.5f),
	glm::vec3(1.5f,  0.2f, -1.5f),
	glm::vec3(-1.3f,  1.0f, -1.5f)
};

std::vector<glm::vec3> staticLightPositions = {
	glm::vec3(0.7f,  0.2f,  2.0f),
	glm::vec3(2.3f, -3.3f, -4.0f),
	glm::vec3(-4.0f,  2.0f, -12.0f),
	glm::vec3(0.5f,  0.8f, -3.0f)
};

std::vector<glm::vec3> lightPositions = {
	glm::vec3(0.7f,  0.2f,  2.0f),
	glm::vec3(2.3f, -3.3f, -4.0f),
	glm::vec3(-4.0f,  2.0f, -12.0f),
	glm::vec3(0.0f,  0.0f, -3.0f)
};

std::vector<std::string> skyboxFaces = {
	"assets/textures/skybox/right.jpg",
	"assets/textures/skybox/left.jpg",
	"assets/textures/skybox/top.jpg",
	"assets/textures/skybox/bottom.jpg",
	"assets/textures/skybox/front.jpg",
	"assets/textures/skybox/back.jpg"
};

// Todo: Needs moving to it's own class
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
unsigned int LoadTexture(char const* path);

// This will eventually be in the camera class v
void MouseCallback(GLFWwindow* window, double xpos, double ypos); 
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

GLenum polyMode = GL_FILL;

int width = 800;
int height = 600;
int nChannels;

unsigned int textureOne, textureTwo;
std::string imagePath = "assets/textures/boxTex.png";
std::string image2Path = "assets/textures/boxSpecular.png";
std::string image3Path = "assets/textures/boxEmission.png";

std::string testModel = "assets/models/helmet/DamagedHelmet.gltf";

bool firstMouseInput = true;
float lastMouseX = 400, lastMouseY = 400;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 90.0f;

Camera mainCamera(glm::vec3(0.0f, 0.0f, 3.0f));

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

static std::string GetFilePathExtension(const std::string& fileName)
{
	if (fileName.find_last_of(".") != std::string::npos)
	{
		return fileName.substr(fileName.find_last_of(".") + 1);
	}
	return "";
}

static void PrintNodes(const tinygltf::Scene& scene)
{
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		printf("node: %d\n\n", scene.nodes[i]);
	}
}

static void CheckError(std::string descript)
{
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
	{
		printf("OGL Error %s: %d (%d)", descript.c_str(), err, err);
		throw; // lol 
	}
}

tinygltf::Model LoadModel(std::string path)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	std::string exten = GetFilePathExtension(path);

	bool ret = false;
	if (exten.compare("glb") == 0)
	{
		// Binary
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	}
	else
	{
		// ASCII
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}

	if (!warn.empty())
	{
		printf("gLTF Warn: %s\n", warn.c_str());
	}

	if (!err.empty())
	{
		printf("gLTF Error: %s\n", err.c_str());
	}

	if (!ret)
	{
		printf("Failed to parse gLTF!\n");
	}

	printf("Success!\n Model loaded: %s\n", path.c_str());

	//PrintNodes(model.scenes[model.defaultScene > -1 ? model.defaultScene : 0]);

	return model;
}

// Todo; if it's missing some of the textures the output is black + doesn't seem like models with multiple meshes wants to work?
void BindMesh(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh)
{
	for (size_t i = 0; i < model.bufferViews.size(); i++)
	{
		const tinygltf::BufferView& bufferView = model.bufferViews[i];

		if (bufferView.target == 0)
		{
			printf("WARNING! Buffer View target is zero!\n Unsupported bufferView\n"); // spec2.0
			continue;
		}

		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
		//printf("Buffer view target, %d\n", bufferView.target);

		GLuint vbo;
		glGenBuffers(1, &vbo);
		vbos[i] = vbo;
		glBindBuffer(bufferView.target, vbo);

		//printf("Buffer size: %zu \n Buffer View offset: %zu\n", buffer.data.size(), bufferView.byteOffset);
		glBufferData(bufferView.target, bufferView.byteLength, &buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);

		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			tinygltf::Primitive primitve = mesh.primitives[i];
			tinygltf::Accessor indAccessor = model.accessors[primitve.indices];

			for (auto& attr : primitve.attributes)
			{
				tinygltf::Accessor accessor = model.accessors[attr.second];
				int bStride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);

				glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

				int size = (accessor.type == TINYGLTF_TYPE_SCALAR) ? 1 : accessor.type;

				int loc = -1;
				if (attr.first.compare("POSITION") == 0) loc = 0;
				if (attr.first.compare("NORMAL") == 0) loc = 1;
				if (attr.first.compare("TEXCOORD_0") == 0) loc = 2;
				if (attr.first.compare("TANGENT") == 0) loc = 3; // Should probably check if this exists... use mikktspace if they're not there

				if (loc > -1)
				{
					glEnableVertexAttribArray(loc);
					glVertexAttribPointer(loc, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, bStride, BUFFER_OFFSET(accessor.byteOffset));
				}
				else
				{
					printf("WARNING!\n Vertex Attribute Array missing! %s\n", attr.first.c_str());
				}
			}

			if (primitve.material >= 0 && primitve.material < model.materials.size())
			{
				const tinygltf::Material& material = model.materials[primitve.material];

				auto bindTexture = [&](int textureIndex, GLenum textureUnit, GLenum uniformLocation)
					{
						if (textureIndex >= 0 && textureIndex < model.textures.size())
						{
							const tinygltf::Texture& texture = model.textures[textureIndex];

							if (texture.source >= 0 && texture.source < model.images.size())
							{
								const tinygltf::Image& image = model.images[texture.source];
								GLuint texID;
								glGenTextures(1, &texID);

								glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
								glActiveTexture(textureUnit);
								glBindTexture(GL_TEXTURE_2D, texID);

								glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image.image[0]);

								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

								glGenerateMipmap(GL_TEXTURE_2D);

								glBindTexture(GL_TEXTURE_2D, texID);

								glActiveTexture(textureUnit);
								glUniform1i(uniformLocation, textureUnit);

							}
						}
					};

				// Albedo
				if (material.values.find("baseColorTexture") != material.values.end())
				{
					int textureIndex = material.values.at("baseColorTexture").TextureIndex();
					bindTexture(textureIndex, GL_TEXTURE0, 0);
				}

				// AO
				if (material.additionalValues.find("occlusionTexture") != material.additionalValues.end())
				{
					int textureIndex = material.additionalValues.at("occlusionTexture").TextureIndex();
					bindTexture(textureIndex, GL_TEXTURE1, 1);
				}

				// Emissive
				if (material.additionalValues.find("emissiveTexture") != material.additionalValues.end())
				{
					int textureIndex = material.additionalValues.at("emissiveTexture").TextureIndex();
					bindTexture(textureIndex, GL_TEXTURE2, 2);
				}

				// Metallic-Roughness
				if (material.values.find("metallicRoughnessTexture") != material.values.end())
				{
					int textureIndex = material.values.at("metallicRoughnessTexture").TextureIndex();
					bindTexture(textureIndex, GL_TEXTURE3, 3);
				}

				// Normal
				if (material.additionalValues.find("normalTexture") != material.additionalValues.end())
				{
					int textureIndex = material.additionalValues.at("normalTexture").TextureIndex();
					bindTexture(textureIndex, GL_TEXTURE4, 4);
				}
			}
		}
	}
}


void BindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Node& node)
{
	if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
	{
		BindMesh(vbos, model, model.meshes[node.mesh]);
	}

	for (size_t i = 0; i < node.children.size(); i++)
	{
		assert((node.children[i] >= 0) && (node.children[i] < model.nodes.size()));
		BindModelNodes(vbos, model, model.nodes[node.children[i]]);
	}
}

std::pair<GLuint, std::map<int, GLuint>> BindModel(tinygltf::Model& model)
{
	std::map<int, GLuint> vbos;
	GLuint vao;
	
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
		BindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
	}

	glBindVertexArray(0);

	return std::make_pair(vao, vbos);
}

void CleanupBuffers(std::pair<GLuint, std::map<int, GLuint>>& vertAndElementBuffers, tinygltf::Model& model)
{
	// Delete VAO
	glDeleteVertexArrays(1, &vertAndElementBuffers.first);

	// Delete VBOs
	for (auto it = vertAndElementBuffers.second.cbegin(); it != vertAndElementBuffers.second.cend();)
	{
		const tinygltf::BufferView& bufferView = model.bufferViews[it->first];

		if (bufferView.target != GL_ELEMENT_ARRAY_BUFFER)
		{
			glDeleteBuffers(1, &it->second);
			vertAndElementBuffers.second.erase(it++);
		}
		else
		{
			it++;
		}
	}

	// Clear map
	vertAndElementBuffers.second.clear();
}

void DrawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model& model, tinygltf::Mesh& mesh)
{
	for (size_t i = 0; i < mesh.primitives.size(); i++)
	{
		tinygltf::Primitive prim = mesh.primitives[i];
		tinygltf::Accessor indAccessor = model.accessors[prim.indices];

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos.at(indAccessor.bufferView));

		glDrawElements(prim.mode, indAccessor.count, indAccessor.componentType, BUFFER_OFFSET(indAccessor.byteOffset));
	}
}

void DrawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& vertAndElementBuffers, tinygltf::Model& model, tinygltf::Node& node)
{
	if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
	{
		DrawMesh(vertAndElementBuffers.second, model, model.meshes[node.mesh]);
	}

	// Recursively draw child nodes
	for (size_t i = 0; i < node.children.size(); i++)
	{
		DrawModelNodes(vertAndElementBuffers, model, model.nodes[node.children[i]]);
	}
}

void DrawModel(const std::pair<GLuint, std::map<int, GLuint>>& vertAndElementBuffers, tinygltf::Model& model)
{
	glBindVertexArray(vertAndElementBuffers.first);

	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		DrawModelNodes(vertAndElementBuffers, model, model.nodes[scene.nodes[i]]);
	}

	glBindVertexArray(0);
}

unsigned int GenerateCubemap(std::vector<std::string> faces) 
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nChannels;
	unsigned char* data;

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		data = stbi_load(faces[i].c_str(), &width, &height, &nChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			printf("Cubemap texture failed to load at %s", faces[i].c_str());
		}
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window context
	GLFWwindow* window = glfwCreateWindow(width, height, "Glimmer", NULL, NULL);
	if (window == NULL)
	{
		printf("Failed to create window!");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

	// Initialize OGL Context
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD!");
		return -1;
	}

	//glViewport(0, 0, 800, 600);
	std::cout << "ver: " << glGetString(GL_VERSION) << std::endl;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	// Setup shaders
	Shader mainShader("shaders/shader.vert", "shaders/shader.frag");
	Shader lightSourceShader("shaders/lightFullBright.vert", "shaders/lightFullBright.frag");
	Shader outlineShader("shaders/shader.vert", "shaders/singleColor.frag");
	Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");
	Shader framebufferShader("shaders/framebuffer.vert", "shaders/framebuffer.frag");

	framebufferShader.Use();
	framebufferShader.SetInt("screenTexture", 5);
	// Setup framebuffer object
	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Create texture attachment
	unsigned int textureCBuffer;
	glGenTextures(1, &textureCBuffer);
	glBindTexture(GL_TEXTURE_2D, textureCBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureCBuffer, 0); // Attach
	//glBindTexture(GL_TEXTURE_2D, 0);

	// Setup renderbuffer object attachment
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // Attach
	//glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("Framebuffer %d is incomplete!\n", fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Screen quad
	unsigned int screenQuadVAO, screenQuadVBO;
	glGenVertexArrays(1, &screenQuadVAO);
	glGenBuffers(1, &screenQuadVBO);
	glBindVertexArray(screenQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// Object for light source
	//unsigned int VBO;
	//glGenBuffers(1, &VBO);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//unsigned int lightVAO;
	//glGenVertexArrays(1, &lightVAO);
	//glBindVertexArray(lightVAO);

	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	//glEnableVertexAttribArray(0);

	//// Unbind
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);

	// Load skybox textures
	unsigned int skyboxTexture = GenerateCubemap(skyboxFaces);

	// Skybox object
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	skyboxShader.Use();
	skyboxShader.SetInt("skybox", 0);

	// Load gLTF model
	mainShader.Use();

	mainShader.SetFloat("reflectionStrength", 1.0f);
	mainShader.SetVec3("inDiffuseColor", 0.5f, 0.2f, 0.0f);
	mainShader.SetFloat("specularPower", 32.0f);
	mainShader.SetFloat("gamma", true);

	tinygltf::Model exModel = LoadModel(testModel);
	auto vertElementbuffers = BindModel(exModel);

	printf("Number of meshes: %zu\n", exModel.meshes.size());
	for (size_t i = 0; i < exModel.meshes.size(); ++i)
	{
		printf("Mesh %zu: %s\n", i, exModel.meshes[i].name.c_str());
	}

	FPSCounter fpsCounter;
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_STENCIL_TEST);

	glDisable(GL_BLEND);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Input process
		ProcessInput(window);
		mainCamera.HandleCameraInput(window);

		fpsCounter.Update();
		std::stringstream windowTitle;
		windowTitle << "Glimmer [ " << fpsCounter.GetFPS() << " fps ]";
		glfwSetWindowTitle(window, windowTitle.str().c_str());

		// Render
		glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glClearColor(0.25f, 0.25f, 0.8f, 1.0f);

		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		glPolygonMode(GL_FRONT_AND_BACK, polyMode);

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
		glm::mat4 view = mainCamera.Update();
		glm::mat4 model = glm::mat4(1.0f);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Draw to framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		// gLTF object
		mainShader.Use();
		glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments pass stencil test
		glStencilMask(0xFF); // Enable writing to stencil buffer

		mainShader.SetMatrix4("projection", projection);
		mainShader.SetMatrix4("view", view);

		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		mainShader.SetMatrix4("model", model);

		DrawModel(vertElementbuffers, exModel);

		//// gLTF object -- Scaled
		//glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		//glStencilMask(0x00);
		//glDisable(GL_DEPTH_TEST);

		//outlineShader.Use();

		//outlineShader.SetMatrix4("projection", projection);
		//outlineShader.SetMatrix4("view", view);

		////glm::mat4 model = glm::mat4(1.0f);
		//model = glm::scale(model, glm::vec3(1.1, 1.1, 1.1));
		//outlineShader.SetMatrix4("model", model);

		//DrawModel(vertElementbuffers, exModel);

		//glStencilMask(0xFF);
		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
		//glEnable(GL_DEPTH_TEST);

		// Skybox - Drawn last
		glDepthFunc(GL_LEQUAL);
		skyboxShader.Use();
		view = glm::mat4(glm::mat3(mainCamera.GetViewMatrix()));

		skyboxShader.SetMatrix4("view", view);
		skyboxShader.SetMatrix4("projection", projection);

		// Draw skybox cube
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // Revert to default framebuffer

		framebufferShader.Use();
		glBindVertexArray(screenQuadVAO);
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, textureCBuffer);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Light object
		//lightSourceShader.Use();
		//lightSourceShader.SetMatrix4("projection", projection);
		//lightSourceShader.SetMatrix4("view", view);

		//glBindVertexArray(lightVAO);

		////TODO: Should eventually be changed to allow adding/ removing pointlights, but this should be simple to change		

		//model = glm::translate(model, lightPositions[0]);
		//model = glm::scale(model, glm::vec3(0.3f));
		//lightSourceShader.SetMatrix4("model", model);

		//lightPositions[0].x = sin((float)glfwGetTime() + 6) * staticLightPositions[0].x * 2.5;
		//lightPositions[0].y = sin((float)glfwGetTime() + 4) * staticLightPositions[0].y * 6;
		//lightPositions[0].z = sin((float)glfwGetTime() + 2);

		//glDrawArrays(GL_TRIANGLES, 0, 36);

		// Call events + swap buffers
		glfwSwapInterval(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deallocate
	glDeleteVertexArrays(1, &vertElementbuffers.first);
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteVertexArrays(1, &screenQuadVAO);
	glDeleteBuffers(1, &skyboxVBO);
	glDeleteBuffers(1, &screenQuadVBO);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteFramebuffers(1, &fbo);
	//glDeleteVertexArrays(1, &lightVAO);
	//glDeleteBuffers(1, &VBO);

	CleanupBuffers(vertElementbuffers, exModel);
	mainShader.Delete();

	glfwTerminate();
	return 0;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void ProcessInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

}

void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
		polyMode = polyMode == GL_FILL ? GL_LINE : GL_FILL;
	}
}

// Pretty rough function, but for now it's fine
unsigned int LoadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nComponents, 0);

	if (data)
	{
		GLenum format{};
		if (nComponents == 1)
		{
			format = GL_RED;
		}
		else if (nComponents == 3)
		{
			format = GL_RGB;
		}
		else if (nComponents == 4)
		{
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		printf("Failed to load texture at path: %s!", path);
		stbi_image_free(data);
	}

	return textureID;
}

void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouseInput)
	{
		lastMouseX = xpos;
		lastMouseY = ypos;
		firstMouseInput = false;
	}

	float xOffset = xpos - lastMouseX;
	float yOffset = lastMouseY - ypos;

	lastMouseX = xpos;
	lastMouseY = ypos;

	const float speed = 0.1f;
	xOffset *= speed;
	yOffset *= speed;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw) * cos(glm::radians(pitch)));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	
	mainCamera.SetFront(glm::normalize(direction));
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 90.0f)
		fov = 90.0f;
}

void MessageCallback(GLenum source, GLenum type, GLenum id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::cerr << "OGL Debug\n";
	printf("Source: 0x%x | Type: 0x%x | ID: %u\n Severity: 0x%x\n%s\n\n", source, type, id, severity, message);
}
