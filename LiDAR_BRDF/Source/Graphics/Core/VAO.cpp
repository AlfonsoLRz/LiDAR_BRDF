#include "stdafx.h"
#include "VAO.h"

/// [Public methods]

VAO::VAO(bool gpuGeometry): _vao(-1), _vbo(RendEnum::numVBOTypes()), _ibo(RendEnum::numIBOTypes())
{
	// [VAO]
	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	// [VBO]
	if (!gpuGeometry)
	{
		_vboIndex = 0;
		this->declareSimpleVBO(RendEnum::VBO_POSITION, sizeof(vec4), GL_FLOAT, _vboIndex);
		this->declareSimpleVBO(RendEnum::VBO_NORMAL, sizeof(vec3), GL_FLOAT, _vboIndex);
		this->declareSimpleVBO(RendEnum::VBO_TEXT_COORD, sizeof(vec2), GL_FLOAT, _vboIndex);
		this->declareSimpleVBO(RendEnum::VBO_TANGENT, sizeof(vec3), GL_FLOAT, _vboIndex);
	}
	else
	{
		this->genGPUGeometryVBO();
	}

	// [IBO]
	glGenBuffers(RendEnum::numIBOTypes(), _ibo.data());
}

VAO::~VAO()
{
	glDeleteBuffers(RendEnum::numVBOTypes(), _vbo.data());
	glDeleteBuffers(RendEnum::numIBOTypes(), _ibo.data());
	glDeleteVertexArrays(1, &_vao);
}

void VAO::defineColorTextCoordVBO()
{
	glBindVertexArray(_vao);

	// VBOs
	glGenBuffers(1, &_vbo[RendEnum::VBO_COLOR_TEXT_COORD_X]);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_COLOR_TEXT_COORD_X]);

	// Offset
	glEnableVertexAttribArray(10);
	glVertexAttribPointer(10, sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(GLfloat), (GLubyte*)nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(10, 1);
}

void VAO::defineGPSTimeVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_GPS_TIME, sizeof(GLfloat), GL_FLOAT, _vboIndex);
}

void VAO::defineIntensityVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_INTENSITY, sizeof(GLfloat), GL_FLOAT, _vboIndex);
}

void VAO::defineNormalVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_NORMAL, sizeof(vec3), GL_FLOAT, _vboIndex);
}

void VAO::defineReturnNumberVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_RETURN_NUMBER, sizeof(GLfloat), GL_FLOAT, _vboIndex);
}

void VAO::defineReturnPercentageVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_RETURN_DIVISION, sizeof(GLfloat), GL_FLOAT, _vboIndex);
}

void VAO::defineOffsetVBO()
{
	glBindVertexArray(_vao);

	// VBOs
	glGenBuffers(1, &_vbo[RendEnum::VBO_OFFSET]);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_OFFSET]);

	// Offset
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, sizeof(vec4) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vec4), (GLubyte*)nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(4, 1);
}

void VAO::defineRotationVBO()
{
	glBindVertexArray(_vao);

	// Rotation 
	glGenBuffers(1, &_vbo[RendEnum::VBO_ROTATION]);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_ROTATION]);

	glEnableVertexAttribArray(6);
	glVertexAttribPointer(6, sizeof(vec4) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, sizeof(vec4), (GLubyte*)nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(6, 1);
}

void VAO::defineScaleVBO()
{
	glBindVertexArray(_vao);

	// VBOs
	glGenBuffers(1, &_vbo[RendEnum::VBO_SCALE]);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_SCALE]);

	// Scale as a float (uniform)
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, sizeof(vec4) / sizeof(GL_FLOAT), GL_FLOAT, GL_FALSE, sizeof(vec4), (GLubyte*)nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(5, 1);
}

void VAO::defineScanAngleRankVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_SCAN_ANGLE_RANK, sizeof(GLfloat), GL_FLOAT, _vboIndex);
}

void VAO::defineScanDirectionVBO()
{
	glBindVertexArray(_vao);
	this->declareSimpleVBO(RendEnum::VBO_SCAN_ANGLE_DIRECTION, sizeof(vec3), GL_FLOAT, _vboIndex);
}

