// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include "Scene/Skybox.h"
#include "Scene/Camera.h"
#include "Object/Entity.h"
#include "Object/Model.h"
#include "Object/Shader.h"
#include <stdio.h>
#include <glm/gtc/type_ptr.hpp>

#include "Object/Emitter.h"
#include "Object/Particle.h"
#include <iterator>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include <glad/glad.h>  // Initialize with gladLoadGL()

#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions
#include <spdlog/spdlog.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

bool init();
void init_imgui();

void input();
void update();
void render();

void imgui_begin();
void imgui_render();
void imgui_end();

void end_frame();

constexpr int32_t WINDOW_WIDTH  = 1920;
constexpr int32_t WINDOW_HEIGHT = 1080;

GLFWwindow* window = nullptr;

// Change these to lower GL version like 4.5 if GL 4.6 can't be initialized on your machine
const     char*   glsl_version     = "#version 460";
constexpr int32_t GL_VERSION_MAJOR = 4;
constexpr int32_t GL_VERSION_MINOR = 6;

bool   show_demo_window    = true;
bool   show_another_window = false;
ImVec4 clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

std::unique_ptr<Skybox> skybox;

// camera
Camera camera(glm::vec3(0.5f, 0.0f, 0.5f));


// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;



std::unique_ptr<Entity> ballParent;
std::unique_ptr<Entity> mirror;
std::unique_ptr<Entity> lamp;
std::unique_ptr<Entity> tree;
std::unique_ptr<Entity> leaves;


std::unique_ptr<Entity> floorEntity;
std::unique_ptr<Entity> grass;

std::unique_ptr<Shader> shader;
std::unique_ptr<Shader> shadowMapShader;
std::unique_ptr<Shader> pointShadowMapShader;
std::unique_ptr<Shader> reflectionShader;
std::unique_ptr<Shader> refractShader;
std::unique_ptr<Shader> instancedShader;
std::unique_ptr<Shader> lightboxShader;
std::unique_ptr<Shader> blurShader;
std::unique_ptr<Shader> blurShaderFinal;

float childoffsetX{};
float childoffsetY{};
float childoffsetZ{};

float parentOffsetX{};
bool bloom = true;


int NR_POINT_LIGHTS = 1;

unsigned int depthMapFBO;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
unsigned int depthMap;

unsigned int cubeDepthMapFBO;
unsigned int depthCubemap;

glm::mat4* modelMatrices;
glm::mat4* treeModelMatrices;
glm::mat4* leavesModelMatrices;
unsigned int amount = 10000;
unsigned int treeAmount = 200;

unsigned int quadVAO = 0;
unsigned int quadVBO;

Entity* selectedEntity = nullptr; // Default to ballParent

