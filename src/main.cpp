#include <stdio.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// TEMP
float vertices[] = {
	0.5f, 0.5f, 0.0f,   // Top right
	0.5f, -0.5f, 0.0f,  // Bottom right
	-0.5f, -0.5f, 0.0f, // Bottom left
	-0.5f, 0.5f, 0.0f   // Top left
};
unsigned int indices[] = {
	0, 1, 3, // First
	1, 2, 3  // Second
};

// Will be loaded in from file eventually
const char* vertexShaderSource = "#version 460 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 460 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
"}\0";


// Todo: Needs moving to it's own class
void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
void checkShaderCompilation(unsigned int& shader, std::string shaderType); // Could probably change it so type isn't needed. 
void checkProgramCompilation(unsigned int& program);

GLenum polyMode = GL_FILL;

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

	// Create shader object for vertex shader
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);

	// Attach shader source to shader object
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	checkShaderCompilation(vertexShader, "Vertex");

	// Compile fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	checkShaderCompilation(fragmentShader, "Fragment");

	// Create shader program and link compiled shaders
	unsigned int shaderProgram = glCreateProgram();

	// Attach shaders
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	checkProgramCompilation(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Input process
		processInput(window);

		// Render
		glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glPolygonMode(GL_FRONT_AND_BACK, polyMode);
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Call events + swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deallocate resources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);

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

void checkShaderCompilation(unsigned int& shader, std::string shaderType)
{
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		printf("%s shader failed to compile! %s\n", shaderType.c_str(), infoLog);
	}
}

void checkProgramCompilation(unsigned int& program)
{
	int success;
	char infoLog[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		printf("Shader program failed! %s\n", infoLog);
	}

}