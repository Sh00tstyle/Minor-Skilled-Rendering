//main.cpp : Defines the entry point for the console application.

#include <iostream>
#include <map>

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
#include "Mesh.h"

//functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height); //register a callback function when the window gets resized
void mouse_callback(GLFWwindow* window, double xPos, double yPos);
void scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(char const* filepath, bool gammaCorrection);
unsigned int loadCubemap(std::vector<std::string> faces);

void renderQuad();
void renderCube();

//screen settings
unsigned int screenWidth = 1280;
unsigned int screenHeight = 720;

//time
float deltaTime = 0.0f; //time between the current and the last frame
float lastFrameTime = 0.0f; //time of the last frame

//camera setup
Camera* camera = new Camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastMouseX = screenWidth / 2.0f;
float lastMouseY = screenHeight / 2.0f;
bool firstMouse = true;

//meshes
unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int cubeVAO = 0;
unsigned int cubeVBO;

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

	//create shader programs
	Shader* gBufferShader = new Shader("assets/shader/gBuffer.vs", "assets/shader/gBuffer.fs");
	Shader* deferredShader = new Shader("assets/shader/deferredLight.vs", "assets/shader/deferredLight.fs");
	Shader* colorShader = new Shader("assets/shader/color.vs", "assets/shader/color.fs");

	//load models
	Model* model = new Model("assets/objects/nanosuit/nanosuit.obj");

	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 3.0));

	//configure gBuffer
	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	
	unsigned int gPosition, gNormal, gAlbedoSpecular;

	//position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	//normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	//color & specular color buffer
	glGenTextures(1, &gAlbedoSpecular);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpecular);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpecular, 0);

	//tell OpenGL we to which three rendertargets we render
	unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, attachments);

	//create and attach depth renderbuffer
	unsigned int gRBO;
	glGenRenderbuffers(1, &gRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, gRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gRBO);

	//check for completeness
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//set lighting information
	unsigned int lightAmount = 32;
	std::vector<glm::vec3> lightPositions;
	std::vector<glm::vec3> lightColors;
	srand(13); //set randomizer seed

	for(unsigned int i = 0; i < lightAmount; i++) {
		//calculate slightly random offsets
		float xPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		float yPos = ((rand() % 100) / 100.0) * 6.0 - 4.0;
		float zPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		lightPositions.push_back(glm::vec3(xPos, yPos, zPos));

		//also calculate random color
		float rColor = ((rand() % 100) / 200.0f) + 0.5; //between 0.5 and 1.0
		float gColor = ((rand() % 100) / 200.0f) + 0.5; //between 0.5 and 1.0
		float bColor = ((rand() % 100) / 200.0f) + 0.5; //between 0.5 and 1.0
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

	//configure shader
	deferredShader->use();
	deferredShader->setInt("gPosition", 0);
	deferredShader->setInt("gNormal", 1);
	deferredShader->setInt("gAlbedoSpecular", 2);

	//render loop
	while(!glfwWindowShouldClose(window)) {
		//update time
		float currentFrameTime = (float)glfwGetTime();
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		//input
		processInput(window);

		//clear relevant buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//setup mvp
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix = camera->getViewMatrix();
		glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera->zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);

		//geometry pass to the g-buffer
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//set shader uniforms
		gBufferShader->use();
		gBufferShader->setMat4("viewMatrix", viewMatrix);
		gBufferShader->setMat4("projectionMatrix", projectionMatrix);

		//setup model matrices and draw nanosuit models
		for(unsigned int i = 0; i < objectPositions.size(); i++) {
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, objectPositions[i]);
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.25f));
			gBufferShader->setMat4("modelMatrix", modelMatrix);

			model->draw(gBufferShader);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0); //bind back to normal framebuffer
		
		//render to screen and calculate light
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		deferredShader->use();
		deferredShader->setVec3("cameraPos", camera->position);

		for(unsigned int i = 0; i < 32; i++) {
			deferredShader->setVec3("lights[" + std::to_string(i) + "].position", lightPositions[i]);
			deferredShader->setVec3("lights[" + std::to_string(i) + "].color", lightColors[i]);

			//update attenuation parameters and calculate radius
			float constant = 1.0;
			float linear = 0.7;
			float quadratic = 1.8;

			float lightMax = std::fmaxf(std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b); //get largest light coloc component
			float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256 / 5.0f) * lightMax)));

			deferredShader->setFloat("lights[" + std::to_string(i) + "].linear", linear);
			deferredShader->setFloat("lights[" + std::to_string(i) + "].quadratic", quadratic);
			deferredShader->setFloat("lights[" + std::to_string(i) + "].radius", radius);
		}

		//bind buffers to texture units
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpecular);

		//render screen quad
		renderQuad();

		//blit depth information from the gBuffer to the default framebuffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//render lights on top of the scene using forward rendering using the copied depth information
		colorShader->use();
		colorShader->setMat4("viewMatrix", viewMatrix);
		colorShader->setMat4("projectionMatrix", projectionMatrix);

		for(unsigned int i = 0; i < lightPositions.size(); i++) {
			modelMatrix = glm::mat4(1.0f);
			modelMatrix = glm::translate(modelMatrix, lightPositions[i]);
			modelMatrix = glm::scale(modelMatrix, glm::vec3(0.125f));

			colorShader->setMat4("modelMatrix", modelMatrix);
			colorShader->setVec3("lightColor", lightColors[i]);
			renderCube();
		}

		//check and call events and swap buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	//cleanup resources
	glfwTerminate();
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

unsigned int loadTexture(char const* filepath, bool gammaCorrection) {
	//create opengl texture object
	unsigned int textureID;
	glGenTextures(1, &textureID);

	//load texture from file
	int width, height, nrComponents;
	unsigned char* textureData = stbi_load(filepath, &width, &height, &nrComponents, 0);

	if(textureData) {
		//identify format
		GLenum internalFormat;
		GLenum dataFormat;

		if(nrComponents == 1) {
			internalFormat = dataFormat = GL_RED;
		} else if(nrComponents == 3) {
			internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		} else if(nrComponents == 4) {
			internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			dataFormat = GL_RGBA;
		}

		//load texture into opengl
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, textureData);
		glGenerateMipmap(GL_TEXTURE_2D);

		//set texture filter options
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

unsigned int loadCubemap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	std::string filename;

	for(unsigned int i = 0; i < faces.size(); i++) {
		filename = "assets/skybox/" + faces[i];

		unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
		if(data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void renderQuad() {
	if(quadVAO == 0) {
		float quadVertices[] = {
			//positions        //uv
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};

		//setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void renderCube() {
	//lazy initialize
	if(cubeVAO == 0) {
		float vertices[] = {
			//back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, //bottom-left
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, //top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, //bottom-right         
			1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, //top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, //bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, //top-left
			//front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, //bottom-left
			1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, //bottom-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, //top-right
			1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, //top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, //top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, //bottom-left
			//left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, //top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, //top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, //bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, //bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, //bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, //top-right
			//right face
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, //top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, //bottom-right
			1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, //top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, //bottom-right
			1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, //top-left
			1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, //bottom-left     
			//bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, //top-right
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, //top-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, //bottom-left
			1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, //bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, //bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, //top-right
			//top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, //top-left
			1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, //bottom-right
			1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, //top-right     
			1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, //bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, //top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  //bottom-left        
		};

		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	//render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}