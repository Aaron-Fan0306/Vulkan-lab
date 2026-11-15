#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Vertex//a simple vertex struct :)
{
    glm::vec3 position = glm::vec3(0.f);
    glm::vec3 color = glm::vec3(1.f);
    glm::vec3 normal = glm::vec3(0.f);
    glm::vec2 uv = glm::vec2(0.f);
};

struct SubMesh
{
    size_t vertex_start = 0;
    size_t vertex_count = 0;
    size_t index_start = 0;
    size_t index_count = 0;
    size_t material_id = 0;
};

struct Mesh
{
    std::string name;
    std::vector<SubMesh> sub_meshes;
};

struct Image
{
    int width;
    int height;
    int channels;
    unsigned char *data;
};