#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <gl\glew.h>
#include <gl/GL.h>
#include "vmath.h"
class Shadow
{
private:
	GLuint vertexShaderObject;
	GLuint fragmentShaderObject;
	GLuint shadowShaderProgramObject;

public:
	GLuint depthMapFBO;
	GLuint depthMap;
	GLuint SHADOW_HEIGHT;
	GLuint SHADOW_WIDTH;
	GLuint gLightSpaceMatUniform;
	GLuint gModelUniform;

	Shadow();
	~Shadow();

	GLuint getDepthMap(void);
	void createDepthBuffer(void);
	void createTextureMap(void);
	void attachTextureToFBO(void);
	GLuint createShadowShaderProgram(void);
	void setLightSpaceMatUniform(vmath::mat4 matrix);
	void setModelMatUniform(vmath::mat4 matrix);
	void bindDepthFramebuffer(void);
	void unbindDepthFramebuffer(void);
	
	enum
	{
		VDG_ATTRIBUTE_VERTEX = 0,
		VDG_ATTRIBUTE_COLOR,
		VDG_ATTRIBUTE_NORMAL,
		VDG_ATTRIBUTE_TEXTURE0,
	};
};

