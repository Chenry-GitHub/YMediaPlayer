#include "windows.h"  
#include "TCHAR.h"  
#include "YMediaPlayer.h"

#include <glfw3.h>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);


const GLuint WIDTH = 800, HEIGHT = 600;

int main(int argc, _TCHAR* argv[])
{


	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	g_hwnd = window;
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);


	glfwMakeContextCurrent(NULL);

	YMediaPlayer::InitPlayer();
	YMediaPlayer player;
	player.SetMediaFromFile("C:\\video.mp4");
	player.Play();

						  // Game loop
	while (!glfwWindowShouldClose(window))
	{
		// 检查事件，调用相应的回调函数，如下文的key_callback函数
		glfwPollEvents();

	}
	glfwTerminate();
	return 0;

}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}