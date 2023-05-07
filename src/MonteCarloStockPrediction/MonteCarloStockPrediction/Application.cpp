#include "Application.h"
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static inline bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}


Application::Application() :screen{ 
    [this](boost::gregorian::date date, float value) {
        this->data.insert(std::make_pair(date, value));
        //std::cout << boost::gregorian::to_simple_string(this->data.begin()->first) << std::endl;;
    },
    cl::sycl::device::get_devices(),
    [this](std::string deviceName, int workload) {
        this->deviceNameToWorkload.insert(std::make_pair(deviceName, workload));
    },
    [this]()->AlgorithmParameter& {
        return this->parameter;
    },
    [this]() {
        std::vector<float> StocksData;
        int starting_date_offset = max(0, this->data.size() - this->parameter.m_NumberOfDaysToUse);
        auto dataIterator = this->data.begin();
        std::advance(dataIterator, starting_date_offset);
        while (dataIterator != this->data.end()) {
            StocksData.push_back(dataIterator->second);
            dataIterator++;
        }
        this->HMC_Wiggins = new WigginsAlgorithm(this->parameter, std::move(StocksData), this->deviceNameToWorkload);
    }
} {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()){
        std::cout << "glfwInit failed. Terminating program" << std::endl;
        std::terminate();
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    const unsigned int SCREEN_WIDTH = 1920;
    const unsigned int SCREEN_HEIGHT = 1024;


    this->window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Team Dekus Monte Carlo Engine", nullptr, nullptr);
    if (this->window == nullptr)
    {
        std::cout << "window initialisation faled. Terminating program" << std::endl;
        std::terminate();
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);
    std::filesystem::path goodFontFileLocationObject = 
        std::filesystem::current_path() / "assets" / "Roboto-Medium.ttf";
    std::string fileLocation = goodFontFileLocationObject.string();
    if(file_exists(fileLocation)) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fileLocation.c_str(), 20.0f);
    }
    

    {
        this->parameter.m_MCMCIteration = 3000;
        this->parameter.m_GraphUpdateIteration = 100;
        this->parameter.m_NumberOfDaysToUse = 365;
        this->parameter.m_BurnIn = 1000;
        this->parameter.m_DiscretCountOfContinuiosSpace = 100;
        this->parameter.m_leapfrog = 12;
        this->parameter.m_epsilon = 0.19;

        this->parameter.m_volatility_theta.lower = 0.0;
        this->parameter.m_volatility_theta.upper = 1.0;
        this->parameter.m_volatility_theta.testval = 0.5;
        this->parameter.m_volatility_theta.guassian_step_sd = 1.0f / (4.0 * 6.0);

        this->parameter.m_volatility_mu.mean = -5.0f;
        this->parameter.m_volatility_mu.sd = 0.1f;
        this->parameter.m_volatility_mu.testval = -5.0f;
        this->parameter.m_volatility_mu.guassian_step_sd = 0.1f / 4.0f;
        this->parameter.m_volatility_mu.buffer_range_sigma_multiplier = 4;

        this->parameter.m_volatility_sigma.lower = 0.001f;
        this->parameter.m_volatility_sigma.upper = 0.2f;
        this->parameter.m_volatility_sigma.testval = 0.05f;
        this->parameter.m_volatility_sigma.guassian_step_sd = (0.02 - 0.001) / (4.0 * 6.0);

        this->parameter.m_volatility.dt = 1.0f;
        this->parameter.m_volatility.testval = 1.0f;
        this->parameter.m_volatility.dw_lower = 0.0f;
        this->parameter.m_volatility.dw_upper = 0.1f;
    }
}

void Application::main_loop() {
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        this->screen.Render();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
