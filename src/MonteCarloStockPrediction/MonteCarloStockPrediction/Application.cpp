#include "Application.h"
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
static inline bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

const unsigned int SCREEN_WIDTH = 1920;
const unsigned int SCREEN_HEIGHT = 1024;
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
    }, 
    [this]() {
        this->HMC_Wiggins->iterate();
    },
    [this]() {
        return this->HMC_Wiggins->percent_of_completion();
    },
    [this]() {
        return this->HMC_Wiggins->is_completed();
    },
    [this]() {
        return this->HMC_Wiggins->get_response();
    },    
        SCREEN_WIDTH, SCREEN_HEIGHT
} {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()){
        std::cout << "glfwInit failed. Terminating program" << std::endl;
        throw std::runtime_error("glfwInit failed. Terminating");
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only




    this->window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Team Dekus Monte Carlo Engine", nullptr, nullptr);
    if (this->window == nullptr)
    {
        std::cout << "window initialisation faled. Terminating program" << std::endl;
        throw std::runtime_error("window initialisation faled");
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

    std::filesystem::path goodFontFileLocationObject = 
        std::filesystem::current_path() / "assets" / "Roboto-Medium.ttf";
    std::string fileLocation = goodFontFileLocationObject.string();
    if(file_exists(fileLocation)) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fileLocation.c_str(), 20.0f);
    }
    
    std::filesystem::path intelImageFileLocationObject =
        std::filesystem::current_path() / "assets" / "oneAPI.jpg";
    std::string imageFileLoc = intelImageFileLocationObject.string();
    if (file_exists(imageFileLoc)) {
        stbi_set_flip_vertically_on_load(1);
        int m_Width = SCREEN_WIDTH, m_Height=SCREEN_HEIGHT, m_BPP = 0;

        this->screen.intelBackgroundImageBuffer = 
            stbi_load(imageFileLoc.c_str(), &m_Width, &m_Height, &m_BPP, 4);

    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
        glClearColor(5.0f / 255, 1.0f / 255, 74.0f / 255, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        this->screen.Render();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();

    }
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}