void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderInstancesInit() {
    modelMatrices = new glm::mat4[amount];
    srand(glfwGetTime()); // initialize random seed
    float planeWidth = 20.f; // width of the plane
    float planeHeight = 20.0f; // height of the plane
    float offset = 2.5f; // offset for randomness

    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);

        // 1. translation: distribute evenly on a plane with random offset
        float x = ((rand() % (int)(planeWidth * 100)) / 100.0f) - planeWidth / 2.0f;
        float z = ((rand() % (int)(planeHeight * 100)) / 100.0f) - planeHeight / 2.0f;
        //float y = 0.0f; // all objects lie on the XZ plane
        model = glm::translate(model, glm::vec3(x, 0, z));

        // 2. scale: scale between 0.05 and 0.25f
        float scale = (rand() % 20) / 100.0f + 0.05;
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        //float rotAngle = glm::radians(static_cast<float>(rand() % 360));
       // model = glm::rotate(model, rotAngle, glm::vec3(0, 0, 0));


        // 4. now add to list of matrices
        modelMatrices[i] = model;

    }

}
void renderTreesInstancesInit() {
    treeModelMatrices = new glm::mat4[treeAmount];
    leavesModelMatrices = new glm::mat4[treeAmount];
    srand(glfwGetTime()); // initialize random seed
    float radius = 5.0;
    float offset = 2.5f;

    for (unsigned int i = 0; i < treeAmount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = 0.0f;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        
        model = glm::scale(model, glm::vec3(1.5,2.0,1.5));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = glm::radians(-90.0f); // Convert degrees to radians
        model = glm::rotate(model, rotAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        float rotZAngle = (rand() % 360);
        model = glm::rotate(model, rotZAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        // 4. now add to list of matrices
        treeModelMatrices[i] = model;
        leavesModelMatrices[i] = model;

    }

}

void configureInstancedArray() {
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < grass->meshes.size(); i++)
    {
        unsigned int VAO = grass->meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }
}

void configureTreeInstancedArray() {
    unsigned int treeBuffer;
    glGenBuffers(1, &treeBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, treeBuffer);
    glBufferData(GL_ARRAY_BUFFER, treeAmount * sizeof(glm::mat4), &treeModelMatrices[0], GL_STATIC_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < tree->meshes.size(); i++)
    {
        unsigned int VAO = tree->meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

}

void configureLeavesInstancedArray() {
    unsigned int leavesBuffer;
    glGenBuffers(1, &leavesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, leavesBuffer);
    glBufferData(GL_ARRAY_BUFFER, treeAmount * sizeof(glm::mat4), &leavesModelMatrices[0], GL_STATIC_DRAW);

    // set transformation matrices as an instance vertex attribute (with divisor 1)
    // note: we're cheating a little by taking the, now publicly declared, VAO of the model's mesh(es) and adding new vertexAttribPointers
    // normally you'd want to do this in a more organized fashion, but for learning purposes this will do.
    // -----------------------------------------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < leaves->meshes.size(); i++)
    {
        unsigned int VAO = leaves->meshes[i].VAO;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

}

std::unique_ptr<Shader> particleSpawnShader;
std::unique_ptr<Shader> particleUpdateShader;
std::unique_ptr<Shader> particleRenderShader;
std::unique_ptr <ParticleEmitter> emitter;
void particlesInit()
{
    // create shaders
    particleSpawnShader = std::make_unique<Shader>("res/shaders/particlegen.comp");
    particleUpdateShader = std::make_unique<Shader>("res/shaders/particle.comp");
    particleRenderShader = std::make_unique<Shader>("res/shaders/particle.vert", "res/shaders/particle.frag");

    // initialize SSBOs
    emitter = std::make_unique<ParticleEmitter>();



}
void SetParticleVertexAttributesAndBindings(ParticleEmitter& e) {
    GLuint VAO, VBO, EBO, instanceVBO;

    // Step 1: Create the Vertex Array Object (VAO)
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Step 2: Create the Quad VBO (Static Vertex Data)
    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };
    GLuint quadIndices[] = { 0, 1, 2, 0, 2, 3 }; // Two triangles to form the quad

    // Create and bind the VBO for the quad vertices
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Set vertex attributes for the quad (position and texture coordinates)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);


    // Create and bind the Element Buffer Object (EBO) for the quad indices
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // Step 3: Create the Instance VBO (Dynamic Per-Particle Data)
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, e.maxParticles * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW); // Allocate space for the particle data

}
void sceneSetup() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    ballParent = std::make_unique<Entity>("res/models/TestScene/sphere/sphere.fbx");
    mirror = std::make_unique<Entity>("res/models/TestScene/mirrorFrame/mirrorFrame.fbx");
    lamp= std::make_unique<Entity>("res/models/TestScene/lamp/lamp.fbx");
    floorEntity = std::make_unique<Entity>("res/models/TestScene/ground/ground.fbx");
    grass = std::make_unique<Entity>("res/models/TestScene/grass/grass.fbx");
    tree = std::make_unique<Entity>("res/models/TestScene/tree/tree.fbx");
    leaves = std::make_unique<Entity>("res/models/TestScene/leaves/leaves.fbx");
    
    shader = std::make_unique<Shader>("res/shaders/object.vert", "res/shaders/object.frag");
    shadowMapShader = std::make_unique<Shader>("res/shaders/shadowmap.vert", "res/shaders/shadowmap.frag");
    pointShadowMapShader = std::make_unique<Shader>("res/shaders/pointshadowmap.vert", "res/shaders/pointshadowmap.frag", "res/shaders/pointshadowmap.geom");
    reflectionShader = std::make_unique<Shader>("res/shaders/reflection.vert", "res/shaders/reflection.frag");
    refractShader = std::make_unique<Shader>("res/shaders/reflection.vert", "res/shaders/refract.frag");
    instancedShader = std::make_unique<Shader>("res/shaders/objectInstanced.vert", "res/shaders/object.frag");
    lightboxShader = std::make_unique<Shader>("res/shaders/lightbox.vert", "res/shaders/lightbox.frag");
    blurShader = std::make_unique<Shader>("res/shaders/blur.vert", "res/shaders/blur.frag");
    blurShaderFinal = std::make_unique<Shader>("res/shaders/blurShaderFinal.vert", "res/shaders/blurShaderFinal.frag");

    ballParent->transform.setLocalPosition(glm::vec3(0, 0.8, 0));
    ballParent->transform.setLocalScale(glm::vec3(0.1, 0.1, 0.1));
    
    mirror->transform.setLocalPosition(glm::vec3(0, 0.0, -0.5));
    mirror->transform.setLocalScale(glm::vec3(0.005, 0.005, 0.005));

    lamp->transform.setLocalPosition(glm::vec3(-0.4, 0.8, -0.5));
    lamp->transform.setLocalScale(glm::vec3(0.1, 0.1, 0.1));

    

    leaves->transform.setLocalPosition(glm::vec3(0.0, 0.0, 0.0));
    leaves->transform.setLocalScale(glm::vec3(1.2, 1.2, 1.2));
    leaves->transform.setLocalRotation(glm::vec3(-90, 0, 0));

    ballParent->addChild
    
    ("res/models/TestScene/sphere/sphere.fbx");


    ballParent->children.front()->transform.setLocalPosition(glm::vec3(childoffsetX, childoffsetY, childoffsetZ));
    ballParent->children.front()->transform.setLocalScale(glm::vec3(1,1,1));

    ballParent->updateSelfAndChild();

    floorEntity->transform.setLocalScale(glm::vec3(10.0, 10.0, 10.0));
    floorEntity->transform.setLocalPosition(glm::vec3(0, 0, 0));
    floorEntity->transform.setLocalRotation(glm::vec3(-90, 0, 0));

    //mirror
    floorEntity->addChild("res/models/TestScene/mirrorFrame/mirrorFrame.fbx");
    floorEntity->children.back()->transform.setLocalPosition(glm::vec3(0, 0.0, 0.0));
    floorEntity->children.back()->transform.setLocalScale(glm::vec3(0.0005, 0.0005, 0.0005));
    floorEntity->children.back()->transform.setLocalRotation(glm::vec3(90, 0, 0));

    floorEntity->children.back()->addChild("res/models/TestScene/mirrorFrame/mirrorGlass.fbx");

    //lamp
    floorEntity->addChild("res/models/TestScene/lamp/lamp.fbx");
    floorEntity->children.back()->transform.setLocalPosition(glm::vec3(-0.03, 0.0, 0.08));
    floorEntity->children.back()->transform.setLocalScale(glm::vec3(0.01, 0.01, 0.01));
    floorEntity->children.back()->transform.setLocalRotation(glm::vec3(90, 0, 0));

    floorEntity->children.back()->addChild("res/models/TestScene/lamp/lamp_inside.fbx");


    floorEntity->updateSelfAndChild();

    grass->transform.setLocalScale({ 0.1,0.2,0.1 });
    grass->updateSelfAndChild();

    
    tree->updateSelfAndChild();



   renderInstancesInit();
   renderTreesInstancesInit();
   configureInstancedArray();
   configureTreeInstancedArray();
   configureLeavesInstancedArray();
   particlesInit();


}

