#include "ClothSimulationScene.hpp"
#include "SceneSetup.hpp"

#include <iomanip>

#include <util/OCL_CALL.hpp>
#include <util/math_util.hpp>
#include <util/paths.hpp>

#include <geometry/MeshLoader.hpp>
#include <rendering/MeshObject.hpp>

namespace pbd {
    ClothSimulationScene::ClothSimulationScene(cl::Context &context, cl::Device &device, cl::CommandQueue &queue)
            : BaseScene(context, device, queue) {
        mCurrentSetupFile = RESOURCEPATH("setups/cloth_sheet.json");

        loadShaders();

        createCamera();
        createLights();

        loadKernels();
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
        mLabelAverageFrameTime = new Label(win, "");
        mLabelFPS = new Label(win, "");
        updateTimeLabelsInGUI(0.0);
    }

    void ClothSimulationScene::reset() {
        while (!mSimulationTimes.empty()) mSimulationTimes.pop_back();
        updateTimeLabelsInGUI(0.0);

        mRenderObjects.clear();
        mClothMeshes.clear();
        //mLights.clear();
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

        OCL_CALL(mQueue.enqueueReleaseGLObjects(&mMemObjects, NULL, &event));
        OCL_CALL(event.wait());

        double timeEnd = glfwGetTime();
        while (mSimulationTimes.size() > NUM_AVG_SIM_TIMES) {
            mSimulationTimes.pop_back();
        }
        mSimulationTimes.push_front(timeEnd - timeBegin);

        // update GUI twice each second
        if (timeEnd - mTimeOfLastUpdate > 2.0f) {
            updateTimeLabelsInGUI(timeEnd - mTimeOfLastUpdate);
            mFramesSinceLastUpdate = 0;
        }
    }

    void ClothSimulationScene::render() {
        const glm::mat4 VP = mCamera->getPerspectiveTransform() * glm::inverse(mCamera->getTransform());

        for (auto shader : mShaders) {
            shader.second->use();
            for (auto light : mLights) {
                light->setUniformsInShader(shader.second, light->getType());
            }
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        for (auto renderObject : mRenderObjects) {
            renderObject->render(VP);
        }
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
    }

    bool ClothSimulationScene::mouseButtonEvent(const glm::ivec2 &p, int button, bool down, int modifiers) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            mIsRotatingCamera = down;
            return true;
        }

        return false;
    }

    bool ClothSimulationScene::mouseMotionEvent(const glm::ivec2 &p, const glm::ivec2 &rel, int button, int modifiers) {
        if (mIsRotatingCamera) {
            glm::vec3 eulerAngles = mCameraRotator->getEulerAngles();
            eulerAngles.x += 0.05f * rel.y;
            eulerAngles.y += 0.05f * rel.x;
            eulerAngles.x = util::clamp(eulerAngles.x, - CL_M_PI_F / 2, CL_M_PI_F / 2);
            mCameraRotator->setEulerAngles(eulerAngles);

            return true;
        }

        return false;
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
        for (SceneSetup::ShaderConfig config : mCurrentSetup.shaders) {
            auto shader = std::make_shared<clgl::BaseShader>(
                    std::unordered_map<GLuint, std::string>{{GL_VERTEX_SHADER,   SHADERPATH(config.vertex)},
                                                            {GL_FRAGMENT_SHADER, SHADERPATH(config.fragment)}});
            shader->compile();
            mShaders[config.name] = shader;
        }
    }

    void ClothSimulationScene::loadKernels() {

    }

    void ClothSimulationScene::loadSetup() {
        std::string contents = "";
        if (!bwgl::TryReadFromFile(mCurrentSetupFile, contents)) {
            displayError("Failed to load setup");
        }

        displayError();

        mCurrentSetup = SceneSetup::LoadFromJsonString(contents);
        mCurrentSetup.filepath = mCurrentSetupFile;

        mCameraRotator->setEulerAngles(glm::vec3(0.0f, 0.0f, 0.0f));
        mCamera->setFieldOfViewY(mCurrentSetup.camera.fovY);
        mCamera->setPosition(mCurrentSetup.camera.position);

        loadShaders();

        for (SceneSetup::MeshConfig meshconfig : mCurrentSetup.meshes) {
            std::shared_ptr<pbd::Mesh> mesh;

            if (meshconfig.isCloth) {
                auto cloth = MeshLoader::LoadClothMesh(RESOURCEPATH(meshconfig.path));
                mClothMeshes.push_back(cloth);
                mesh = cloth;
            } else {
                mesh = MeshLoader::LoadMesh(RESOURCEPATH(meshconfig.path));
            }

            mesh->uploadHostData();
            mesh->generateBuffersCL(mContext);
            mesh->clearHostData();

            auto shader = mShaders[meshconfig.shader];

            auto meshobject = std::make_shared<clgl::MeshObject>(mesh, shader);
            meshobject->setScale(meshconfig.scale);
            meshobject->setEulerAngles(meshconfig.orientation);
            meshobject->setPosition(meshconfig.position);

            mRenderObjects.push_back(meshobject);
        }
    }

    void ClothSimulationScene::createCamera() {
        mCameraRotator = std::make_shared<clgl::SceneObject>();
        mCameraRotator->translate(glm::vec3(0.0f, -1.0f, 0.0f));
        mCamera = std::make_shared<clgl::Camera>(glm::uvec2(100, 100), 75);
        mCamera->setPosition(glm::vec3(0.0f, 0.0f, 10.0f));
        clgl::SceneObject::attach(mCameraRotator, mCamera);
    }

    void ClothSimulationScene::createLights() {
        mAmbLight = std::make_shared<clgl::AmbientLight>(glm::vec3(1.0f, 0.4f, 0.1f), 0.2f);
        mDirLight = std::make_shared<clgl::DirectionalLight>(glm::vec3(1.0f, 1.0f, 1.0f), 0.1f);
        //mPointLight = std::make_shared<clgl::PointLight>(glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
        //mPointLight->setAttenuation(clgl::Attenuation(0.1f, 0.1f));
        //mPointLight->translate(glm::vec3(0.0f, 2.0f, 0.0f));

        mLights.push_back(mAmbLight);
        mLights.push_back(mDirLight);
        //mLights.push_back(mPointLight);
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
