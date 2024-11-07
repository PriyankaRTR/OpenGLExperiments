#pragma once
#include <stdio.h> // for FILE I/O
#include <stdlib.h>
// OGL
#include <gl\glew.h> // for GLSL extensions IMPORTANT : This Line Should Be Before #include<gl\gl.h> And #include<gl\glu.h>
#include <gl/GL.h>
class ShaderLoader
{
private:
	GLuint shaderObjectID;
	//FILE* gpFile;

public:
	ShaderLoader();
	GLuint loadShader(const GLchar* shaderSource, GLenum type, FILE* gpFile);
	void cleanUp(void);
};

