// main.cpp : Defines the entry point for the console application.

#include <iostream>

#include <glad\glad.h> //IMPORTANT: glad needs to the be included BEFORE glfw, throws errors otherwise!!!
#include <GLFW\glfw3.h>

#define STB_IMAGE_IMPLEMENTATION //IMPORTANT: has to be done once in the project BEFORE including std_image.h
#include "stb_image.h"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

//functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height); //register a callback function when the window gets resized
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* filepath);

//screen settings
unsigned int screenWidth = 800;
unsigned int screenHeight = 600;

//time
float deltaTime = 0.0f; //time between the current and the last frame
float lastFrameTime = 0.0f; //time of the last frame

//camera setup
Camera* camera = new Camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastMouseX = screenWidth / 2.0f;
float lastMouseY = screenHeight / 2.0f;
bool firstMouse = true;

int main() {
	//initialize GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//create a window object
	GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Minor Skilled Rendering", NULL, NULL);

	if(window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//register callbacks
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //register the window resize callback function															
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//set input mode
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); //"captures" the cursor and makes it invisible

	//initialize GLAD
	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST); //enable depth test (z-buffer/depth buffer)
	glDepthFunc(GL_LESS);

	glEnable(GL_STENCIL_TEST); //enable stencil test (stencil buffer)
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //enable wireframe

	//create shader programs
	Shader* lightingShader = new Shader("assets/shader/lit.vs", "assets/shader/lit.fs");
	Shader* outlineShader = new Shader("assets/shader/outline.vs", "assets/shader/outline.fs");

	//load model
	//Model* model = new Model("assets/objects/nanosuit/nanosuit.obj");

	float cubeVertices[] = {
		// positions          // texture Coords
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	float planeVertices[] = {
		// positions          // texture Coords (note we set these higher than 1 (together with GL_REPEAT as texture wrapping mode). this will cause the floor texture to repeat)
		5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f,  5.0f,  0.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,

		5.0f, -0.5f,  5.0f,  2.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,  0.0f, 2.0f,
		5.0f, -0.5f, -5.0f,  2.0f, 2.0f
	};

	//cube VAO
	unsigned int cubeVAO, cubeVBO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);

	//plane VAO
	unsigned int planeVAO, planeVBO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);

	// load textures
	// -------------
	unsigned int cubeTexture = loadTexture("assets/textures/container2.png");
	unsigned int floorTexture = loadTexture("assets/textures/container.jpg");

	// shader configuration
	// --------------------
	lightingShader->use();
	lightingShader->setInt("material.textureDiffuse1", 0);

	//render loop
	while(!glfwWindowShouldClose(window)) {
		//update time
		float currentFrameTime = (float)glfwGetTime();
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//input
		processInput(window);

		//rendering
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//mvp matrix
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		glm::mat4 viewMatrix = camera->getViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

		//set uniforms
		outlineShader->use();
		outlineShader->setMat4("viewMatrix", viewMatrix);
		outlineShader->setMat4("projectionMatrix", projectionMatrix);

		lightingShader->use();
		lightingShader->setVec3("cameraPos", camera->position);

		//material shininess, diffuse and specular maps have been bound already
		lightingShader->setFloat("material.shininess", 32.0f);

		//directional light
		lightingShader->setVec3("dirLight.direction", -0.2f, -0.5f, -0.7f);
		lightingShader->setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
		lightingShader->setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		lightingShader->setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

		lightingShader->setMat4("viewMatrix", viewMatrix);
		lightingShader->setMat4("projectionMatrix", projectionMatrix);
		//lightingShader->setMat4("modelMatrix", modelMatrix);
		
		//draw floor as normal, but don't write the floor to the stencil buffer, we only care about the containers. We set its mask to 0x00 to not write to the stencil buffer.
		glStencilMask(0x00);
		//floor
		glBindVertexArray(planeVAO);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		lightingShader->setMat4("modelMatrix", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		//st. render pass, draw objects as normal, writing to the stencil buffer
		//--------------------------------------------------------------------
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);

		//cubes
		glBindVertexArray(cubeVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, 0.0f, -1.0f));
		lightingShader->setMat4("modelMatrix", modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		modelMatrix = glm::mat4();
		modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.0f, 0.0f));
		lightingShader->setMat4("model", modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		//2nd. render pass: now draw slightly scaled versions of the objects, this time disabling stencil writing.
		//Because the stencil buffer is now filled with several 1s. The parts of the buffer that are 1 are not drawn, thus only drawing 
		//the objects' size differences, making it look like borders.
		//-----------------------------------------------------------------------------------------------------------------------------
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilMask(0x00);
		glDisable(GL_DEPTH_TEST);
		outlineShader->use();
		float scale = 1.1;

		//cubes
		glBindVertexArray(cubeVAO);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-1.0f, 0.0f, -1.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, scale));
		outlineShader->setMat4("modelMatrix", modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		modelMatrix = glm::mat4();
		modelMatrix = glm::translate(modelMatrix, glm::vec3(2.0f, 0.0f, 0.0f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(scale, scale, scale));
		outlineShader->setMat4("modelMatrix", modelMatrix);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);
		glStencilMask(0xFF);
		glEnable(GL_DEPTH_TEST);

		//check and call events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	//cleanup resources
	glfwTerminate();
	delete lightingShader;
	delete outlineShader;
	//delete model;
	delete camera;

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	//update screen height und width
	screenWidth = width;
	screenHeight = height;
	glViewport(0, 0, width, height); //change the viewport everytime the window is resized
}

void mouse_callback(GLFWwindow* window, double xPos, double yPos) {
	if(firstMouse) {
		//initialize
		lastMouseX = xPos;
		lastMouseY = yPos;
		firstMouse = false;
	}

	//calculate offset movement between the last and the current frame
	float xOffset = xPos - lastMouseX;
	float yOffset = lastMouseY - yPos; //reversed since y-coordinates range from bottom to top

	lastMouseX = xPos;
	lastMouseY = yPos;

	camera->processMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
	camera->processMouseScroll((float)yOffset);
}

void processInput(GLFWwindow* window) {
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true); //tell the window to close when pressign escape

	float cameraSpeed = 2.5f * deltaTime;

	//camera movement input
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera->processKeyboard(FORWARD, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera->processKeyboard(BACKWARD, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera->processKeyboard(LEFT, deltaTime);
	if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera->processKeyboard(RIGHT, deltaTime);
}

unsigned int loadTexture(char const* filepath) {
	//create opengl texture object
	unsigned int textureID;
	glGenTextures(1, &textureID);

	//load texture from file
	int width, height, nrComponents;
	unsigned char* textureData = stbi_load(filepath, &width, &height, &nrComponents, 0);

	if(textureData) {
		//identify format
		GLenum format;
		if(nrComponents == 1) format = GL_RED;
		else if(nrComponents == 3) format = GL_RGB;
		else if(nrComponents == 4) format = GL_RGBA;

		//load texture into opengl
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);

		//set texture filter options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(textureData); //free memory

	} else {
		std::cout << "Texture failed to load at path: " << filepath << std::endl;
		stbi_image_free(textureData); //free memory
	}

	return textureID; //texture id
}