#include "ClothSimulationScene.hpp"
#include "SceneSetup.hpp"

#include <iomanip>

#include <util/OCL_CALL.hpp>
#include <util/math_util.hpp>
#include <util/paths.hpp>

#include <geometry/MeshLoader.hpp>
#include <rendering/MeshObject.hpp>

#include <util/cl_util.hpp>

#include <glm/ext.hpp>
#include <OpenCL/opencl.h>

#define ENQUEUE(kernelptr, clothptr, what) \
        OCL_CALL(mQueue.enqueueNDRangeKernel(*kernelptr, cl::NullRange, \
cl::NDRange(clothptr->what()), cl::NullRange));

#define ENQUEUE_VERTICES(kernelptr, clothptr)   ENQUEUE(kernelptr, clothptr, numVertices)
#define ENQUEUE_EDGES(kernelptr, clothptr)      ENQUEUE(kernelptr, clothptr, numEdges)
#define ENQUEUE_TRIANGLES(kernelptr, clothptr)  ENQUEUE(kernelptr, clothptr, numTriangles)

namespace pbd {
    ClothSimulationScene::ClothSimulationScene(cl::Context &context, cl::Device &device, cl::CommandQueue &queue)
            : BaseScene(context, device, queue) {
        mCurrentSetupFile = RESOURCEPATH("setups/simple.json");
        mParams = ClothSimParams::ReadFromFile(RESOURCEPATH("params/default.json"));
        createCamera();
        createAxis();
        loadMarker();
        loadKernels();

        mGridCL = util::make_unique<pbd::Grid>();
        mGridCL->halfDimensions = {1.0f, 1.0f, 1.0f, 0.0f};
        mGridCL->binSize = 0.1f;
        mGridCL->binCount3D = {16, 20, 20, 0};
        mGridCL->binCount = 16 * 20 * 20;

        OCL_ERROR;

        OCL_CHECK(mBinCountCL = util::make_unique<cl::Buffer>(mContext,
                                                              CL_MEM_READ_WRITE,
                                                              sizeof(cl_uint) * mGridCL->binCount,
                                                              (void*)0, CL_ERROR));
        OCL_CHECK(mBinStartIDCL = util::make_unique<cl::Buffer>(mContext,
                                                                CL_MEM_READ_WRITE,
                                                                sizeof(cl_uint) * mGridCL->binCount,
                                                                (void*)0, CL_ERROR));

        mIsGrabbingCloth = false;
    }

    void ClothSimulationScene::addGUI(nanogui::Screen *screen) {
        auto size = screen->size();

        mCamera->setScreenDimensions(glm::uvec2(size[0], size[1]));
        mCamera->setClipPlanes(0.01f, 100.f);

        using namespace nanogui;
        Window *win = new Window(screen, "Scene Controls");
        win->setPosition(Eigen::Vector2i(15, 125));
        win->setLayout(new GroupLayout());

        mErrorLabel = new Label(win, "Status:");

        Button *b = new Button(win, "Reload shaders");
        b->setCallback([this]() {
            loadShaders();
        });
        b = new Button(win, "Reload kernels");
        b->setCallback([this]() {
            loadKernels();
        });

        b = new Button(win, "Load setup");
        b->setCallback([&](){
            const std::string filename = file_dialog({ {"json", "Javascript Object Notation"},
                                                       {"json", "Javascript Object Notation"} }, false);

            mCurrentSetupFile = filename;
            reset();
        });

        /// FPS Labels
        mLabelFrameNumber = new Label(win, "Current frame: 0");
        mLabelAverageFrameTime = new Label(win, "");
        mLabelFPS = new Label(win, "");
        updateTimeLabelsInGUI(0.0);

        /// Cloth simulation parameters GUI
        FormHelper *gui = new FormHelper(screen);
        gui->addWindow(Eigen::Vector2i(screen->width() - 200, 15), "Cloth parameters");

        gui->addButton("Load", [&, gui] {
            std::string filename = file_dialog({ {"json", "Javascript Object Notation"},
                                                 {"json", "Javascript Object Notation"} }, false);
            mParams = ClothSimParams::ReadFromFile(filename);
            gui->refresh();
        });
        gui->addButton("Save", [&, gui] {
            std::string filename = file_dialog({ {"json", "Javascript Object Notation"},
                                                 {"json", "Javascript Object Notation"} }, true);
            mParams.writeToFile(filename);
        });

        gui->addVariable("Projection steps", mParams.numSubSteps);
        gui->addVariable("Delta time (s)", mParams.deltaTime);
        gui->addVariable("Stretch constant", mParams.k_stretch);
        gui->addVariable("Bend constant", mParams.k_bend);
    }

