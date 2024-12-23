#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Shader.h"
#include "stb_image.h"

// TEMP
float vertices[] = {
	// Pos				   Col                  Tex Coord
	0.5f, 0.5f,   0.0f,    1.0f, 0.0f, 0.0f,    1.0f, 1.0f,		// Top right
	0.5f, -0.5f,  0.0f,    1.0f, 0.0f, 0.0f,	1.0f, 0.0f,		// Bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,	0.0f, 0.0f,		// Bottom left
	-0.5f, 0.5f,  0.0f,    0.0f, 0.0f, 1.0f,	0.0f, 1.0f		// Top left
};

unsigned int indices[] = {
	0, 1, 3, // First
	1, 2, 3  // Second
};

// Todo: Needs moving to it's own class
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
void loadTexture(std::string path, bool flip, unsigned int textureObject, GLenum activeTexture);

GLenum polyMode = GL_FILL;

int width, height, nChannels;
unsigned int textureOne, textureTwo;
std::string imagePath = "assets/textures/uvmap.jpg";
std::string image2Path = "assets/textures/exit.png";

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window context
	GLFWwindow* window = glfwCreateWindow(800, 600, "Glimmer", NULL, NULL);
	if (window == NULL)
	{
		printf("Failed to create window!");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyCallback);

	// Initialize OGL Context
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD!");
		return -1;
	}

	//glViewport(0, 0, 800, 600);

	Shader mainShader("shaders/shader.vert", "shaders/shader.frag");

	// Vertex Array Object, Vertex Buffer Object & Element Buffer Object 
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind VAO then bind + set vertex buffers
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Interpret vertex data, 3 * float as we have 3 * 4 byte values for position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // Pos
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // Col
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // Tex coord
	glEnableVertexAttribArray(2);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Load and bind texture
	loadTexture(imagePath, true, textureOne, GL_TEXTURE0);
	loadTexture(image2Path, true, textureTwo, GL_TEXTURE1);

	mainShader.Use();
	mainShader.SetInt("uTexture", 0);
	mainShader.SetInt("uTexture2", 1);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Input process
		processInput(window);

		// Render
		glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, polyMode);
		mainShader.Use();

		//glBindTexture(GL_TEXTURE_2D, texture);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Call events + swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deallocate resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	mainShader.Delete();

	glfwTerminate();
	return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

}

void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS)
	{
		polyMode = polyMode == GL_FILL ? GL_LINE : GL_FILL;
	}
}

// Pretty rough function, but for now it's fine
void loadTexture(std::string path, bool flip, unsigned int textureObject, GLenum activeTexture)
{
	glGenTextures(1, &textureObject);
	glActiveTexture(activeTexture);
	glBindTexture(GL_TEXTURE_2D, textureObject);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_set_flip_vertically_on_load(flip);
	unsigned char* imageData = stbi_load(path.c_str(), &width, &height, &nChannels, 4);
	if (imageData)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		printf("Failed to load texture! %s", path.c_str());
	}
	stbi_image_free(imageData);
}
