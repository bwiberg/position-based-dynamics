#pragma once

#include "BaseScene.hpp"

#include <bwgl/bwgl.hpp>

#include "rendering/BaseShader.hpp"
#include "rendering/Camera.hpp"
#include "rendering/light/DirectionalLight.hpp"
#include "rendering/light/AmbientLight.hpp"
#include "rendering/light/PointLight.hpp"

#include <deque>
#include <rendering/RenderObject.hpp>

namespace pbd {
    /// @brief //todo add brief description to FluidScene
    /// @author Benjamin Wiberg
    class ClothSimulationScene : public clgl::BaseScene {
    public:
        ClothSimulationScene(cl::Context &context, cl::Device &device, cl::CommandQueue &queue);

        virtual void addGUI(nanogui::Screen *screen) override;

        virtual void reset() override;

        virtual void update() override;

        virtual void render() override;

        virtual bool mouseButtonEvent(const glm::ivec2 &p, int button, bool down, int modifiers) override;

        virtual bool mouseMotionEvent(const glm::ivec2 &p, const glm::ivec2 &rel, int button, int modifiers) override;

        virtual bool resizeEvent(const glm::ivec2 &p) override;

        virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    private:
        void createCamera();

        void createLights();

        void loadShaders();

        void loadKernels();

        void loadSetup();

        std::string mCurrentSetup;

        std::shared_ptr<clgl::Camera> mCamera;

        std::shared_ptr<clgl::SceneObject> mCameraRotator;

        std::shared_ptr<clgl::DirectionalLight> mDirLight;
        std::shared_ptr<clgl::AmbientLight> mAmbLight;
        std::shared_ptr<clgl::PointLight> mPointLight;

        std::vector<std::shared_ptr<clgl::Light>> mLights;
        std::vector<std::shared_ptr<clgl::BaseShader>> mShaders;
        std::vector<std::shared_ptr<clgl::RenderObject>> mRenderObjects;

        std::shared_ptr<clgl::BaseShader> mBaseShader;
        std::shared_ptr<clgl::BaseShader> mCheckerboardShader;

        bool mIsRotatingCamera;

        std::vector<cl::Memory> mMemObjects;

        /// FPS

        void updateTimeLabelsInGUI(double timeSinceLastUpdate);

        std::deque<double> mSimulationTimes;

        static const uint NUM_AVG_SIM_TIMES;
        double mTimeOfLastUpdate;
        uint mFramesSinceLastUpdate;

        nanogui::Label *mLabelFPS;
        nanogui::Label *mLabelAverageFrameTime;
    };
}