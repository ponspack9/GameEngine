#include "Mesh.h"
#include "Application.h"
#include "ModuleEditor.h"
#include "Viewport.h"
#include "Shader.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"
#include "Assimp/include/cfileio.h"

Mesh::Mesh() : Resources(Resources::Type::mesh)
{
}

Mesh::~Mesh()
{
	glDeleteProgram(shader_program);
	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);
}

void Mesh::ImportMesh(aiMesh* mesh)
{
	LOG("Importing mesh '%s'", (mesh->mName.C_Str()), 'g');

	// Vertices
	num_vertices = mesh->mNumVertices;
	vertices = new float3[num_vertices];

	LOG("Importing vertices %u",mesh->mNumVertices , 'g');
	for (uint i = 0; i < mesh->mNumVertices; ++i)
	{
		vertices[i].x = mesh->mVertices[i].x;
		vertices[i].y = mesh->mVertices[i].y;
		vertices[i].z = mesh->mVertices[i].z;
	}

	// Indices
	num_indices = mesh->mNumFaces * 3;
	indices = new GLuint[num_indices];

	for (uint i = 0; i < mesh->mNumFaces; ++i)
	{

		assert(mesh->mFaces[i].mNumIndices == 3); // assert if face is not a triangle

		memcpy(&indices[i * 3], mesh->mFaces[i].mIndices, 3 * sizeof(GLuint));
	}

	//// Normals
	//if (mesh->HasNormals())
	//{
	//	LOG("Importing normals %u",mesh->mNumVertices , 'g');
	//	for (unsigned i = 0; i < mesh->mNumVertices; ++i)
	//	{
	//		this->vertices[i].normal[0] = *((GLfloat*)&mesh->mNormals[i].x);
	//		this->vertices[i].normal[1] = *((GLfloat*)&mesh->mNormals[i].y);
	//		this->vertices[i].normal[2] = *((GLfloat*)&mesh->mNormals[i].z);
	//	}
	//}

	// Texture Coordinates
	if (mesh->HasTextureCoords(0))
	{
		num_tex_coords = mesh->mNumVertices;
		tex_coords = new float2[num_tex_coords * 2];

		for (unsigned i = 0; i < mesh->mNumVertices; ++i)
		{
			LOG("Importing texture coord to vertex %u", i, 'g');
			tex_coords[i].x = mesh->mTextureCoords[0][i].x;
			tex_coords[i].y = mesh->mTextureCoords[0][i].y;
		}
	}
	/*if (new_mesh->HasTextureCoords(0))
	{
		m->num_tex_coords = m->num_vertex;
		m->tex_coords = new float[m->num_tex_coords * 2];

		for (int i = 0; i < m->num_tex_coords; ++i)
		{
			m->tex_coords[i * 2] = new_mesh->mTextureCoords[0][i].x;
			m->tex_coords[(i * 2) + 1] = new_mesh->mTextureCoords[0][i].y;
		}
	}*/

	// Colors
	/*for (unsigned i = 0; i < mesh->mNumVertices; ++i)
	{
		if (mesh->HasVertexColors(i))
		{
			LOG("Importing colors to vertex %u", i , 'g');
			this->vertices[i].color[0] = *((GLfloat*)&mesh->mColors[i]->r);
			this->vertices[i].color[1] = *((GLfloat*)&mesh->mColors[i]->g);
			this->vertices[i].color[2] = *((GLfloat*)&mesh->mColors[i]->b);
			this->vertices[i].color[3] = *((GLfloat*)&mesh->mColors[i]->a);
		}
	}*/

	

	
	LOG("Generating VBO", 'g');
	GenVBO();
	LOG("Generating IBO", 'g');
	GenIBO();
	LOG("Generating VAO", 'g');
	GenVAO();

	LOG("Generating Texture", 'g');
	GenTexture();
	//LOG("Generating shaders", 'g');
	//GenShaders();
}

void Mesh::GenTexture()
{
	glGenBuffers(1, &tex_coords_id);
	glBindBuffer(GL_ARRAY_BUFFER, tex_coords_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float2) * num_tex_coords, tex_coords, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::ImportTexture(const char* path)
{
	// Devil
	unsigned int imageID;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);
	//ilEnable(IL_ORIGIN_SET);
	//ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

	bool loaded = ilLoadImage(path);
	if (!loaded) LOG("IMAGE '%s' COULD NOT BE LOADED PROPERLY", path, 'e');

	LOG("Importing texture [%d,%d] data size: %u", ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), sizeof(ilGetData()), 'g');
	
	glGenTextures(1, &TEX);
	//glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TEX);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGB, GL_UNSIGNED_BYTE, ilGetData());
	glGenerateMipmap(GL_TEXTURE_2D);

	ilDeleteImages(1, &imageID);

}

void Mesh::GenVBO()
{
	assert(vertices != nullptr);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * num_vertices, vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::GenIBO()
{
	assert(indices != nullptr);

	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * num_indices, indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::GenVAO()
{
	//glGenVertexArrays(1, &VAO);
	//glBindVertexArray(VAO);
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position))); //pos
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal))); //normal
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)(offsetof(Vertex, color))); //color
	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, tex_coords))); //texture coordinates
	//glEnableVertexAttribArray(3);
	///*
	//GLint pos_attrib = glGetAttribLocation(shader_program, "position");
	//glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position))); //pos
	//glEnableVertexAttribArray(pos_attrib);

	//GLint normal_attrib = glGetAttribLocation(shader_program, "normal");
	//glVertexAttribPointer(normal_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal))); //normal
	//glEnableVertexAttribArray(normal_attrib);

	//GLint color_attrib = glGetAttribLocation(shader_program, "color");
	//glVertexAttribPointer(color_attrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)(offsetof(Vertex, color))); //color
	//glEnableVertexAttribArray(color_attrib);

	//GLint tex_coord_attrib = glGetAttribLocation(shader_program, "tex_coord");
	//glVertexAttribPointer(tex_coord_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, tex_coords))); //texture coordinates
	//glEnableVertexAttribArray(tex_coord_attrib);*/

	//glBindVertexArray(0);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void CheckShaderErrors(GLuint shader)
{
	// Error checking
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE)
	{
		char buffer[1024];
		glGetShaderInfoLog(shader, 1024, NULL, buffer);
		LOG("%s", buffer, 'e');

	}
}
void Mesh::GenShaders()
{
	// Create and compile the vertex shader
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &Shader::vertex_source, NULL);
	glCompileShader(vertex_shader);

	//Error checking
	CheckShaderErrors(vertex_shader);

	// Create and compile the fragment shader
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &Shader::fragment_source, NULL);
	glCompileShader(fragment_shader);

	//Error checking
	CheckShaderErrors(fragment_shader);

	// Link the vertex and fragment shader into a shader program
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	glBindFragDataLocation(shader_program, 0, "outColor");

	glLinkProgram(shader_program);
	glUseProgram(shader_program);
}