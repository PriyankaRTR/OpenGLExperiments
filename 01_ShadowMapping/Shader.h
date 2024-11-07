#pragma once

#include<string>
#include<fstream>

#include<gl/glew.h>
#include<gl\GL.h>

#include"vmath.h"
class Shader
{
private:

	GLuint programID;
	GLuint vertexShaderID;
	GLuint fragmentShaderID;
	FILE* fpLog;
	GLuint nearPlaneUniform, farPlaneUniform, sampler;
	

	enum
	{
		VDG_ATTRIBUTE_VERTEX = 0,
		VDG_ATTRIBUTE_COLOR,
		VDG_ATTRIBUTE_NORMAL,
		VDG_ATTRIBUTE_TEXTURE0,
	};
	
	
public:
	Shader(const char* fileVertexShader, const char* fileFragmentShader, FILE* fp);
	~Shader();
	void start(void);
	void stop(void);
	void cleanUp(void);
	GLuint loadShader(const char* file, GLuint type);
	bool readFile(const char* pFileName, std::string& outFile);
	
//	//it's a pure virtual function since the implmentation will be different in each derived class
//// to be implemented as 'override' in derived class
//// check difference between pure virtual function in c++ and abstract method in java => it's the same. just different terminology :)
	//virtual void bindAttributes(void) = 0;
	//virtual void getAllUniformLocations(void) = 0;


//protected:



	GLuint getUniformLocation(const char* uniformName);
	void loadMatrix(GLuint location, vmath::mat4 matrix);
	void loadVector(GLuint location, vmath::vec3 vector);
	void loadFloat(GLuint location, GLfloat value);
	void loadBoolean(GLuint location, GLboolean value);
	void loadUint(GLuint location, GLuint value);
	void bindAttribute(GLuint attribute, const char* variableName);
	void setUniformValue(GLfloat nearPlane, GLfloat farPlane);
};