void VAO::drawObject(const RendEnum::IBOTypes iboType, const GLuint openGLPrimitive, const GLuint numIndices)
{
	glBindVertexArray(_vao);
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo[iboType]);
		glDrawElements(openGLPrimitive, numIndices, GL_UNSIGNED_INT, nullptr);
	}
}

void VAO::drawObject(const RendEnum::IBOTypes iboType, const GLuint openGLPrimitive, const GLuint numIndices, const GLuint numObjects)
{
	glBindVertexArray(_vao);
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo[iboType]);
		glDrawElementsInstanced(openGLPrimitive, numIndices, GL_UNSIGNED_INT, nullptr, numObjects);
	}
}

void VAO::drawObject(const GLuint openGLPrimitive, const GLuint numVertices, const GLuint numObjects)
{
	glBindVertexArray(_vao);
	{
		glDrawArraysInstanced(openGLPrimitive, 0, numVertices, numObjects);
	}
}

void VAO::setVBOData(const std::vector<Model3D::VertexGPUData>& geometryData, const GLuint changeFrequency)
{
	glBindVertexArray(_vao);

	{
		glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_POSITION]);
		glBufferData(GL_ARRAY_BUFFER, geometryData.size() * sizeof(Model3D::VertexGPUData), geometryData.data(), changeFrequency);
	}
}

void VAO::setIBOData(const RendEnum::IBOTypes iboType, const std::vector<GLuint>& topologyData, const GLuint changeFrequency)
{
	glBindVertexArray(_vao);
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo[iboType]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, topologyData.size() * sizeof(GLuint), topologyData.data(), changeFrequency);
	}
}

/// [Protected methods]

void VAO::declareSimpleVBO(const RendEnum::VBOTypes vboType, const GLsizei dataSize, const GLuint unitType, int& shaderSlot)
{
	// VAO is considered to be binded at this point

	GLuint vbo = 0;

	glGenBuffers(1, &vbo);	_vbo[vboType] = vbo;						// Our VBO array now contains the identifier for such an VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	{
		glEnableVertexAttribArray(shaderSlot);
		glVertexAttribPointer(shaderSlot++, dataSize / sizeof(unitType), unitType, GL_FALSE, dataSize, ((GLubyte*)nullptr));			// Once shader slot is defined at both sentences, it can be increased
	}
}

void VAO::declareInterleavedVBO(const RendEnum::VBOTypes vboType, const GLsizei totalSize, const std::vector<unsigned long long>& dataSize, const std::vector<GLuint>& unitType, int& shaderSlot)
{
	// VAO is considered to be binded at this point

	GLuint vbo = 0;
	const int subelements = std::min(dataSize.size(), unitType.size());			// Just in case there is any discrepance between both arrays
	unsigned long long accumSize = 0;											// Pointer displacement for any attribute

	glGenBuffers(1, &vbo);	_vbo[vboType] = vbo;								// Our VBO array now contains the identifier for such an VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	for (int i = 0; i < subelements; ++i)
	{
		glEnableVertexAttribArray(shaderSlot);
		glVertexAttribPointer(shaderSlot++, (GLint) (dataSize[i] / sizeof(unitType[i])), unitType[i], GL_FALSE, totalSize, ((GLubyte*)nullptr + (accumSize)));

		accumSize += dataSize[i];
	}
}

void VAO::genGPUGeometryVBO()
{
	size_t accumSize = 0;
	const size_t structSize = sizeof(Model3D::VertexGPUData);

	// VBOs
	glGenBuffers(1, &_vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo[RendEnum::VBO_POSITION]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, sizeof(vec3) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, structSize, ((GLubyte*)nullptr));
	accumSize += sizeof(vec4);

	// Normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, sizeof(vec3) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, structSize, ((GLubyte*)nullptr + accumSize));
	accumSize += sizeof(vec4);

	// Texture coordinates
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, sizeof(vec2) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, structSize, ((GLubyte*)nullptr + accumSize));
	accumSize += sizeof(vec4);

	// Tangents
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, sizeof(vec3) / sizeof(GLfloat), GL_FLOAT, GL_FALSE, structSize, ((GLubyte*)nullptr + accumSize));
}
