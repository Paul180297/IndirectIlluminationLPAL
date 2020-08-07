#include "vertex_array_object.h"

#include <vector>
#include <unordered_map>
#include <tiny_obj_loader.h>

struct Vertex {
    Vertex()
        : position(0.0f, 0.0f, 0.0f)
        , texcoord(0.0f, 0.0f)
        , normal(0.0f, 0.0f, 0.0f) {
    }

	Vertex(const glm::vec3 &position, const glm::vec2 &texcoord, const glm::vec3 &normal)
        : position(position)
        , texcoord(texcoord) 
        , normal(normal) {
    }

    bool operator==(const Vertex& other) const {
        if (position != other.position) return false;
        if (texcoord != other.texcoord) return false;
        if (normal != other.normal) return false;
        return true;
    }

	glm::vec3 position;
	glm::vec2 texcoord;
	glm::vec3 normal;
};

namespace std {

template <>
struct hash<Vertex> {
    size_t operator()(const Vertex &v) const {
        size_t h = 0;
        h = hash<glm::vec3>()(v.position) ^ (h << 1);
        h = hash<glm::vec2>()(v.texcoord) ^ (h << 1);
        h = hash<glm::vec3>()(v.normal) ^ (h << 1);
        return h;
    }
};

}  // namespace std

VertexArrayObject::VertexArrayObject() {
}

VertexArrayObject::~VertexArrayObject() {
}

void VertexArrayObject::create() {
    glGenVertexArrays(1, &vaoId);
    glBindVertexArray(vaoId);

    glGenBuffers(1, &vboId);
    glBindBuffer(GL_ARRAY_BUFFER, vboId);

    glGenBuffers(1, &iboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);

    glBindVertexArray(0);
}

void VertexArrayObject::draw(GLenum mode) {
    glBindVertexArray(vaoId);
    glDrawElements(mode, bufferSize, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void VertexArrayObject::addMeshFromFile(const std::string& filename) {
    // Load OBJ file.
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());
    if (!err.empty()) {
        std::cerr << "[WARNING] " << err << std::endl;
    }

    if (!success) {
        std::cerr << "Failed to load OBJ file: " << filename << std::endl;
        exit(1);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    for (const auto &s: shapes) {
        for (const auto &index : s.mesh.indices) {
            Vertex vertex;
            if (index.vertex_index >= 0) {
                vertex.position = glm::vec3(
                    attrib.vertices[index.vertex_index * 3 + 0],
                    attrib.vertices[index.vertex_index * 3 + 1],
                    attrib.vertices[index.vertex_index * 3 + 2]
                );
            }

            if (index.texcoord_index >= 0) {
                vertex.texcoord = glm::vec2(
                    attrib.texcoords[index.texcoord_index * 2 + 0],
                    attrib.texcoords[index.texcoord_index * 2 + 1]
                );
            }

            if (index.normal_index >= 0) {
                vertex.normal = glm::vec3(
                    attrib.normals[index.normal_index * 3 + 0],
                    attrib.normals[index.normal_index * 3 + 1],
                    attrib.normals[index.normal_index * 3 + 2]
                );
            }

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = vertices.size();
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    // Prepare VAO.
    glBindVertexArray(vaoId);

    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indices.size(), indices.data(), GL_STATIC_DRAW);
    bufferSize = indices.size();

    glBindVertexArray(0);
}

void VertexArrayObject::destroy() {
    if (vboId != 0) {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }

    if (iboId != 0) {
        glDeleteBuffers(1, &iboId);
        iboId = 0;
    }

    if (vaoId != 0) {
        glDeleteVertexArrays(1, &vaoId);
        vaoId = 0;
    }
}
