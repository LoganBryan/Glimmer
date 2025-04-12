#include "OpenXRManager_OpenGL.h"

bool OpenXRManager_OpenGL::Init(XRApplicationInfo applicationInfo)
{
    sharedContext = wglCreateContext(mHDC);
    if (!wglShareLists(mGLRC, sharedContext))
    {
        printf("Failed to share lists!\n");
        return false;
    }

    mInstance = new OpenXRInstance_OpenGL();
    mInstance->CreateInstance(applicationInfo.applicationName, applicationInfo.applicationVersion, applicationInfo.engineName, applicationInfo.engineVersion);
    mInstance->CreateDebugMessenger();
    mInstance->GetInstanceProperties();
    mInstance->CreateSystemID();
    mInstance->CreateViewConfigurationViews();
    mInstance->GetEnvironmentBlendModes();

    mSession = new XRSession_OpenGL(mInstance, mHDC, mGLRC, mWindow);
    mSession->CreateActionSet("glimmer-primary-actionset", "Glimmer Primary ActionSet", 0);
    mSession->SuggestBindings();
    mSession->CreateSession();
    mSession->CreateActionPoses();
    mSession->AttachActionSet();

    if (mInstance->handTrackingSystemProperties.supportsHandTracking)
    {
        mSession->CreateHandTrackers();
    }

    mSession->CreateReferenceSpace();

    mSwapchainManager = new SwapchainManager<XrSwapchainImageOpenGLKHR, OpenGLSwapchainTraits>(mSession->GetSession(), mInstance->GetViewConfigurationViews());
    mSwapchainManager->CreateSwapchains();

    // TODO: in session check that swapchain and renderer are set before attempting to use them!
    mSession->SetSwapchainManager(mSwapchainManager); 

    mRenderer = new Renderer(mWindow);
    mRenderer->Init();
    mSession->SetRenderer(mRenderer);

    return true;
}

void OpenXRManager_OpenGL::Run()
{
    while (!glfwWindowShouldClose(mWindow) && mSession->IsApplicationRunning())
    {
        mSession->PollSystemEvents();
        mSession->PollEvents();

        mSession->RenderDesktopWindow();

        if (mSession->IsSessionRunning())
        {
            std::lock_guard<std::mutex> lock(mGLMutex);
            mSession->RenderFrame();
        }

        glfwPollEvents();
    }
}

void OpenXRManager_OpenGL::Destroy()
{
    if (calledDestroy)
    {
        printf("Attempted to destroy XR application multiple times!\n");
        return;
    }
    calledDestroy = true;

    wglDeleteContext(sharedContext);

    if (mSwapchainManager)
        mSwapchainManager->DestroySwapchains();
    if (mSession)
    {
        mSession->DestroyReferenceSpace();
        mSession->DestroySession();
    }
    if (mInstance)
    {
        mInstance->DestroyDebugMessenger();
        mInstance->DestroyInstance();
    }
}
