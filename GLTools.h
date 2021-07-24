#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <random>
#include <fstream>
#include <sstream>
#include "HSV.h"
#include <SFML/Graphics.hpp>




namespace nr {
	namespace util {
		std::string ReadFile(const std::string& fileName) {
			std::string srcCode;
			std::ifstream vShaderFile;
			vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			vShaderFile.open(fileName);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			vShaderFile.close();
			return vShaderStream.str();
		}
		sf::Image LoadImage(const std::string& fileName) {
			sf::Image img;
			if (img.loadFromFile(fileName)) {
				return img;
			}
			throw std::exception();
		}
	}

	namespace driver {
		unsigned int NUM_POINTS;
		bool wireframeMode_ = false;
		enum class VERTEXATTRIBUTE : GLuint {
			POSITION = 0,
			COLOR = 1,
		};
		class Camera {
		private:
			float scroll{ 0 };
			float pitch{ 0 };
			float yaw{ 90 };
			float roll{ 0 };

			glm::vec3 cameraPosition_;
			glm::vec3 cameraFront_;
			glm::vec3 cameraUp_;
			glm::mat4 viewMatrix_;

			const float cameraSpeed_ = 1.5;
			const float eulerSpeed_ = 4;
			void UpdateRotation() {
				glm::vec3 direction = glm::vec3(
					cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
					sin(glm::radians(pitch)),
					sin(glm::radians(yaw)) * cos(glm::radians(pitch))
				);
				cameraFront_ = glm::normalize(direction);
			}
		public:
			Camera()
				:viewMatrix_(glm::mat4(1.0f)) {
				cameraPosition_ = glm::vec3(5.0f, 100.0f, 10.0f);
				cameraUp_ = glm::vec3(0.0f, 1.0f, 0.0f);
				cameraFront_ = glm::vec3(-2.0f, -2.0f, -1.0f);
				UpdateRotation();
			}
			inline void MoveNorth() {
				cameraPosition_ = cameraPosition_ + cameraSpeed_ * cameraFront_;
			}
			inline void MoveSouth() {
				cameraPosition_ -= cameraSpeed_ * cameraFront_;
			}
			inline void MoveWest() {
				cameraPosition_ += cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}
			inline void MoveEast() {
				cameraPosition_ -= cameraSpeed_ * glm::normalize(glm::cross(cameraFront_, cameraUp_));
			}
			inline void LookUp() {
				pitch += eulerSpeed_;
				UpdateRotation();
			}
			inline void LookDown() {
				pitch -= eulerSpeed_;
				UpdateRotation();
			}
			inline void LookLeft() {
				yaw -= eulerSpeed_;
				UpdateRotation();

			}
			inline void LookRight() {
				yaw += eulerSpeed_;
				UpdateRotation();
			}
			inline glm::vec3 Position() const noexcept { return cameraPosition_; }
			inline glm::vec3 Up() const noexcept { return cameraUp_; }
			inline glm::vec3 Front() const noexcept { return cameraFront_; }
		};
		class Shader {
		private:
			std::string shaderName_;
			std::string shaderSource_;
			GLuint shaderID_;
		public:
			Shader(const int& shaderType_, const std::string& shaderName, const std::string& shaderSourceFile)
				:
				shaderName_(shaderName),
				shaderSource_(nr::util::ReadFile(shaderSourceFile)) {
				// allocate an id for the shader
				shaderID_ = glCreateShader(shaderType_);
				// bind the source code
				auto str = shaderSource_.data();
				glShaderSource(shaderID_, 1, &str, NULL);
				// compile shader
				glCompileShader(shaderID_);
			}
			bool CheckShader() {
				int success;
				char infoLog[512];
				glGetShaderiv(shaderID_, GL_COMPILE_STATUS, &success);
				if (!success) {
					glGetShaderInfoLog(shaderID_, 512, NULL, infoLog);
					std::cout << infoLog << std::endl;
					return false;
				}
				std::cout << "SHADER FINE" << std::endl;
				return true;
			}
			void Destroy() {
			}
			inline GLuint ID() const noexcept { return shaderID_; }
			~Shader() {
				Destroy();
			}
		};
		
		auto camera_ = std::make_unique<nr::driver::Camera>();

