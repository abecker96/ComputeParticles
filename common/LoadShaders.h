

#ifndef LOADSHADER_H
#define LOADSHADER_H

#include <GL/glew.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>


// Function to load shaders
// It looks pretty long and annoying, but it's mostly boilerplate code to
//  read from a file. Anything actually interesting with shaders happens a little
//  less than halfway through
// TODO: this is actually all boilerplate. Putting it in a separate file to use later
//  would be a good idea
GLuint LoadShaders(const char * vertex_file_path, const char * geometry_file_path, const char * fragment_file_path)
{
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
    // This part is just reading from a file, nothing to do with computer graphics
    // This is always so fiddly, I really don't mind copy/pasting from the tutorial
    // even if I would do it differently.
    // http://www.opengl-tutorial.org/beginners-tutorials/tutorial-2-the-first-triangle/
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Geometry Shader code from the file
	std::string GeometryShaderCode;
	std::ifstream GeometryShaderStream(geometry_file_path, std::ios::in);
	if(GeometryShaderStream.is_open()){
		std::stringstream sstr;
		sstr << GeometryShaderStream.rdbuf();
		GeometryShaderCode = sstr.str();
		GeometryShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", geometry_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}else{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
		getchar();
		return 0;
	}

    // Init result variables to check return values
	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
    // Read shader as c_string
	char const * VertexSourcePointer = VertexShaderCode.c_str();
    // Read shader source into VertexShaderID
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    // Compile shader
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
    // These functions get the requested shader information
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("Compiling shader : %s\n", vertex_file_path);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Geometry Shader
    // Read shader as c_string
	char const * GeometrySourcePointer = GeometryShaderCode.c_str();
    // Read shader source into GeometryShaderID
	glShaderSource(GeometryShaderID, 1, &GeometrySourcePointer , NULL);
    // Compile shader
	glCompileShader(GeometryShaderID);

	// Check Vertex Shader
    // These functions get the requested shader information
	glGetShaderiv(GeometryShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(GeometryShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> GeometryShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(GeometryShaderID, InfoLogLength, NULL, &GeometryShaderErrorMessage[0]);
		printf("Compiling shader : %s\n", geometry_file_path);
		printf("%s\n", &GeometryShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
    // Same steps as vertex shader
    // TODO: condense into a single function or loop maybe?
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("Compiling shader : %s\n", fragment_file_path);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, GeometryShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("Linking program\n");
		printf("%s\n", &ProgramErrorMessage[0]);
	}

    // Cleanup
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	// printf("Vertex Shader source:\n%s\n", VertexShaderCode.c_str());
	// printf("Geometry Shader source:\n%s\n", GeometryShaderCode.c_str());
	// printf("Fragment Shader source:\n%s\n", FragmentShaderCode.c_str());

	return ProgramID;
}


#endif