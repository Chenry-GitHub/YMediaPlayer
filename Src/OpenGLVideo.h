#pragma once
#include "BaseVideo.h"

#include <glm.hpp>
#include <ext.hpp>
#include <glew.h>
#include <glfw3.h>
#include <gl/GL.h>

class OpenGLVideo:public BaseVideo
{
public:
	OpenGLVideo();
	~OpenGLVideo();
;

virtual void SetDisplay(void *) override;

protected:
	virtual void PlayThread() override;


	GLuint vao;
	GLuint vertex_buffer;
	GLuint texture_buffer;
	GLuint TextureID;
	GLuint vert_shader, frag_shader;
	GLuint program;
	GLFWwindow * win_;
};