		class Program {
		private:
			std::vector<std::unique_ptr<nr::driver::Shader>> shaders_;
			GLuint programID_;
			inline GLuint GetLocation(const std::string& uniformName) const {
				return glGetUniformLocation(programID_, uniformName.data());
			}
		public:
			void RegisterShader(std::unique_ptr<Shader>&& shader) {
				shaders_.emplace_back(std::move(shader));
			}
			bool Run() {
				// check if shaders are fine first
				for (const auto& shader : shaders_) {
					if (!shader->CheckShader()) return false;
				}
				// create the program
				programID_ = glCreateProgram();
				// attatch the registered shaders
				std::for_each(shaders_.begin(), shaders_.end(), [this](const std::unique_ptr<Shader>& shader) {
					glAttachShader(programID_, shader->ID());
					});

				// link the program
				glLinkProgram(programID_);
				int success;
				char infoLog[512];
				glGetProgramiv(programID_, GL_LINK_STATUS, &success);
				if (!success) {
					glGetProgramInfoLog(programID_, 512, NULL, infoLog);
					std::cout << infoLog << std::endl;
				}
				// decouple the shaders
				std::for_each(shaders_.begin(), shaders_.end(), [](std::unique_ptr<Shader>& shader) {
					glDeleteShader(shader->ID());
					});
				return success;
			}
			void Use() {
				glUseProgram(programID_);
			}
			void SetUniformVec3(const std::string& uniformName, const glm::vec3& vec) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform3f(uniformLoc, vec.x, vec.y, vec.z);
			}
			void SetUniformMat4(const std::string& uniformName, const glm::mat4& mat) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniformMatrix4fv(uniformLoc, 1, GL_FALSE, glm::value_ptr(mat));
			}
			void SetUniformInt(const std::string& uniformName, const int& val) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform1i(uniformLoc, val);
			}
			void SetUniformFloat(const std::string& uniformName, const float& val) {
				GLuint uniformLoc = GetLocation(uniformName);
				glUniform1f(uniformLoc, val);
			}
		};
	}
}
namespace nr {
	namespace callbacks {
		void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
			switch (key) {
			case GLFW_KEY_SPACE: {

				if (glfwGetKey(window, GLFW_KEY_SPACE) != GLFW_PRESS) break;

				nr::driver::wireframeMode_ = !nr::driver::wireframeMode_;
				if (nr::driver::wireframeMode_)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				
				break;
			}
			case GLFW_KEY_S:
			{
				nr::driver::camera_->MoveSouth();
				break;
			}
			case GLFW_KEY_W:
			{
				nr::driver::camera_->MoveNorth();
				break;
			}
			case GLFW_KEY_A:
			{
				nr::driver::camera_->MoveEast();
				break;
			}
			case GLFW_KEY_D:
			{
				nr::driver::camera_->MoveWest();
				break;
			}
			case GLFW_KEY_LEFT:
			{
				nr::driver::camera_->LookLeft();
				break;
			}
			case GLFW_KEY_RIGHT:
			{
				nr::driver::camera_->LookRight();
				break;
			}
			case GLFW_KEY_UP: {
				nr::driver::camera_->LookUp();
				break;
			}
			case GLFW_KEY_DOWN: {
				nr::driver::camera_->LookDown();
				break;
			}
			}
		}
	}
}
namespace nr {
	namespace driver {
		std::unique_ptr<nr::driver::Program> shaderProgram_;
		GLFWwindow* window_;
		glm::mat4 projectionMatrix_;
		GLuint VAO_;
		GLuint VBO_;
		GLuint EBO_;
		namespace init {
			inline bool InitContext() {
				glfwMakeContextCurrent(nr::driver::window_);
				return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			}
			bool InitWindow(const unsigned int& width, const unsigned int& height, const char* title) {
				glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
				glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
				glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
				nr::driver::window_ = glfwCreateWindow(width, height, title, NULL, NULL);
				return nr::driver::window_;
			}
			inline void InitCallbacks() {
				glfwSetKeyCallback(nr::driver::window_, &nr::callbacks::KeyCallback);
			}

