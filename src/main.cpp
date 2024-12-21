#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Todo: Needs moving to it's own class
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

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

	// Initialize OGL Context
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		printf("Failed to initialize GLAD!");
		return -1;
	}

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Input process
		processInput(window);

		// Render
		glClearColor(0.5f, 0.25f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Call events + swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

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
