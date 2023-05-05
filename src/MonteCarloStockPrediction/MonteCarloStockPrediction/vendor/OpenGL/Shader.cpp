#include "Shader.h"

unsigned int Shader::CompileShader(unsigned int type, const std::string& source) {
	GLCall(unsigned int id = glCreateShader(type));
	const char* src = source.c_str();
	GLCall(glShaderSource(id, 1, &src, nullptr));
	GLCall(glCompileShader(id));

	//error handeling
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		char* message = (char*)alloca(length * sizeof(char));
		glGetShaderInfoLog(id, length, &length, message);
		std::cout << "Failed to compile shader" << std::endl;
		std::cout << ((type == GL_VERTEX_SHADER) ? "Vertex Shader" : "Fragment Shader") << std::endl;
		std::cout << message << std::endl;
		return 0;
	}
	return id;
}

unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
	GLCall(unsigned int program = glCreateProgram());
	GLCall(unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader));
	GLCall(unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader));

	GLCall(glAttachShader(program, vs));
	GLCall(glAttachShader(program, fs));
	GLCall(glLinkProgram(program));
	GLCall(glValidateProgram(program));

	GLCall(glDeleteShader(vs));
	GLCall(glDeleteShader(fs));
	return program;
}

Shader::Shader(const std::string& filepathVertexShader, const std::string& filepathFragmentShader) :
	m_RendererID(0), m_filepathFragmentShader(filepathFragmentShader), m_filepathVertexShader(filepathVertexShader) {
	std::ifstream vertexShaderFile(filepathVertexShader);
	std::ifstream fragmentShaderFile(filepathFragmentShader);
	std::string vertexShader((std::istreambuf_iterator<char>(vertexShaderFile)), std::istreambuf_iterator<char>());
	std::string fragmentShader((std::istreambuf_iterator<char>(fragmentShaderFile)), std::istreambuf_iterator<char>());
	// std::cout<<vertexShader<<std::endl;
	// std::cout<<fragmentShader<<std::endl;
	m_RendererID = CreateShader(vertexShader, fragmentShader);
}
Shader::~Shader() {
	GLCall(glDeleteProgram(m_RendererID));
}

void Shader::Bind() const {
	GLCall(glUseProgram(m_RendererID));
}
void Shader::Unbind() const {
	GLCall(glUseProgram(0));
}
void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
	GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}
int Shader::GetUniformLocation(const std::string& name) {
	// std::cout<<m_RendererID<<std::endl;
	if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) {
		return m_UniformLocationCache[name];
	}
	GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
	if (location == -1)
		std::cout << "Warning : uniform '" << name << "' doesn't exist" << std::endl;
	m_UniformLocationCache[name] = location;
	return location;
}