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

        // Release mesh renderer component mesh file dependencies
        std::vector<MeshRenderer*>& components = fileSystem->fileToMeshRendererComponents[file];
        for (auto& comp : components)
            comp->meshFile = NULL;
        fileSystem->fileToMeshRendererComponents.erase(file);

        glewContext->deleteRenderBuffers(1, &RBO);
        glewContext->deleteTextures(1, &fileTextureId);
        glewContext->deleteFrameBuffers(1, &FBO);
        glewContext->deleteVertexArrays(1, &VAO);

        fileSystem->meshFiles.erase(std::remove(fileSystem->meshFiles.begin(), fileSystem->meshFiles.end(), file), fileSystem->meshFiles.end());
        fileSystem->fileToMeshFile.erase(file);
        fileSystem->meshFileToFile.erase(this);
    }

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
        // retrieve the directory path of the filepath
        //directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(file, scene->mRootNode, scene);
    }

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

    void MeshFile::processMesh(File* file, aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<glm::vec3> points;
        std::vector<unsigned int> indicesWireframe;


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

            float radius = glm::length(vector);
            if (radius > this->radius) this->radius = radius;

            points.push_back(vector);
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
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        //-- Create VAO for actual mesh
        Core::instance->glewContext->initVAO(VAO, vertices, indices);
        indiceCount = indices.size();

        //-- Create wireframe VAO

        for (int i = 0; i < indices.size(); i += 3) {

            indicesWireframe.push_back(indices[i]);
            indicesWireframe.push_back(indices[i + 1]);
            indicesWireframe.push_back(indices[i + 1]);
            indicesWireframe.push_back(indices[i + 2]);
            indicesWireframe.push_back(indices[i + 2]);
            indicesWireframe.push_back(indices[i]);
        }
        //wireframeIndiceCount = indicesWireframe.size();

        //unsigned int VBO;
        //unsigned int EBO;
        //core->glewContext->genVertexArrays(1, &wireframeVAO);
        //core->glewContext->genBuffers(1, &VBO);
        //core->glewContext->genBuffers(1, &EBO);
        //core->glewContext->bindVertexArray(wireframeVAO);
        //core->glewContext->bindBuffer(0x8892, VBO);
        //core->glewContext->bufferData(0x8892, points.size() * sizeof(glm::vec3), (void*)&points[0], 0x88E4);
        //core->glewContext->bindBuffer(0x8893, EBO);
        //core->glewContext->bufferData(0x8893, wireframeIndiceCount * sizeof(unsigned int), (void*)&indicesWireframe[0], 0x88E4);
        //core->glewContext->enableVertexAttribArray(0);
        //core->glewContext->vertexAttribPointer(0, 3, 0x1406, 0, sizeof(glm::vec3), (void*)0);
        //core->glewContext->bindVertexArray(0);

        //-- Create FBO for file texture id
        MeshFile::createFBO(file);
    }

    void MeshFile::createFBO(File* file) {

        GlewContext* glewContext = Core::instance->glewContext;
        glewContext->createFrameBuffer(FBO, RBO, fileTextureId, 64, 64);

        float z = (radius * 1.2f);// *glm::sin(3.14f / 4);
        float y = (radius * 1.5f) * glm::sin(3.14f / 4);
        float x = (radius * 1.5f) * glm::cos(3.14f / 4);

        glm::vec3 camPos(z, -z, z);
        glm::mat4 view = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 projection = glm::perspective(glm::radians(60.f), 1.f, 0.01f, 100.f);

        glewContext->bindFrameBuffer(FBO);
        glewContext->viewport(64, 64);
        glewContext->clearScreen(glm::vec3(0.12f, 0.12f, 0.12f));

        unsigned int programId = Core::instance->fileSystem->pbrMaterialNoTexture->programId;
        glewContext->useProgram(programId);
        glewContext->setVec3(programId, "camPos", camPos);
        glewContext->setMat4(programId, "PV", projection * view);
        glewContext->setMat4(programId, "model", glm::mat4(1));

        glewContext->bindVertexArray(VAO);
        glewContext->drawElements(0x0004, indiceCount, 0x1405, (void*)0);
        glewContext->bindVertexArray(0);
        glewContext->bindFrameBuffer(0);
        file->textureID = fileTextureId;
    }

}