void shadowMapFramebufferInit() {
    glGenFramebuffers(1, &depthMapFBO);


    glGenTextures(1, &depthMap);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void pointShadowMapInit() {
    glGenFramebuffers(1, &cubeDepthMapFBO);

    glGenTextures(1, &depthCubemap);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
            SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, cubeDepthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}





void updateAndRenderParticles()
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // particle spawn loop- one dispatch per emitter
    particleSpawnShader->use();
    emitter->timer += deltaTime;


        unsigned particlesToSpawn =  emitter->timer / emitter->spawnInterval;
        emitter->timer = fmod(emitter->timer, emitter->spawnInterval);

        if (particlesToSpawn > 0)
        {


        // sets the uniforms for the u_emitter object from the previous code snippet
        // since there's a bunch of them, they are hidden in this function's definition
            emitter->setUniformSettings(*particleSpawnShader.get());

            particleSpawnShader->setInt("u_particlesToSpawn", particlesToSpawn);


        // run the compute shader
        int workGroupSize = 64;
        int numWorkGroups = (particlesToSpawn + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(numWorkGroups, 1, 1);
        }

    // ensure that the spawned particles are visible in particle updates
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // particle update loop- again, one dispatch per emitter
    particleUpdateShader->use();
    particleUpdateShader->setFloat("u_dt", deltaTime);

        // run the compute shader
    int workGroupSize = 128;
        int numWorkGroups = (emitter->maxParticles + workGroupSize - 1) / workGroupSize;
        glDispatchCompute(numWorkGroups, 1, 1);

        SetParticleVertexAttributesAndBindings(*emitter.get());
        particleRenderShader->use();
        auto v = camera.GetViewMatrix();
        particleRenderShader->setMat4("u_viewProj", glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f) * camera.GetViewMatrix());
        particleRenderShader->setVec3("u_cameraRight", camera.Right);
        particleRenderShader->setVec3("u_cameraUp", { camera.Up});

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,emitter->sprite);
        glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, emitter->maxParticles);
       // glBufferSubData( emitter.get()->particlesBuffer


        
}


