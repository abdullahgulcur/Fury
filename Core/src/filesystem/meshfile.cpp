#include "pch.h"
#include "meshfile.h"
#include "core.h"

namespace Fury {

	MeshFile::MeshFile(File* file) {

        MeshFile::loadModel(file);
	}

    MeshFile::~MeshFile() {

        FileSystem* fileSystem = Core::instance->fileSystem;
        GlewContext* glewContext = Core::instance->glewContext;
        File* file = fileSystem->meshFileToFile[this];
        std::vector<File*>& meshFiles = fileSystem->meshFiles;

        // Release mesh renderer component mesh file dependencies
        std::vector<MeshRenderer*>& components = fileSystem->fileToMeshRendererComponents[file];
        for (auto& comp : components)
            comp->meshFile = NULL;
        fileSystem->fileToMeshRendererComponents.erase(file);

        glewContext->deleteRenderBuffers(1, &fileIconRBO);
        glewContext->deleteTextures(1, &fileTextureId);
        glewContext->deleteFrameBuffers(1, &fileIconFBO);
        glewContext->deleteVertexArrays(1, &VAO);

        meshFiles.erase(std::remove(meshFiles.begin(), meshFiles.end(), file), meshFiles.end());
        fileSystem->fileToMeshFile.erase(file);
        fileSystem->meshFileToFile.erase(this);
    }

    // ref: https://learnopengl.com/Model-Loading/Model
    void MeshFile::loadModel(File* file)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(file->path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }
        
        // process ASSIMP's root node recursively
        processNode(file, scene->mRootNode, scene);
    }

    // ref: https://learnopengl.com/Model-Loading/Model
    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void MeshFile::processNode(File* file, aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* aimesh = scene->mMeshes[node->mMeshes[i]];
            MeshFile::processMesh(file, aimesh, scene);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(file, node->mChildren[i], scene);
        }
    }

    // ref: https://learnopengl.com/Model-Loading/Model
    void MeshFile::processMesh(File* file, aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        /* For AABB Box boundaries */
        int minX = 99999;
        int minY = 99999;
        int minZ = 99999;
        int maxX = -99999;
        int maxY = -99999;
        int maxZ = -99999;

#ifdef EDITOR_MODE
        float radius = -1.f;
#endif
        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.position = vector;

#ifdef EDITOR_MODE
            float r = glm::length(vector);
            if (r > radius)
                radius = r;
#endif
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.normal = vector;
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.texCoord = vec;
            }
            else
                vertex.texCoord = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);

            /* Calculate AABB Box boundaries */
            if (vector.x > maxX)
                maxX = vector.x;

            if (vector.y > maxY)
                maxY = vector.y;

            if (vector.z > maxZ)
                maxZ = vector.z;

            if (vector.x < minX)
                minX = vector.x;

            if (vector.y < minY)
                minY = vector.y;

            if (vector.z < minZ)
                minZ = vector.z;
        }

        /* Set Box boundaries */
        aabbBox.start.x = minX;
        aabbBox.start.y = minY;
        aabbBox.start.z = minZ;
        aabbBox.start.w = 1.f;
        aabbBox.end.x = maxX;
        aabbBox.end.y = maxY;
        aabbBox.end.z = maxZ;
        aabbBox.end.w = 1.f;

        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Create VAO for actual mesh
        Core::instance->glewContext->initVAO(VAO, vertices, indices);
        indiceCount = indices.size();

#ifdef EDITOR_MODE
        // Create FBO for file texture id
        MeshFile::createFileIcon(file, radius);
#endif
    }

#ifdef EDITOR_MODE
    void MeshFile::createFileIcon(File* file, float radius) {

        GlewContext* glewContext = Core::instance->glewContext;
        FileSystem* fileSystem = Core::instance->fileSystem;
        glewContext->createFrameBuffer(fileIconFBO, fileIconRBO, fileTextureId, 64, 64);

        float z = (radius * 1.2f);// *glm::sin(3.14f / 4);
        float y = (radius * 1.5f) * glm::sin(3.14f / 4);
        float x = (radius * 1.5f) * glm::cos(3.14f / 4);

        glm::vec3 camPos(z, -z, z);
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

        glewContext->bindFrameBuffer(fileIconFBO);
        glewContext->viewport(64, 64);
        glewContext->clearScreen(glm::vec3(0.12f, 0.12f, 0.12f));

        unsigned int pbrShaderProgramId = fileSystem->pbrMaterial->pbrShaderProgramId_old;
        glewContext->useProgram(pbrShaderProgramId);
        glewContext->setVec3(pbrShaderProgramId, "camPos", camPos);
        glewContext->setMat4(pbrShaderProgramId, "PV", projection * view);
        glewContext->setMat4(pbrShaderProgramId, "model", glm::mat4(1));

        unsigned int whiteTextureId = fileSystem->whiteTexture->textureId;
        unsigned int blackTextureId = fileSystem->blackTexture->textureId;

        // albedo
        glewContext->activeTex(0);
        glewContext->bindTex(whiteTextureId);
        glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(0), 0);

        // normal
        glewContext->activeTex(1);
        glewContext->bindTex(fileSystem->flatNormalMapTexture->textureId);
        glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(1), 1);

        // metallic
        glewContext->activeTex(2);
        glewContext->bindTex(blackTextureId);
        glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(2), 2);

        // roughness
        glewContext->activeTex(3);
        glewContext->bindTex(blackTextureId);
        glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(3), 3);

        // ao
        glewContext->activeTex(4);
        glewContext->bindTex(whiteTextureId);
        glewContext->setInt(pbrShaderProgramId, "texture" + std::to_string(4), 4);

        glewContext->bindVertexArray(VAO);
        glewContext->drawElements(0x0004, indiceCount, 0x1405, (void*)0);
        glewContext->bindVertexArray(0);
        glewContext->bindFrameBuffer(0);
        file->textureID = fileTextureId;
    }
#endif

}