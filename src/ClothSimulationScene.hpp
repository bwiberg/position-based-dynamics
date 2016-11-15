#pragma once

#include "BaseScene.hpp"
#include "SceneSetup.hpp"

#include <bwgl/bwgl.hpp>

#include "rendering/BaseShader.hpp"
#include "rendering/Camera.hpp"
#include "rendering/light/DirectionalLight.hpp"
#include "rendering/light/PointLight.hpp"

#include <deque>
#include <rendering/RenderObject.hpp>
#include <geometry/Mesh.hpp>

#include <simulation/Grid.hpp>

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

        virtual bool scrollEvent(const glm::ivec2 &p, const glm::vec2 &rel) override;

    private:
        void createCamera();

        void loadShaders();

        void loadKernels();

        void loadSetup();

        void displayError(const std::string &str = "");

        void renderAxes();

        std::shared_ptr<clgl::BaseShader> mAxisShader;
        std::shared_ptr<bwgl::VertexBuffer> mAxisPositions;
        std::shared_ptr<bwgl::VertexBuffer> mAxisColors;
        std::shared_ptr<bwgl::VertexArray> mAxis;

        std::string mCurrentSetupFile;
        SceneSetup mCurrentSetup;

        std::shared_ptr<clgl::Camera> mCamera;

        std::shared_ptr<clgl::SceneObject> mCameraRotator;

        std::vector<std::shared_ptr<clgl::Light>> mLights;
        std::vector<std::shared_ptr<clgl::RenderObject>> mRenderObjects;

        std::vector<std::shared_ptr<pbd::ClothMesh>> mClothMeshes;

        std::map<std::string, std::shared_ptr<clgl::BaseShader>> mShaders;

        bool mIsRotatingCamera;

        std::vector<cl::Memory> mMemObjects;

        uint mFrameCounter;

        std::unique_ptr<cl::Program> mPredictPositionsProgram;
        std::unique_ptr<cl::Kernel> mPredictPositions;
        std::unique_ptr<cl::Kernel> mSetPositionsToPredicted;

        std::unique_ptr<cl::Program> mClothSimulationProgram;

        /// Cloth Mesh setup kernels ///
        std::unique_ptr<cl::Kernel> mCalcClothMass;
        std::unique_ptr<cl::Kernel> mCalcInverseMass;
        std::unique_ptr<cl::Kernel> mCalcEdgeProperties;

        /// Position correction kernels ///

        std::unique_ptr<cl::Kernel> mClipToPlanes;

        std::unique_ptr<pbd::Grid> mGridCL;
        std::unique_ptr<cl::Buffer> mBinCountCL; // CxCxC-sized uint buffer, containing particle count per cell
        std::unique_ptr<cl::Buffer> mBinStartIDCL;

        /// FPS

        void updateTimeLabelsInGUI(double timeSinceLastUpdate);

        std::deque<double> mSimulationTimes;

        static const uint NUM_AVG_SIM_TIMES;
        double mTimeOfLastUpdate;
        uint mFramesSinceLastUpdate;

        nanogui::Label *mLabelFPS;
        nanogui::Label *mLabelFrameNumber;
        nanogui::Label *mLabelAverageFrameTime;
        nanogui::Label *mErrorLabel;
    };
}