    void ClothSimulationScene::reset() {
        while (!mSimulationTimes.empty()) mSimulationTimes.pop_back();
        updateTimeLabelsInGUI(0.0);

        mFrameCounter = 0;
        mMemObjects.clear();
        mShaders.clear();
        mRenderObjects.clear();
        mClothMeshes.clear();
        mLights.clear();
        loadSetup();
    }

    void ClothSimulationScene::update() {
        /// Rotate camera with keys
        glm::vec3 eulerAngles = mCameraRotator->getEulerAngles();
        if (isKeyDown(GLFW_KEY_A)) eulerAngles.y += 0.04f;
        if (isKeyDown(GLFW_KEY_D)) eulerAngles.y -= 0.04f;
        if (isKeyDown(GLFW_KEY_S)) eulerAngles.x -= 0.04f;
        if (isKeyDown(GLFW_KEY_W)) eulerAngles.x += 0.04f;
        eulerAngles.x = util::clamp(eulerAngles.x, - CL_M_PI_F / 2, CL_M_PI_F / 2);
        mCameraRotator->setEulerAngles(eulerAngles);

        double timeBegin = glfwGetTime();
        if (mFramesSinceLastUpdate == 0) {
            mTimeOfLastUpdate = timeBegin;
        }

        ++mFramesSinceLastUpdate;

        cl::Event event;
        OCL_CALL(mQueue.enqueueAcquireGLObjects(&mMemObjects));

        /// Update simulation
        /// ...
        /// ...

        // if is grabbing cloth, move cloth vertex toward cursor ray
        if (mIsGrabbingCloth) {
            const glm::vec3 rayOrigin = getCameraWorldPosition();
            const glm::vec3 rayDirection = getCursorWorldRay();
            const float fraction = 0.01f;

            cl_float3 velocityCL = {0.0f, 0.0f, 0.0f, 0.0f};
            Vertex vertex;
            mQueue.enqueueReadBuffer(mGrabbedClothMesh->mVertexBufferCL, true,
                                                 sizeof(Vertex) * mGrabbedVertexIndex, sizeof(Vertex), &vertex);

            glm::vec3 position = vertex.position;
            position.y = -position.y;

            glm::vec3 relposition = position - rayOrigin;
            glm::vec3 positionParallelRay = glm::dot(relposition, rayDirection) * rayDirection;
            glm::vec3 projectedPosition = positionParallelRay + rayOrigin;

#define FRACTION_NEW 0.8f
#define FRACTION_OLD (1.0f - FRACTION_NEW)

            glm::vec3 newPosition = FRACTION_OLD * position + FRACTION_NEW * projectedPosition;

            // apply impulse toward cursor ray
            vertex.position.x = newPosition.x;
            vertex.position.y = -newPosition.y;
            vertex.position.z = newPosition.z;

            mQueue.enqueueWriteBuffer(mGrabbedClothMesh->mVertexBufferCL, true,
                                      sizeof(Vertex) * mGrabbedVertexIndex, sizeof(Vertex), &vertex);
            mQueue.enqueueWriteBuffer(mGrabbedClothMesh->mVertexVelocitiesBufferCL, true,
                                      sizeof(cl_float3) * mGrabbedVertexIndex, sizeof(cl_float3), &velocityCL);
        }

        /// apply gravity and predict positions
        for (auto clothmesh : mClothMeshes) {
            OCL_CALL(mPredictPositions->setArg(0, clothmesh->mVertexPredictedPositionsBufferCL));
            OCL_CALL(mPredictPositions->setArg(1, clothmesh->mVertexVelocitiesBufferCL));
            OCL_CALL(mPredictPositions->setArg(2, clothmesh->mVertexBufferCL));
            OCL_CALL(mPredictPositions->setArg(3, clothmesh->mVertexClothBufferCL));
            OCL_CALL(mPredictPositions->setArg(4, mParams.deltaTime));
            ENQUEUE_VERTICES(mPredictPositions, clothmesh);
        }

        /// do a number of position-level update iterations
        for (uint iter = 0; iter < mParams.numSubSteps; ++iter) {

            /// update every clothmesh independently
            for (auto clothmesh : mClothMeshes) {

                /// clip cloth vertices to be above ground plane
                mClipToPlanes->setArg(0, clothmesh->mVertexPredictedPositionsBufferCL);
                ENQUEUE_VERTICES(mClipToPlanes, clothmesh);

                /// calculate position correction based on cloth stretch/bend constraints
                OCL_CALL(mCalcPositionCorrections->setArg(0, clothmesh->mVertexBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(1, clothmesh->mVertexClothBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(2, clothmesh->mEdgeBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(3, clothmesh->mEdgeClothBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(4, clothmesh->mTriangleBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(5, clothmesh->mTriangleClothBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(6, clothmesh->mVertexPredictedPositionsBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(7, clothmesh->mVertexPositionCorrectionsBufferCL));
                OCL_CALL(mCalcPositionCorrections->setArg(8, sizeof(ClothSimParams), (const void *) &mParams));
                ENQUEUE_EDGES(mCalcPositionCorrections, clothmesh);

                /// update predictions based on the corrections
                OCL_CALL(mCorrectPredictions->setArg(0, clothmesh->mVertexPositionCorrectionsBufferCL));
                OCL_CALL(mCorrectPredictions->setArg(1, clothmesh->mVertexPredictedPositionsBufferCL));
                ENQUEUE_VERTICES(mCorrectPredictions, clothmesh);
            }
        }

        /// write predicted/corrected position to actual position
        for (auto clothmesh : mClothMeshes) {
            OCL_CALL(mSetPositionsToPredicted->setArg(0, clothmesh->mVertexPredictedPositionsBufferCL));
            OCL_CALL(mSetPositionsToPredicted->setArg(1, clothmesh->mVertexBufferCL));
            OCL_CALL(mSetPositionsToPredicted->setArg(2, clothmesh->mVertexVelocitiesBufferCL));
            OCL_CALL(mSetPositionsToPredicted->setArg(3, mParams.deltaTime));
            ENQUEUE_VERTICES(mSetPositionsToPredicted, clothmesh);
        }

        OCL_CALL(mQueue.enqueueReleaseGLObjects(&mMemObjects, NULL, &event));
        OCL_CALL(event.wait());

        double timeEnd = glfwGetTime();
        while (mSimulationTimes.size() > NUM_AVG_SIM_TIMES) {
            mSimulationTimes.pop_back();
        }
        mSimulationTimes.push_front(timeEnd - timeBegin);
        ++mFrameCounter;

        // update GUI twice each second
        if (timeEnd - mTimeOfLastUpdate > 2.0f) {
            updateTimeLabelsInGUI(timeEnd - mTimeOfLastUpdate);
            mFramesSinceLastUpdate = 0;
        }

        std::stringstream ss;
        ss << "Frame: " << mFrameCounter;
        mLabelFrameNumber->setCaption(ss.str());
    }

    void ClothSimulationScene::render() {
        const glm::mat4 VP = mCamera->getPerspectiveTransform() * glm::inverse(mCamera->getTransform());
        const glm::vec4 WorldEye = mCamera->getParent()->getTransform() * glm::vec4(mCamera->getPosition(), 1.0f);

        for (auto shaderpair : mShaders) {
            auto &shader = shaderpair.second;

            shader->use();
            shader->uniform("WorldEye", glm::vec3(WorldEye));
            shader->uniform("shininess", 1.0f);
            for (auto light : mLights) {
                light->setUniformsInShader(shader);
            }
        }

        OGL_CALL(glEnable(GL_DEPTH_TEST));
        OGL_CALL(glEnable(GL_CULL_FACE));
        OGL_CALL(glCullFace(GL_FRONT));
        for (auto renderObject : mRenderObjects) {
            renderObject->render(VP);
        }
        if (mIsGrabbingCloth) {
            Vertex vertex;
            mQueue.enqueueReadBuffer(mGrabbedClothMesh->mVertexBufferCL, true, sizeof(Vertex) * mGrabbedVertexIndex, sizeof(Vertex), &vertex);
            auto position = vertex.position;
            position.y = -position.y;
            mMarker->setPosition(position);
            mMarker->render(VP);
        }

        OGL_CALL(glDisable(GL_DEPTH_TEST));

        renderAxes();
    }

    void ClothSimulationScene::renderAxes() {
        OGL_CALL(glEnable(GL_LINE_SMOOTH));
        mAxis->bind();
        mAxisShader->use();
        mAxisShader->uniform("VP", mCamera->getPerspectiveTransform() * glm::inverse(glm::toMat4(mCameraRotator->getOrientation())));
        OGL_CALL(glDrawArrays(GL_LINES, 0, 6));
        mAxis->unbind();
    }

    bool ClothSimulationScene::mouseButtonEvent(const glm::ivec2 &p, int button, bool down, int modifiers) {
        if (button != GLFW_MOUSE_BUTTON_LEFT) {
            return false;
        }

        if (down && modifiers == GLFW_MOD_ALT) {
            mIsRotatingCamera = true;
            return true;
        }

        if (!down) {
            mIsRotatingCamera = false;
            mIsGrabbingCloth = false;
            return false;
        }

        const glm::vec3 rayWorld = getCursorWorldRay();
        const glm::vec3 rayOrigin = getCameraWorldPosition();

        std::cout << "RayWorld = " << glm::to_string(rayWorld) << std::endl
                  << "RayOrigin = " << glm::to_string(rayOrigin) << std::endl;

        cl_float3 rayWorldCL, rayOriginCL;
        for (uint i = 0; i < 3; ++i) {
            rayWorldCL.s[i] = rayWorld[i];
            rayOriginCL.s[i] = rayOrigin[i];
        }

        std::shared_ptr<ClothMesh> closestcloth = nullptr;
        std::vector<cl_float> distances;
        int closestindex = -1;
        float closestdistance = 1e10;

        for (auto &clothmesh : mClothMeshes) {
            OCL_CALL(mCalcDistToLine->setArg(0, clothmesh->mVertexBufferCL));
            OCL_CALL(mCalcDistToLine->setArg(1, clothmesh->mDistToLineBufferCL));
            OCL_CALL(mCalcDistToLine->setArg(2, rayOriginCL));
            OCL_CALL(mCalcDistToLine->setArg(3, rayWorldCL));
            ENQUEUE_VERTICES(mCalcDistToLine, clothmesh);

            distances.reserve(clothmesh->numVertices());
            mQueue.enqueueReadBuffer(clothmesh->mDistToLineBufferCL, true, 0,
                                     sizeof(cl_float) * clothmesh->numVertices(), distances.data());

            for (uint i = 0; i < clothmesh->numVertices(); ++i) {
                std::cout << distances[i] << ", ";
                if (distances[i] < closestdistance) {

                    closestcloth = clothmesh;
                    closestdistance = distances[i];
                    closestindex = i;
                }
                std::cout << std::endl;
            }
        }

        if (closestcloth) {
            mIsGrabbingCloth = true;
            mGrabbedClothMesh = closestcloth;
            mGrabbedVertexIndex = static_cast<uint>(closestindex);
            std::cout << "Grabbed vertex index = " << mGrabbedVertexIndex << " at distance = " << closestdistance << std::endl;

            if (modifiers == GLFW_MOD_SHIFT) {
                // pin this vertex
                cl_float invmass_ = 0.0f;
                OCL_CALL(mQueue.enqueueWriteBuffer(mGrabbedClothMesh->mVertexClothBufferCL, true,
                                                   sizeof(ClothVertexData) * mGrabbedVertexIndex + offsetof(ClothVertexData, invmass),
                                                   sizeof(cl_float), &invmass_));
            }

            return true;
        }

        return false;
    }

    bool ClothSimulationScene::mouseMotionEvent(const glm::ivec2 &p, const glm::ivec2 &rel, int button, int modifiers) {
        mCursorPosition = p;

        if (mIsRotatingCamera) {
            glm::vec3 eulerAngles = mCameraRotator->getEulerAngles();
            eulerAngles.x += 0.02f * rel.y;
            eulerAngles.y += 0.02f * rel.x;
            eulerAngles.x = util::clamp(eulerAngles.x, - CL_M_PI_F / 2, CL_M_PI_F / 2);
            mCameraRotator->setEulerAngles(eulerAngles);

            return true;
        }

        return false;
    }

    bool ClothSimulationScene::scrollEvent(const glm::ivec2 &p, const glm::vec2 &rel) {
        const float y = rel.y;
        mCamera->translate(glm::vec3(0.0f, 0.0f, y));
    }

    bool ClothSimulationScene::resizeEvent(const glm::ivec2 &p) {
        mCamera->setScreenDimensions(glm::uvec2(static_cast<unsigned int>(p.x),
                                                static_cast<unsigned int>(p.y)));
        return false;
    }

    bool ClothSimulationScene::keyboardEvent(int key, int scancode, int action, int modifiers) {
        return BaseScene::keyboardEvent(key, scancode, action, modifiers);
    }

    void ClothSimulationScene::loadShaders() {
        for (auto &shaderpair : mShaders) {
            auto shader = shaderpair.second;
            shader->compile();
        }

        mAxisShader->compile();
    }

    void ClothSimulationScene::loadKernels() {
        OCL_ERROR;

        mPredictPositionsProgram = util::LoadCLProgram("predict_positions.cl", mContext, mDevice);
        OCL_CHECK(mCalcDistToLine = util::make_unique<cl::Kernel>(*mPredictPositionsProgram,
                                                                  "calc_dist_to_line",
                                                                  CL_ERROR));
        OCL_CHECK(mApplyGrabImpulse = util::make_unique<cl::Kernel>(*mPredictPositionsProgram,
                                                                    "apply_grab_impulse",
                                                                    CL_ERROR));
        OCL_CHECK(mPredictPositions = util::make_unique<cl::Kernel>(*mPredictPositionsProgram,
                                                                    "predict_positions",
                                                                    CL_ERROR));
        OCL_CHECK(mSetPositionsToPredicted = util::make_unique<cl::Kernel>(*mPredictPositionsProgram,
                                                                           "set_positions_to_predicted", CL_ERROR));

        mClothSimulationProgram = util::LoadCLProgram("cloth_simulation.cl", mContext, mDevice);
        OCL_CHECK(mCalcClothMass = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                 "calc_cloth_mass",
                                                                 CL_ERROR));
        OCL_CHECK(mCalcInverseMass = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                   "calc_inverse_mass",
                                                                   CL_ERROR));
        OCL_CHECK(mCalcEdgeProperties = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                      "calc_edge_properties",
                                                                      CL_ERROR));
        OCL_CHECK(mFixVertex = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                             "fix_vertex",
                                                             CL_ERROR));
        OCL_CHECK(mClipToPlanes = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                "clip_to_planes",
                                                                CL_ERROR));
        OCL_CHECK(mCalcPositionCorrections = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                           "calc_position_corrections",
                                                                           CL_ERROR));
        OCL_CHECK(mCorrectPredictions = util::make_unique<cl::Kernel>(*mClothSimulationProgram,
                                                                      "correct_predictions",
                                                                      CL_ERROR));

    }

    void ClothSimulationScene::loadSetup() {
        std::string contents = "";
        if (!bwgl::TryReadFromFile(mCurrentSetupFile, contents)) {
            displayError("Failed to load setup");
        }

        displayError();

        mCurrentSetup = SceneSetup::LoadFromJsonString(contents);
        mCurrentSetup.filepath = mCurrentSetupFile;

        mCamera->setFieldOfViewY(mCurrentSetup.camera.fovY);

        for (ShaderConfig config : mCurrentSetup.shaders) {
            auto shader = std::make_shared<clgl::BaseShader>(
                    std::unordered_map<GLuint, std::string>{{GL_VERTEX_SHADER,   SHADERPATH(config.vertex)},
                                                            {GL_FRAGMENT_SHADER, SHADERPATH(config.fragment)}});
            shader->compile();
            mShaders[config.name] = shader;
        }

        for (const MeshConfig &meshconfig : mCurrentSetup.meshes) {
            std::shared_ptr<pbd::Mesh> mesh = nullptr;
            std::shared_ptr<ClothMesh> cloth = nullptr;

            if (meshconfig.isCloth) {
                cloth = MeshLoader::LoadClothMesh(RESOURCEPATH(meshconfig.path));
                mClothMeshes.push_back(cloth);

                // pre-process each vertex with its transform matrix
                const glm::mat4 rotation = glm::toMat4(glm::quat(meshconfig.orientation));
                const glm::mat4 translation = glm::translate(glm::mat4(1.0f), meshconfig.position);
                const glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(meshconfig.scale));

                const glm::mat4 transform = translation * rotation * scale;
                const glm::mat3 normalTransform(transform);

                for (auto &vertex : cloth->mVertices) {
                    glm::vec4 position = glm::vec4(vertex.position, 1.0f);
                    position = transform * position;
                    vertex.position = glm::vec3(position);
                    vertex.normal = normalTransform * vertex.normal;

                    if (meshconfig.flipNormals) {
                        vertex.normal = -vertex.normal;
                    }
                }

                mesh = cloth;
            } else {
                mesh = MeshLoader::LoadMesh(RESOURCEPATH(meshconfig.path));
            }

            mesh->uploadHostData();
            mesh->generateBuffersCL(mContext);
            mesh->clearHostData();

            // Add these OpenGL memory objects to a vector for easy acquire/release
            auto memObjects = mesh->getMemoryCL();
            mMemObjects.insert(mMemObjects.end(), memObjects.begin(), memObjects.end());
            memObjects.clear();

            /// Calculate initial values for this cloth mesh
            if (cloth) {
                auto memory = cloth->getMemoryCL();
                OCL_CALL(mQueue.enqueueAcquireGLObjects(&memory));

                /// kernels/cloth_simulation.cl -> calc_cloth_mass
                OCL_CALL(mCalcClothMass->setArg(0, cloth->mVertexBufferCL));
                OCL_CALL(mCalcClothMass->setArg(1, cloth->mVertexClothBufferCL));
                OCL_CALL(mCalcClothMass->setArg(2, cloth->mTriangleBufferCL));
                OCL_CALL(mCalcClothMass->setArg(3, cloth->mTriangleClothBufferCL));
                ENQUEUE_TRIANGLES(mCalcClothMass, cloth);

                /// kernels/cloth_simulation.cl -> calc_inverse_mass
                OCL_CALL(mCalcInverseMass->setArg(0, cloth->mVertexClothBufferCL));
                ENQUEUE_VERTICES(mCalcInverseMass, cloth);

                //OCL_CALL(mFixVertex->setArg(0, cloth->mVertexClothBufferCL));
                //OCL_CALL(mFixVertex->setArg(1, 0));
                //ENQUEUE_VERTICES(mFixVertex, cloth);

                /// kernels/cloth_simulation.cl -> calc_edge_properties
                OCL_CALL(mCalcEdgeProperties->setArg(0, cloth->mVertexBufferCL));
                OCL_CALL(mCalcEdgeProperties->setArg(1, cloth->mTriangleBufferCL));
                OCL_CALL(mCalcEdgeProperties->setArg(2, cloth->mEdgeBufferCL));
                OCL_CALL(mCalcEdgeProperties->setArg(3, cloth->mEdgeClothBufferCL));
                OCL_CALL(mCalcEdgeProperties->setArg(4, cloth->mTriangleClothBufferCL));
                ENQUEUE_EDGES(mCalcEdgeProperties, cloth);

                OCL_CALL(mQueue.enqueueReleaseGLObjects(&memory));
            }

            auto shader = mShaders[meshconfig.shader];

            auto meshobject = std::make_shared<clgl::MeshObject>(mesh, shader);
            if (!cloth) {
                meshobject->setScale(meshconfig.scale);
                meshobject->setEulerAngles(meshconfig.orientation);
                meshobject->setPosition(meshconfig.position);
            }

            mRenderObjects.push_back(meshobject);
        }

        for (const PointLightConfig &config : mCurrentSetup.pointLights) {
            auto pointLight = std::make_shared<clgl::PointLight>();
            pointLight->mAmbientColor = config.color.ambient;
            pointLight->mDiffuseColor = config.color.diffuse;
            pointLight->mSpecularColor = config.color.specular;
            pointLight->mAttenuation = clgl::Attenuation(config.attenuation.linear, config.attenuation.quadratic);

            pointLight->setPosition(config.position);
            mLights.push_back(pointLight);
        }

        for (const DirectionalLightConfig &config : mCurrentSetup.directionalLights) {
            auto dirLight = std::make_shared<clgl::DirectionalLight>(
                    config.color.ambient,
                    config.color.diffuse,
                    config.color.specular,
                    config.direction
            );
            mLights.push_back(dirLight);
        }
    }

