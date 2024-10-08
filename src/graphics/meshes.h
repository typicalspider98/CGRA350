#ifndef MESHES
#define MESHES
#pragma once

#include <glad/glad.h>
#include "buffers.h"

#include <vector>
#include <glm/glm.hpp>

#include <map>  // ȷ�����������ͷ�ļ�

struct Material {
    glm::vec3 ambient;  // ��������ɫ
    glm::vec3 diffuse;  // ��������ɫ
    glm::vec3 specular; // ���淴����ɫ
    float shininess;    // �����
    std::string diffuseMap;  // �����������ļ���
    std::string specularMap; // ���淴�������ļ���

    Material() : ambient(0.1f), diffuse(0.8f), specular(0.5f), shininess(32.0f) {}
};

struct MeshPart {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;
    GLuint vao, vbo_positions, vbo_normals, vbo_texCoords, ebo_indices;

    // ��ʼ��ÿ��MeshPart��VAO��VBO
    void initialise() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo_positions);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        if (!normals.empty()) {
            glGenBuffers(1, &vbo_normals);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(1);
        }

        if (!texCoords.empty()) {
            glGenBuffers(1, &vbo_texCoords);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
            glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(2);
        }

        glGenBuffers(1, &ebo_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }

    // ��Ⱦ����
    void render() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

// --- Mesh abstract class ---
class Mesh
{
private:
	VAO m_vao;
	VBO m_positionsVBO;
	VBO m_normalsVBO;
	VBO m_texCoordsVBO;
	EBO m_ebo;
	int m_vertexCount;

	void Mesh::loadDataToGPU(const std::vector<float> &positions, 
							const std::vector<float> &normals, 
							const std::vector<float> &texCoords, 
							const std::vector<int> &indices);
	
	// Initialise the data arrays for positions, normals, texture coordinates & vertex indices
	virtual void initPositions(std::vector<float> &pos) = 0;
	virtual void initNormals(std::vector<float> &normals) = 0;
	virtual void initTexCoords(std::vector<float> &texCoords) = 0;
	virtual void initIndices(std::vector<int> &indices) = 0;

protected:
	Mesh();

public:
	virtual ~Mesh();
	void render();

	void initialise();

	VAO &getVAO();
	VBO &getPositionsVBO();
	VBO &getNormalsVBO();
	VBO &getTexCoordsVBO();
};


// --- Simple test mesh for initial debugging ---
class TestMesh : public Mesh
{
private:
	void initPositions(std::vector<float> &pos);
	void initNormals(std::vector<float> &normals);
	void initTexCoords(std::vector<float> &texCoords);
	void initIndices(std::vector<int> &indices);
};

// --- Cubemap mesh ---
class CubeMapMesh : public Mesh
{
private:
	static CubeMapMesh *s_instance;

	CubeMapMesh();

	void initPositions(std::vector<float> &pos);
	void initNormals(std::vector<float> &normals);
	void initTexCoords(std::vector<float> &texCoords);
	void initIndices(std::vector<int> &indices);

public:
	static CubeMapMesh &getInstance();
	~CubeMapMesh();
};


// --- Grid mesh (for Ocean & Terrain) ---
class GridMesh : public Mesh
{
private:
	int m_grid_width;
	int m_grid_length;

	void initPositions(std::vector<float> &pos);
	void initNormals(std::vector<float> &normals);
	void initTexCoords(std::vector<float> &texCoords);
	void initIndices(std::vector<int> &indices);

public:
	GridMesh(int grid_width, int grid_length);

	void updateMeshGrid(int new_grid_width, int new_grid_length);

	int getGridWidth() const;
	int getGridLength() const;
};


// --- Screen Quad mesh (for visual debugging) ---
class ScreenQuadMesh : public Mesh
{
private:
	void initPositions(std::vector<float> &pos);
	void initNormals(std::vector<float> &normals);
	void initTexCoords(std::vector<float> &texCoords);
	void initIndices(std::vector<int> &indices);

public:
	ScreenQuadMesh();
};

class ObjMesh {
public:
    // ��Ա����
    GLuint vao;  // Vertex Array Object
    GLuint vbo_positions, vbo_normals, vbo_texCoords, ebo_indices;  // VBOs��EBO

    std::vector<glm::vec3> positions;  // ����λ��
    std::vector<glm::vec3> normals;    // ����
    std::vector<glm::vec2> texCoords;  // ��������
    std::vector<unsigned int> indices; // ����

    // ���캯��
    ObjMesh(const std::vector<glm::vec3>& pos,
        const std::vector<glm::vec3>& norm,
        const std::vector<glm::vec2>& uv,
        const std::vector<unsigned int>& ind)
        : positions(pos), normals(norm), texCoords(uv), indices(ind) {}

    // ��ʼ������
    void initialise() {
        // ���ɲ���VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // ���ɲ��󶨶��㻺����� (VBO)
        glGenBuffers(1, &vbo_positions);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_positions);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        // ����з���
        if (!normals.empty()) {
            glGenBuffers(1, &vbo_normals);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(1);
        }

        // �������������
        if (!texCoords.empty()) {
            glGenBuffers(1, &vbo_texCoords);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_texCoords);
            glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glEnableVertexAttribArray(2);
        }

        // ���ɲ���Ԫ�ػ������ (EBO)
        glGenBuffers(1, &ebo_indices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // ���VAO
        glBindVertexArray(0);
    }

    // ��Ⱦ����
    void render() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // ��ȡVAO�ĺ���
    GLuint getVAO() const {
        return vao;
    }

    // ��ȡPosition VBO�ĺ���
    GLuint getPositionsVBO() const {
        return vbo_positions;
    }

    // ����OpenGL��Դ
    void destroy() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo_positions);
        glDeleteBuffers(1, &vbo_normals);
        glDeleteBuffers(1, &vbo_texCoords);
        glDeleteBuffers(1, &ebo_indices);
    }

    std::map<std::string, MeshPart> parts;  // �洢��ͬ�Ĳ��������硰walls������wood������glass��

    // ��ʼ����������Բ�ͬ��λ���ò�ͬ�ĳ�ʼ����
    void initialisePart(const std::string& partName, const std::vector<glm::vec3>& pos,
        const std::vector<glm::vec3>& norm,
        const std::vector<glm::vec2>& uv,
        const std::vector<unsigned int>& ind)
    {
        parts[partName].positions = pos;
        parts[partName].normals = norm;
        parts[partName].texCoords = uv;
        parts[partName].indices = ind;
        parts[partName].initialise();  // ��ʼ���ò�λ��VAO��VBO
    }

    // ��Ⱦָ���Ĳ���
    void renderPart(const std::string& partName) {
        if (parts.find(partName) != parts.end()) {
            parts[partName].render();
        }
    }

    // �����ļ�
    // �����洢ÿ�����ʵ����ƺ����Ӧ�� Material ����
    std::map<std::string, Material> materials;

    // ���ز����ļ��ĺ���
    void loadMtl(const std::string& filepath);
};

#endif