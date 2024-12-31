#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <sstream>

#include "Shader.h"
#include "Camera.h"
#include "FPSCounter.h"
#include "stb_image.h"

// TEMP
float vertices[] = {
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
	-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
	-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

// Todo: Needs moving to it's own class
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
void LoadTexture(std::string path, bool flip, unsigned int textureObject, GLenum activeTexture);

// This will eventually be in the camera class v
void MouseCallback(GLFWwindow* window, double xpos, double ypos); 
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

GLenum polyMode = GL_FILL;

int width = 800;
int height = 600;
int nChannels;

unsigned int textureOne, textureTwo;
std::string imagePath = "assets/textures/uvmap.jpg";
std::string image2Path = "assets/textures/exit.png";

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

	Shader mainShader("shaders/light.vert", "shaders/light.frag");
	Shader lightSourceShader("shaders/lightFullBright.vert", "shaders/lightFullBright.frag");

	// Vertex Array Object, Vertex Buffer Object
	unsigned int objectVAO, VBO;
	glGenVertexArrays(1, &objectVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(objectVAO);

	// Interpret vertex data, 3 * float as we have 3 * 4 byte values for position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // Pos
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // normal
	glEnableVertexAttribArray(1);

	// Object for light source
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))); // Tex Coord
	//glEnableVertexAttribArray(1);

		//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // Tex coord
		//glEnableVertexAttribArray(2);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Load and bind texture
		//LoadTexture(imagePath, true, textureOne, GL_TEXTURE0);
		//LoadTexture(image2Path, true, textureTwo, GL_TEXTURE1);

	mainShader.Use();
		//mainShader.SetInt("uTexture", 0);
		//mainShader.SetInt("uTexture2", 1);
	mainShader.SetVec3("objectColor", { 0.0f, 0.5f, 0.8f });
	mainShader.SetVec3("lightColor", { 1.0f, 1.0f, 1.0f });

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

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);
		glm::mat4 view = mainCamera.GetViewMatrix();
		view = mainCamera.Update();
		mainShader.SetMatrix4("projection", projection);
		mainShader.SetMatrix4("view", view);

		glm::mat4 model = glm::mat4(1.0f);
		mainShader.SetMatrix4("model", model);
		mainShader.SetVec3("objectColor", { sin((float)glfwGetTime() + 6), sin((float)glfwGetTime() + 4), sin((float)glfwGetTime() + 2)});
		mainShader.SetVec3("lightPos", lightPos);
		mainShader.SetVec3("viewPos", mainCamera.GetCameraPosition());

		// render cube object
		glBindVertexArray(objectVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// render light object
		lightSourceShader.Use();
		lightSourceShader.SetMatrix4("projection", projection);
		lightSourceShader.SetMatrix4("view", view);

		model = glm::mat4(1.0f);
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(0.3f));
		lightSourceShader.SetMatrix4("model", model);

		lightPos = glm::vec3(sin((float)glfwGetTime() + 6) * 2, sin((float)glfwGetTime() + 4) * 1.5, sin((float)glfwGetTime() + 2));

		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);


		////model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));
		//model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.5f, 1.0f, 0.0f));
		////view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
		//view = mainCamera.Update();
		//projection = glm::perspective(glm::radians(fov), (float)width / (float)height, 0.1f, 100.0f);

		//mainShader.SetMatrix4("model", model);
		//glUniformMatrix4fv(glGetUniformLocation(mainShader.GetID(), "view"), 1, GL_FALSE, &view[0][0]);
		//mainShader.SetMatrix4("projection", projection);

		//// SRT
		//glm::mat4 trans = glm::mat4(1.0f);
		//trans = glm::scale(trans, glm::vec3(1.0f, 1.0f, 1.0f));
		//trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
		//trans = glm::translate(trans, cos((float)glfwGetTime()) * glm::vec3(0.5f, 0.5f, 0.0f));
		//mainShader.SetMatrix4("transform", trans);

		////glBindTexture(GL_TEXTURE_2D, texture);
		//glBindVertexArray(lVAO);
		////glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		//glDrawArrays(GL_TRIANGLES, 0, 36);

		// Call events + swap buffers
		//glfwSwapInterval(0);
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
void LoadTexture(std::string path, bool flip, unsigned int textureObject, GLenum activeTexture)
{
	glGenTextures(1, &textureObject);
	glActiveTexture(activeTexture);
	glBindTexture(GL_TEXTURE_2D, textureObject);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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
