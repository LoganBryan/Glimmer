#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <sstream>
#include <format>
#include <vector>

#include "Shader.h"
#include "Camera.h"
#include "FPSCounter.h"
#include "stb_image.h"

#define PI 3.14159265358979323846
#define TWO_PI 6.2831855

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

bool firstMouseInput = true;
float lastMouseX = 400, lastMouseY = 400;
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 90.0f;

Camera mainCamera(glm::vec3(0.0f, 0.0f, 3.0f));

glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int main()
{
	glfwInit();
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

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(MessageCallback, nullptr);

	Shader mainShader("shaders/light.vert", "shaders/multiLight.frag");
	Shader lightSourceShader("shaders/lightFullBright.vert", "shaders/lightFullBright.frag");

	// Vertex Array Object, Vertex Buffer Object
	unsigned int objectVAO, VBO;
	glGenVertexArrays(1, &objectVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(objectVAO);

	// Interpret vertex data, 3 * float as we have 3 * 4 byte values for position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // Pos
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // normal
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // tex
	glEnableVertexAttribArray(2);

	// Object for light source
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Load textures
	unsigned int mainDiffTexture = LoadTexture(imagePath.c_str());
	unsigned int mainSpecularTexture = LoadTexture(image2Path.c_str());
	unsigned int mainEmissionTexture = LoadTexture(image3Path.c_str());
	
	mainShader.Use();
	mainShader.SetInt("material.diffuse", 0);
	mainShader.SetInt("material.specular", 1);
	mainShader.SetInt("material.emission", 2);
	mainShader.SetFloat("material.shininess", 64.0f);
	mainShader.SetFloat("material.emissionShift", 0.0f);

	FPSCounter fpsCounter;
	glEnable(GL_DEPTH_TEST);

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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, polyMode);

		mainShader.Use();
		mainShader.SetVec3("viewPos", mainCamera.cameraPos);

		// Directional Light
		mainShader.SetVec3("directionalLight.direction", -0.2f, -1.0f, -0.3f);
		mainShader.SetVec3("directionalLight.ambient", 0.05f, 0.05f, 0.05f);
		mainShader.SetVec3("directionalLight.diffuse", 0.4f, 0.4f, 0.4f);
		mainShader.SetVec3("directionalLight.specular", 0.5f, 0.5f, 0.5f);

		// Point Lights
		for (int i = 0; i < lightPositions.size(); i++)
		{
			auto glslLight = std::format("pointLights[{}].", i);
			std::string out = glslLight + "position";
			mainShader.SetVec3(glslLight + "position", lightPositions[i]);
			mainShader.SetVec3(glslLight + "ambient", 0.05f, 0.05f, 0.05f);
			mainShader.SetVec3(glslLight + "diffuse", 0.8f, 0.8f, 0.8f);
			mainShader.SetVec3(glslLight + "specular", 1.0f, 1.0f, 1.0f);

			// https://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation
			mainShader.SetFloat(glslLight + "constant", 1.0f);
			mainShader.SetFloat(glslLight + "linear", 0.35f);
			mainShader.SetFloat(glslLight + "quadratic", 0.44f);
		}

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
		glm::mat4 view = mainCamera.GetViewMatrix();
		view = mainCamera.Update();
		mainShader.SetMatrix4("projection", projection);
		mainShader.SetMatrix4("view", view);
		
		// bind diffuse tex
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mainDiffTexture);
		// bind specular tex
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mainSpecularTexture);
		// bind emission tex
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mainEmissionTexture);

		const float shift = fmodf(glfwGetTime(), TWO_PI);
		mainShader.SetFloat("material.emissionShift", shift);

		// render cube objects
		glBindVertexArray(objectVAO);
		for (unsigned int i = 0; i < 10; i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, cubePositions[i]);
			float angle = 20.0f * i;
			//model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
			mainShader.SetMatrix4("model", model);
			
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// render light object
		lightSourceShader.Use();
		lightSourceShader.SetMatrix4("projection", projection);
		lightSourceShader.SetMatrix4("view", view);

		glBindVertexArray(lightVAO);

		//TODO: Should eventually be changed to allow adding/ removing pointlights, right now the shader has it's own value for how many lights it has.. but this should be simple to change
		for (unsigned int i = 0; i < lightPositions.size(); i++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, lightPositions[i]);
			model = glm::scale(model, glm::vec3(0.3f));
			lightSourceShader.SetMatrix4("model", model);

			lightPositions[i].x = sin((float)glfwGetTime() + 6) * staticLightPositions[i].x * 1.5;
			lightPositions[i].y = sin((float)glfwGetTime() + 4) * staticLightPositions[i].y * 1.5;
			lightPositions[i].z = sin((float)glfwGetTime() + 2) * staticLightPositions[i].z * 1.5;
			//lightPositions[i].y += sin((float)glfwGetTime() + 4) * 5;
			//lightPositions[i].z = sin((float)glfwGetTime() + 2) * 5;

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// Call events + swap buffers
		glfwSwapInterval(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deallocate resources
	glDeleteVertexArrays(1, &objectVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);
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
