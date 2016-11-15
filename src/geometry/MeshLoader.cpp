#include "MeshLoader.hpp"
#include <util/math_util.hpp>
#include <util/paths.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SOIL.h>

#include <map>
#include <set>

namespace pbd {
    namespace MeshLoader {
        static std::vector<Texture> LoadedTextures;

        glm::vec4 convert44(const aiColor4D &vec) {
            return glm::vec4(vec.r, vec.g, vec.b, vec.a);
        }

        glm::vec3 convert33(const aiVector3D &vec) {
            return glm::vec3(vec.x, vec.y, vec.z);
        }

        glm::vec2 convert22(const aiVector2D &vec) {
            return glm::vec2(vec.x, vec.y);
        }

        glm::vec2 convert32(const aiVector3D &vec) {
            return glm::vec2(vec.x, vec.y);
        }

        Texture LoadTextureFromFile(const std::string path, const Texture::Type type) {
            // Check if texture is loaded already
            auto iter = std::find_if(LoadedTextures.cbegin(), LoadedTextures.cend(),
                                     [path](const Texture &loadedTexture) {
                                         return loadedTexture.path == path;
                                     });

            if (iter != LoadedTextures.cend()) {
                return *iter;
            }

            Texture texture;
            const std::string fullpath = RESOURCEPATH("models/" + path);
            texture.ID = SOIL_load_OGL_texture(
                    fullpath.c_str(),
                    SOIL_LOAD_AUTO,
                    SOIL_CREATE_NEW_ID,
                    SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
            );
            if (texture.ID == 0) {
                std::cerr << "SOIL loading error: " << SOIL_last_result() << std::endl;
            }

            texture.path = path;
            texture.type = type;

            OGL_CALL(glBindTexture(GL_TEXTURE_2D, texture.ID));
            OGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
            OGL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

            LoadedTextures.push_back(texture);
            return texture;
        }

        std::vector<Edge> CalcEdges(const std::vector<Triangle> &triangles) {

            typedef unsigned int uint;
            typedef std::pair<uint, uint> edge;

            std::map<edge, std::vector<uint>> edgeTriangles;

            uint minv, medv, maxv;
            edge e0, e1, e2;
            uint triangleID = 0;

            /// Build look-up table for edge->triangles
            for (const auto &tri : triangles) {
                minv = util::min(tri.vertices.x, tri.vertices.y, tri.vertices.z);
                medv = util::median(tri.vertices.x, tri.vertices.y, tri.vertices.z);
                maxv = util::max(tri.vertices.x, tri.vertices.y, tri.vertices.z);

                e0 = std::make_pair(minv, medv);
                e1 = std::make_pair(medv, maxv);
                e2 = std::make_pair(minv, maxv);

                edgeTriangles[e0].push_back(triangleID);
                edgeTriangles[e1].push_back(triangleID);
                edgeTriangles[e2].push_back(triangleID);

                ++triangleID;
            }

            std::vector<Edge> edgevector;

            /// create Edge struct for each "edge" and lookup neighbouring triangles
#define NO_TRIANGLE -1
            Edge newedge;
            uint count1 = 0, count2 = 0;
            for (const auto &entry : edgeTriangles){

                const auto &edge_ = entry.first;
                const auto &tris = entry.second;

                newedge.vertices[0] = edge_.first;
                newedge.vertices[1] = edge_.second;

                assert(tris.size() == 1 || tris.size() == 2);
                newedge.triangles[0] = tris[0];
                newedge.triangles[1] = (tris.size() == 2 ? tris[1] : NO_TRIANGLE);

                edgevector.push_back(newedge);

                if (tris.size() == 1) ++count1; else ++count2;
            }
            std::cout << count1 << ", " << count2 << std::endl;
#undef NO_TRIANGLE

            edgevector.shrink_to_fit();
            return edgevector;
        }