int main(int, char**)
{
    if (!init())
    {
        spdlog::error("Failed to initialize project!");
        return EXIT_FAILURE;
    }
    spdlog::info("Initialized project.");

    init_imgui();
    spdlog::info("Initialized ImGui.");

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Process I/O operations here
        input();

        // Update game objects' state here
        update();

        // OpenGL rendering code here
        render();

        // Draw ImGui
        imgui_begin();
        imgui_render(); // edit this function to add your own ImGui controls
        imgui_end(); // this call effectively renders ImGui

        // End frame and swap buffers (double buffering)
        end_frame();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void GLAPIENTRY OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cerr << "[OpenGL Debug] " << message << std::endl;
}

void EnableOpenGLDebug() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLDebugCallback, nullptr);
}

bool init()
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) 
    {
        spdlog::error("Failed to initalize GLFW!");
        return false;
    }

    // GL 4.6 + GLSL 460
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Dear ImGui GLFW+OpenGL4 example", NULL, NULL);
    if (window == NULL)
    {
        spdlog::error("Failed to create GLFW Window!");
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync - fixes FPS at the refresh rate of your screen

    bool err = !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    if (err)
    {
        spdlog::error("Failed to initialize OpenGL loader!");
        return false;
    }

    //EnableOpenGLDebug();


    std::vector<std::string> faces =
    {
        "res/textures/right.jpg",
            "res/textures/left.jpg",
            "res/textures/top.jpg",
            "res/textures/bottom.jpg",
            "res/textures/front.jpg",
            "res/textures/back.jpg"
    };
    skybox = std::make_unique<Skybox>("res/shaders/basic.vert", "res/shaders/basic.frag");
    skybox->prepareCubeMap("res\\models\\TestScene\\skybox.hdr");


    glfwSetCursorPosCallback(window, mouse_callback);


    sceneSetup();

    shadowMapFramebufferInit();
    pointShadowMapInit();

    return true;
}

void init_imgui()
{
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
}