			void InitMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices) {
				// load up the height map 
				sf::Image heightMap = nr::util::LoadImage("map.jpg");
				sf::Vector2u mapDim{ heightMap.getSize() };

				// generate a X-Z vertex plane
				const float VERTEX_X_SPACE = 1;
				const float VERTEX_Z_SPACE = 1;
				const float MAX_Y = pow(2,11);

				const sf::Uint8* pxls = heightMap.getPixelsPtr();
				for (unsigned int j = 0; j < mapDim.y; ++j) {
					for (unsigned int i = 0; i < mapDim.x; ++i) {
						sf::Color col = heightMap.getPixel(i, j);
						float colavg = col.r + col.g + col.b;
						colavg /= 255;
						vertices.insert(vertices.end(),
							{ i * VERTEX_X_SPACE ,
							(MAX_Y * colavg) / 255 ,
							j * VERTEX_Z_SPACE
							});
					}
				}
				for (unsigned int j = 0; j < mapDim.y - 1; ++j) {
					for (unsigned int i = 0; i < mapDim.x - 1; ++i) {
						unsigned int index = j * mapDim.x + i;
						indices.push_back(index);
						indices.push_back(index + 1);
						if (i % 2 == 0) {
							indices.insert(indices.end(),
								{
									index + mapDim.x + 1,
									index,
									index + mapDim.x,
									index + mapDim.x + 1
								});
							continue;
						}
						indices.insert(indices.end(),
							{
								index + mapDim.x,
								index + mapDim.x,
								index + mapDim.x + 1,
								index + 1
							});
					}
				}
				NUM_POINTS = vertices.size();
			}
			void InitArrays() {

				std::vector<float> vertices;
				std::vector<unsigned int> indices;

				InitMesh(vertices, indices);

				// store a single VAO, which stores a single VBO for all particles.
				glGenVertexArrays(1, &VAO_);
				// bind the VAO
				glBindVertexArray(VAO_);

				// generate a VBO handle
				glGenBuffers(1, &VBO_);

				// generate an EBO handle
				glGenBuffers(1, &EBO_);

				// copy the data into the VBO
				// bind the VBO
				glBindBuffer(GL_ARRAY_BUFFER, VBO_);

				// copy the vertex data into the vbo.
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

				// bind the vertex attrib pointers.

				// bind the position
				glVertexAttribPointer(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);

		

				// enable the vertex attributes
				glEnableVertexAttribArray(static_cast<GLuint>(nr::driver::VERTEXATTRIBUTE::POSITION));


				// bind the ebo.
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);

				// copy the index data into the ebo.
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(float), indices.data(), GL_STATIC_DRAW);

				std::cout << "done" << std::endl;
			}
			void InitShaders() {
				shaderProgram_ = std::make_unique<nr::driver::Program>();
				shaderProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_VERTEX_SHADER, "vertexShader", "vertexShader.vert"));
				shaderProgram_->RegisterShader(std::make_unique<nr::driver::Shader>(GL_FRAGMENT_SHADER, "fragmentShader", "fragmentShader.frag"));
			}
			bool InitProgram(const unsigned int& windowWidth, const unsigned int& windowHeight, const char* windowName) {
				if (!glfwInit() || !InitWindow(windowWidth, windowHeight, windowName) || !InitContext()) return false;
				InitCallbacks();
				InitArrays();
				InitShaders();
				shaderProgram_->Run();
				return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
			}
		}
		void Render() {
			shaderProgram_->Use();
			projectionMatrix_ = glm::mat4(1.0f);
			projectionMatrix_ = glm::perspective(glm::radians(45.0f), (float)1000 / (float)1000, 0.1f, 500.0f);
			shaderProgram_->SetUniformMat4("projectionMatrix", projectionMatrix_);

			int frameNumber = 0;
			while (!glfwWindowShouldClose(window_)) {
				glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				glm::mat4 viewMatrix_ = glm::mat4(1.0f);
				viewMatrix_ = glm::lookAt(camera_->Position(), camera_->Position() + camera_->Front(), camera_->Up());

				shaderProgram_->Use();
				shaderProgram_->SetUniformMat4("viewMatrix", viewMatrix_);
				shaderProgram_->SetUniformFloat("time", glfwGetTime());

				glBindVertexArray(VAO_);
				glDrawElements(GL_TRIANGLES, 1000*1000, GL_UNSIGNED_INT, 0);

				glfwPollEvents();
				glfwSwapBuffers(window_);
				++frameNumber;
			}
		}
	}

}