        /**
         * http://stackoverflow.com/questions/8846501/neighbor-polygons-from-list-of-polygon-indices
         */
        std::vector<ClothTriangleData> CalcClothTriangleData(const std::vector<Triangle> &triangles) {
            std::vector<ClothTriangleData> clothTriangleData;
            clothTriangleData.resize(triangles.size());

            typedef unsigned int uint;
            typedef std::pair<uint, uint> edge;

            std::map<edge, std::vector<uint>> edgeTriangles;
            std::map<uint, std::vector<edge>> triangleEdges;

            uint minv, medv, maxv;
            edge e0, e1, e2;
            uint triangleID = 0;

            /// Build look-up table for edge->triangles and triangle->edges
            for (const auto &tri : triangles) {
                minv = util::min(tri.vertices.x, tri.vertices.y, tri.vertices.z);
                medv = util::median(tri.vertices.x, tri.vertices.y, tri.vertices.z);
                maxv = util::max(tri.vertices.x, tri.vertices.y, tri.vertices.z);

                e0 = std::make_pair(minv, medv);
                e1 = std::make_pair(medv, maxv);
                e2 = std::make_pair(minv, maxv);

                edgeTriangles[e0].push_back(triangleID);
                edgeTriangles[e1].push_back(triangleID);
                edgeTriangles[e2].push_back(triangleID);

                triangleEdges[triangleID].push_back(e0);
                triangleEdges[triangleID].push_back(e1);
                triangleEdges[triangleID].push_back(e2);

                ++triangleID;
            }

            triangleID = 0;
            std::vector<uint> sharingTriangles;
            ClothTriangleData data;

            /// Look up the neighbouring triangle for each edge of every triangle
            for (const auto &tri : triangles) {
                data.triangleID = triangleID;

                // Find the sharing triangle for each of this triangle's edges
                for (uint edgeid = 0; edgeid < 3; ++edgeid) {
                    sharingTriangles = edgeTriangles[triangleEdges[triangleID][edgeid]];

                    // The only triangle that uses this edge is itself
                    if (sharingTriangles.size() == 1) {
                        data.neighbourIDs[edgeid] = -1;
                        continue;
                    }

                    // There is exactly one (for manifold meshes) other triangle
                    // that uses this edge
                    if (sharingTriangles[0] != triangleID)
                        data.neighbourIDs[edgeid] = sharingTriangles[0];
                    else data.neighbourIDs[edgeid] = sharingTriangles[1];
                }

                clothTriangleData[triangleID] = data;
                ++triangleID;
            }

#ifndef NDEBUG
            uint count1 = 0, count2 = 0;
            for (const auto &ctdata : clothTriangleData) {
                for (uint i = 0; i < 3; ++i) {
                    const int neighbourID = ctdata.neighbourIDs[i];

                    count1 += 1;
                    if (neighbourID == -1) continue;
                    count1 -= 1;
                    count2 += 1;

                    // assert that the neighbour also has THIS triangle as its neighbour
                    auto neighbour = clothTriangleData[neighbourID];
                    assert(std::find(std::begin(neighbour.neighbourIDs),
                                     std::end(neighbour.neighbourIDs),
                                     ctdata.triangleID)
                           != std::end(neighbour.neighbourIDs));
                }
            }
#endif

            return clothTriangleData;
        }

        std::shared_ptr<Mesh> LoadMesh(const std::string &path) {
            Assimp::Importer importer;

            const aiScene *scene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
            if (!scene || !scene->HasMeshes()) {
                return nullptr;
            }

            const aiMesh *aimesh = scene->mMeshes[0];
            const unsigned int numVertices = aimesh->mNumVertices;
            const unsigned int numTriangles = aimesh->mNumFaces;

            std::vector<Vertex> vertices;
            vertices.resize(numVertices);
            std::vector<Triangle> triangles;
            triangles.resize(numTriangles);

            Vertex vertex;
            vertex.texCoord = glm::vec2(0.0f);
            vertex.color = glm::vec4(1.0f);
            for (uint i = 0; i < numVertices; ++i) {
                vertex.position = convert33(aimesh->mVertices[i]);
                vertex.normal = convert33(aimesh->mNormals[i]);

                if (aimesh->HasTextureCoords(0)) {
                    vertex.texCoord = convert32(aimesh->mTextureCoords[0][i]);
                }

                if (aimesh->HasVertexColors(0)) {
                    vertex.color = convert44(aimesh->mColors[0][i]);
                }

                vertices[i] = vertex;
            }

            Triangle triangle;
            for (uint i = 0; i < numTriangles; ++i) {
                auto &face = aimesh->mFaces[i];
                assert(face.mNumIndices == 3); // since we specified aiProcess_Triangulate
                triangle.vertices.x = face.mIndices[0];
                triangle.vertices.y = face.mIndices[2];
                triangle.vertices.z = face.mIndices[1];

                triangles[i] = triangle;
            }

            auto edges = CalcEdges(triangles);

            auto mesh = std::make_shared<Mesh>(std::move(vertices), std::move(edges), std::move(triangles));

            aiMaterial *material = scene->mMaterials[aimesh->mMaterialIndex];
            aiString texturePath;
            if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
                mesh->mTexDiffuse = LoadTextureFromFile(texturePath.C_Str(), Texture::Type::DIFFUSE);
            }

            if (material->GetTextureCount(aiTextureType_SPECULAR) > 0) {
                material->GetTexture(aiTextureType_SPECULAR, 0, &texturePath);
                mesh->mTexSpecular = LoadTextureFromFile(texturePath.C_Str(), Texture::Type::SPECULAR);
            }

            if (material->GetTextureCount(aiTextureType_HEIGHT) > 0) {
                material->GetTexture(aiTextureType_HEIGHT, 0, &texturePath);
                mesh->mTexBump = LoadTextureFromFile(texturePath.C_Str(), Texture::Type::BUMP);
            }

            importer.FreeScene();
            //mesh->flipNormals();
            return mesh;
        }

        std::shared_ptr<ClothMesh> LoadClothMesh(const std::string &path) {
            auto regularMesh = LoadMesh(path);

            auto clothTriangleData = CalcClothTriangleData(regularMesh->mTriangles);

            std::vector<ClothVertexData> clothVertexData;
            {
                ClothVertexData data;
                data.mass = 0.0f;
                data.mass = 0.0f;
                for (unsigned int i = 0; i < regularMesh->numVertices(); ++i) {
                    data.vertexID = i;
                    clothVertexData.push_back(data);
                }
            }

            std::vector<ClothEdgeData> clothEdgeData;
            {
                ClothEdgeData data;
                data.initialDihedralAngle = 0.0f;
                data.initialLength = 0.0f;
                for (unsigned int i = 0; i < regularMesh->numEdges(); ++i) {
                    data.edgeID = i;
                    clothEdgeData.push_back(data);
                }
            }

            return std::make_shared<ClothMesh>(
                    std::move(*regularMesh),
                    std::move(clothVertexData),
                    std::move(clothEdgeData),
                    std::move(clothTriangleData)
            );
        }
    }
}