    void ClothSimulationScene::createCamera() {
        mCameraRotator = std::make_shared<clgl::SceneObject>();
        mCameraRotator->translate(glm::vec3(0.0f, -1.0f, 0.0f));
        mCamera = std::make_shared<clgl::Camera>(glm::uvec2(100, 100), 75);
        mCamera->setPosition(glm::vec3(0.0f, 0.0f, 10.0f));
        clgl::SceneObject::attach(mCameraRotator, mCamera);
    }

    void ClothSimulationScene::createAxis() {
        mAxisShader = std::make_shared<clgl::BaseShader>(
                std::unordered_map<GLuint, std::string>{{GL_VERTEX_SHADER,   SHADERPATH("axis.vert")},
                                                        {GL_FRAGMENT_SHADER, SHADERPATH("axis.frag")}});
        mAxisShader->compile();

        // create axis geometry
        const glm::vec3 positions[6] = {
                glm::vec3(0.0f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)
        };

        const glm::vec3 colors[6] = {
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)
        };

        mAxisPositions = std::make_shared<bwgl::VertexBuffer>(GL_ARRAY_BUFFER);
        mAxisPositions->bind();
        mAxisPositions->bufferData(6 * sizeof(glm::vec3), &positions[0]);
        mAxisPositions->unbind();