void input()
{
    // I/O ops go here
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;


    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

float rotationAngle = 0.f;
void update()
{
    //ballParent->transform.setLocalPosition(glm::vec3(0, parentOffsetX, 0));


    
        // Update rotation angle over time
        rotationAngle += deltaTime; // Adjust speed by scaling deltaTime if necessary

        // Compute new child position relative to the parent
        float radius = 10.0f; // Distance between parent and child
        float childX = radius * cos(rotationAngle);
        float childZ = radius * sin(rotationAngle);

        // Set the child's local position (rotation in the XZ plane around the parent)
        ballParent->children.front()->transform.setLocalPosition(glm::vec3(childX, 0, childZ));
    

    // Update game objects' state here
}


glm::vec3 dirLightColor{ 1,1,1 };

void renderScene()
{
    //bloom setup
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        if (i == 0)
            glActiveTexture(GL_TEXTURE18);
        else glActiveTexture(GL_TEXTURE19);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
        );
    }

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // Create and attach depth renderbuffer
    unsigned int depthRenderbuffer;
    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);


    //rest of the scene

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    skybox->showSkybox(camera, WINDOW_WIDTH, WINDOW_HEIGHT);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();



    shader->setMat4("projection", projection);
    shader->setMat4("view", view);

    shader->setVec3("dirLight.direction", { -1.0f, -1.0f, 1.0f });
    shader->setVec3("dirLight.ambient", { 1.2f, 1.0f, 1.2f });
    shader->setVec3("dirLight.diffuse",  dirLightColor);
    shader->setVec3("dirLight.specular", { 0.5f, 0.5f, 0.5f });
    shader->setVec3("viewPos", camera.Position);
    
    

    

    std::vector<glm::vec3> pointLightPositions = {
    glm::vec3(1.2f, 1.0f, 2.0f),
    glm::vec3(2.3f, -3.3f, -4.0f),
    glm::vec3(-4.0f, 2.0f, -12.0f),
    glm::vec3(0.0f, 0.0f, -3.0f)
    };

    

    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        std::string base = "pointLights[" + std::to_string(i) + "].";
        shader->setVec3(base + "position", glm::vec3(-0.2f, 0.6f, 0));
        shader->setVec3(base + "ambient", glm::vec3(1.0f, 1.0f, 0.6f));
        shader->setVec3(base + "diffuse", glm::vec3(1.0f, 1.0f, 0.6f));
        shader->setVec3(base + "specular", glm::vec3(1.0f, 1.0f, 0.6f));
        shader->setFloat(base + "constant", 1.0f);
        shader->setFloat(base + "linear", 0.09f);
        shader->setFloat(base + "quadratic", 0.032f);
        
        
    }

    shader->setVec3("spotLight.position", camera.Position);
    shader->setVec3("spotLight.direction", camera.Front);
    shader->setVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.2f));
    shader->setVec3("spotLight.diffuse", glm::vec3(0.0f, 0.0f, 1.0f));
    shader->setVec3("spotLight.specular", glm::vec3(0.5f, 0.5f, 1.0f));
    shader->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
    shader->setFloat("spotLight.constant", 1.0f);
    shader->setFloat("spotLight.linear", 0.09f);
    shader->setFloat("spotLight.quadratic", 0.032f);
    
    


    shader->setFloat("material.shininess", 16.0f);
    shader->setInt("irradianceMap", 13);
    shader->setInt("prefilterMap", 14);
    shader->setInt("brdfLUT", 7);
    


    floorEntity->Draw(*shader.get());

    for (auto& child : floorEntity->children) {
        child->Draw(*shader.get());
        
    }
    /*mirror->Draw(*shader.get());
    lamp->Draw(*shader.get());*/
    /*tree->Draw(*shader.get());
    leaves->Draw(*shader.get());*/

    
    


    lightboxShader->use();
    lightboxShader->setMat4("projection", projection);
    lightboxShader->setMat4("view", view);
    lightboxShader->setVec3("lightColor", glm::vec3(5.0f, 5.0f, 5.0f));

    /*ballParent->Draw(*shader.get());*/
    /*ballParent->Draw(*reflectionShader.get());*/
    /*ballParent->Draw(*refractShader.get());*/
    /*ballParent->Draw(*lightboxShader.get());*/
    
    floorEntity->children.front()->children.front()->Draw(*reflectionShader.get());
    auto it = floorEntity->children.begin();
    std::advance(it, 1);
    it->get()->children.front()->Draw(*lightboxShader.get());

    instancedShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, grass->meshes[0].textures[0].id);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, grass->meshes[0].textures[1].id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, grass->meshes[0].textures[2].id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, grass->meshes[0].textures[3].id);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, skybox->getbrdfLUTTexture());

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, grass->meshes[0].textures[4].id);


    instancedShader->setMat4("projection", projection);
    instancedShader->setMat4("view", view);

    instancedShader->setInt("material.albedoMap", 0);
    instancedShader->setInt("material.aoMap", 1);
    instancedShader->setInt("material.metallicMap", 2);
    instancedShader->setInt("material.normalMap", 3);
    instancedShader->setInt("material.roughnessMap", 4);

    instancedShader->setFloat("material.shininess", 16.0f);
    instancedShader->setInt("irradianceMap", 13);
    instancedShader->setInt("prefilterMap", 14);
    instancedShader->setInt("brdfLUT", 7);

    instancedShader->setVec3("dirLight.direction", { -1.0f, -1.0f, 1.0f });
    instancedShader->setVec3("dirLight.ambient", { 1.2f, 1.0f, 1.2f });
    instancedShader->setVec3("dirLight.diffuse", dirLightColor);
    instancedShader->setVec3("dirLight.specular", { 0.5f, 0.5f, 0.5f });
    instancedShader->setVec3("viewPos", camera.Position);

    std::string base = "pointLights[" + std::to_string(0) + "].";
    instancedShader->setVec3(base + "position", glm::vec3(-0.2f, 0.6f, 0));
    instancedShader->setVec3(base + "ambient", glm::vec3(1.0f, 1.0f, 0.6f));  // Light yellow ambient
    instancedShader->setVec3(base + "diffuse", glm::vec3(1.0f, 1.0f, 0.6f));  // Light yellow diffuse
    instancedShader->setVec3(base + "specular", glm::vec3(1.0f, 1.0f, 0.6f)); // Light yellow specular
    instancedShader->setFloat(base + "constant", 1.0f);
    instancedShader->setFloat(base + "linear", 0.09f);
    instancedShader->setFloat(base + "quadratic", 0.032f);

        instancedShader->setVec3("spotLight.position", camera.Position);
    instancedShader->setVec3("spotLight.direction", camera.Front);
    instancedShader->setVec3("spotLight.ambient", glm::vec3(0.0f, 0.0f, 0.2f));
    instancedShader->setVec3("spotLight.diffuse", glm::vec3(0.0f, 0.0f, 1.0f));
    instancedShader->setVec3("spotLight.specular", glm::vec3(0.5f, 0.5f, 1.0f));
    instancedShader->setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    instancedShader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
    instancedShader->setFloat("spotLight.constant", 1.0f);
    instancedShader->setFloat("spotLight.linear", 0.09f);
    instancedShader->setFloat("spotLight.quadratic", 0.032f);

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    instancedShader->setInt("shadowMap", 12);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    instancedShader->setInt("depthMap", 5);


    
    for (unsigned int i = 0; i < grass->meshes.size(); i++)
    {
        glBindVertexArray(grass->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, grass->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount
        );
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tree->meshes[0].textures[0].id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tree->meshes[0].textures[1].id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tree->meshes[0].textures[2].id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, tree->meshes[0].textures[3].id);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, skybox->getbrdfLUTTexture());

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, tree->meshes[0].textures[4].id);

    for (unsigned int i = 0; i < tree->meshes.size(); i++)
    {
        glBindVertexArray(tree->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, tree->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leaves->meshes[0].textures[0].id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, leaves->meshes[0].textures[1].id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, leaves->meshes[0].textures[2].id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, leaves->meshes[0].textures[3].id);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, skybox->getbrdfLUTTexture());

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, leaves->meshes[0].textures[4].id);

    for (unsigned int i = 0; i < leaves->meshes.size(); i++)
    {
        glBindVertexArray(leaves->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, leaves->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }

    updateAndRenderParticles();



    unsigned int pingpongFBO[2];
    unsigned int pingpongBuffer[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        if (i == 0)
            glActiveTexture(GL_TEXTURE20);
        else glActiveTexture(GL_TEXTURE21);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RGBA16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
        );
    }

    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10;
    blurShader->use();
    blurShader->setInt("image", 19);

    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
        blurShader->setInt("horizontal", horizontal);
        glActiveTexture(GL_TEXTURE19);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongBuffer[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)


        //render quad
        renderQuad();
        //end of render quad
        horizontal = !horizontal;
        if (first_iteration)
            first_iteration = false;


    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    blurShaderFinal->use();
    blurShaderFinal->setInt("scene", 18);
    blurShaderFinal->setInt("bloomBlur", 19);
    blurShaderFinal->setFloat("exposure", 1.0f);
    blurShaderFinal->setBool("bloom", bloom);
    renderQuad();

    glDeleteRenderbuffers(1, &depthRenderbuffer);
    glDeleteTextures(2, colorBuffers);
    glDeleteFramebuffers(1, &hdrFBO);
    glDeleteTextures(2, pingpongBuffer);
    glDeleteFramebuffers(2, pingpongFBO);

}