        mAxisColors = std::make_shared<bwgl::VertexBuffer>(GL_ARRAY_BUFFER);
        mAxisColors->bind();
        mAxisColors->bufferData(6 * sizeof(glm::vec3), &colors[0]);
        mAxisColors->unbind();

        mAxis = std::make_shared<bwgl::VertexArray>();
        mAxis->bind();
        mAxis->addVertexAttribute(*mAxisPositions, 3, GL_FLOAT, GL_FALSE, 0);
        mAxis->addVertexAttribute(*mAxisColors, 3, GL_FLOAT, GL_FALSE, 0);
        mAxis->unbind();
    }

    void ClothSimulationScene::loadMarker() {
        auto shader = std::make_shared<clgl::BaseShader>(
                std::unordered_map<GLuint, std::string>{{GL_VERTEX_SHADER,   SHADERPATH("marker.vert")},
                                                        {GL_FRAGMENT_SHADER, SHADERPATH("marker.frag")}});
        shader->compile();

        mShaders["marker"] = shader;

        auto mesh = MeshLoader::LoadMesh(RESOURCEPATH("models/marker/marker.obj"));
        mesh->flipNormals();
        mesh->uploadHostData();

        mMarker = std::make_shared<clgl::MeshObject>(mesh, shader);
        mMarker->setScale(0.1f);
    }

    glm::vec3 ClothSimulationScene::getCameraWorldPosition() {
        return glm::vec3(mCamera->getParent()->getTransform() * glm::vec4(mCamera->getPosition(), 1.0f));
    }


    glm::vec3 ClothSimulationScene::getCursorWorldRay() {
        /// Step 1: 3D normalized device coordinates
        const float x = (2.0f * mCursorPosition.x) / mCamera->getScreenDimensions().x - 1.0f;
        const float y = 1.0f - (2.0f * mCursorPosition.y) / mCamera->getScreenDimensions().y;
        const float z = 1.0f;

        /// Step 2: 4D homogeneous clip coordinates
        const glm::vec4 rayClip(x, y, -1.0f, 1.0f);

        /// Step 3: 4D camera coordinates
        glm::vec4 rayCamera = glm::inverse(mCamera->getPerspectiveTransform()) * rayClip;
        rayCamera.z = -1.0f;
        rayCamera.w = 0.0f;

        /// Step 4: 4D world coordinates
        glm::vec3 rayWorld(mCamera->getTransform() * rayCamera);
        rayWorld = glm::normalize(rayWorld);

        return rayWorld;
    }

    void ClothSimulationScene::updateTimeLabelsInGUI(double timeSinceLastUpdate) {
        double cumSimulationTimes = 0.0;
        for (double simTime : mSimulationTimes) {
            cumSimulationTimes += simTime;
        }
        double avgSimulationTime = cumSimulationTimes / mSimulationTimes.size();
        std::stringstream ss;
        ss << "MS/frame: " << std::setprecision(3) << 1000 * avgSimulationTime;
        mLabelAverageFrameTime->setCaption(ss.str());

        ss.str("");

        double FPS = mFramesSinceLastUpdate / timeSinceLastUpdate;
        ss << "Average FPS: " << std::setprecision(3) << FPS;
        mLabelFPS->setCaption(ss.str());
    }

    void ClothSimulationScene::displayError(const std::string &str) {
        mErrorLabel->setCaption(str);
    }

    const uint ClothSimulationScene::NUM_AVG_SIM_TIMES = 10;
}