void render() {
    
    //DIRECTIONAL SHADOW MAP
    float near_plane = 1.0f, far_plane = 7.5f;
    glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);

    glm::mat4 lightView = glm::lookAt(glm::vec3(-5.0f, 5.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 lightSpaceMatrix = lightProjection * lightView;
    //pass 1
    shadowMapShader->use();
    shadowMapShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    shadowMapShader->setBool("useInstanceMatrix", true);

    for (unsigned int i = 0; i < grass->meshes.size(); i++)
    {
        glBindVertexArray(grass->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, grass->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount
        );
    }
    for (unsigned int i = 0; i < tree->meshes.size(); i++)
    {
        glBindVertexArray(tree->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, tree->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }
    for (unsigned int i = 0; i < leaves->meshes.size(); i++)
    {
        glBindVertexArray(leaves->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, leaves->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }
    shadowMapShader->setBool("useInstanceMatrix", false);
    
    for (auto& child : floorEntity->children) {
        child->Draw(*shadowMapShader.get());

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //POINT SHADOW MAP
    float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
    float Near = 1.0f;
    float Far = 25.0f;
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, Near, Far);
    
    glm::vec3 lightPos(-0.2f, 1.1f, 0.05f);

    std::vector<glm::mat4> shadowTransforms;

    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0))); // +X
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0))); // -X
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0))); // +Y
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0))); // -Y
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 1.0, 0.0))); // +Z
    shadowTransforms.push_back(shadowProj *
        glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0))); // -Z

    //pass 1
    pointShadowMapShader->use();
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, cubeDepthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    for (unsigned int i = 0; i < 6; ++i)
        pointShadowMapShader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
    pointShadowMapShader->setFloat("far_plane", Far);
    pointShadowMapShader->setVec3("lightPos", lightPos);
    pointShadowMapShader->setBool("useInstanceMatrix", true);
    for (unsigned int i = 0; i < grass->meshes.size(); i++)
    {
        glBindVertexArray(grass->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, grass->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, amount
        );
    }
    for (unsigned int i = 0; i < tree->meshes.size(); i++)
    {
        glBindVertexArray(tree->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, tree->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }
    for (unsigned int i = 0; i < leaves->meshes.size(); i++)
    {
        glBindVertexArray(leaves->meshes[i].VAO);
        glDrawElementsInstanced(
            GL_TRIANGLES, leaves->meshes[i].indices.size(), GL_UNSIGNED_INT, 0, treeAmount
        );
    }
    pointShadowMapShader->setBool("useInstanceMatrix", false);
    for (auto& child : floorEntity->children) {
        child->Draw(*pointShadowMapShader.get());

    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //pass 2
    instancedShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    shader->setFloat("far_plane", Far);
    shader->use();
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    shader->setInt("shadowMap", 12);    
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    shader->setInt("depthMap", 5);

    reflectionShader->use();

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    reflectionShader->setMat4("projection", projection);
    glm::mat4 view = glm::mat4(camera.GetViewMatrix());
    reflectionShader->setMat4("view", view);
    reflectionShader->setInt("skybox", 0);

    refractShader->use();
    refractShader->setMat4("projection", projection);
    refractShader->setMat4("view", view);
    refractShader->setInt("skybox", 0);
    
    shader->use();
    renderScene();
}

void imgui_begin()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


void imgui_render()
{

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        ImGui::Begin("controls");                          // Create a window called "Hello, world!" and append into it.


        ImGui::SliderFloat("x", &childoffsetX, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("y", &childoffsetY, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::SliderFloat("z", &childoffsetZ, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        ImGui::SliderFloat("parentX", &parentOffsetX, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::Checkbox("blur", &bloom);
        ImGui::SliderFloat3("dir light color", glm::value_ptr(dirLightColor), 0, 100);



        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

}





void imgui_end()
{
    ImGui::Render();
    int display_w, display_h;
    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &display_w, &display_h);

    glViewport(0, 0, display_w, display_h);
   

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void end_frame()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);
}






