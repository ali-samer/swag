#include <aiecad/core/Application.hpp>
#include <aiecad/core/events/EventBus.hpp>
#include <aiecad/core/Window.hpp>
#include <aiecad/core/logging/Logger.hpp>

/* Dummy Editors for testing */
#include <aiecad/core/editor/EditorLayer.hpp>
#include <aiecad/core/editor/DemoLayer.hpp>
#include <aiecad/core/editor/EmptyLayer.hpp>

/* UI stuff */
#include <aiecad/core/editor/UIStyle.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

/* We're tightly coupling the `Application` API with the windowing and graphics backends temporarily */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace aiecad {
Application *Application::s_instance{ nullptr };

Application& Application::Get() {
	if (not s_instance) {
		AIECAD_CORE_FATAL("Application::Get() called before instance exists");
		throw std::runtime_error("No Application instance");
	}
	return *s_instance;
}

Application::Application(const ApplicationSpecification &app_spec)
	: m_appSpec(app_spec) {
	if (s_instance) {
		AIECAD_CORE_FATAL("Attempted to create another `Application` instance while one already exists");
		throw std::runtime_error("Application instance already exists");
	}

	Logger::Init({
		.level = LoggerLevel::Debug,
		.toFile = { /* TODO: Determine where to log behavior to. */},
		.toStdout = true, .toStderr = false,
	});
	s_instance = this;
	AIECAD_CORE_INFO("Initializing application");
	init();
	AIECAD_CORE_INFO("Application initialized");
}

Application::~Application() {
	shutdown();
	s_instance = nullptr;
}

void Application::init() {
	m_eventBus = std::make_unique<EventBus>();

	WindowSpecification windowSpec{m_appSpec.window_spec};
	m_window = Window::Create(windowSpec);
    AIECAD_CORE_INFO("Window created");

	m_window->setEventCallback(
		[this](const BackendWindowEvent& e) {
			if (e.type == BackendWindowEventType::Close) {
				AIECAD_CORE_INFO("[BackendWindowEventType] Closing application");
				m_eventBus->publish(WindowCloseEvent{});
			}
		}
	);

	m_windowCloseSub = m_eventBus->subscribe<WindowCloseEvent>(
		[this](const WindowCloseEvent&) {
			AIECAD_CORE_INFO("[WindowCloseEvent] Closing application");
			requestClose();
		}
	);

	setupUIFramework();
	pushLayers();
}

void Application::setupUIFramework() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	// TODO: This is a temporary styles configuration. Replace with a proper mechanism for configuring styles.
	UIStyle::ApplyDarkTheme();
	// UIStyle::ApplyDraculaTheme();
	auto* window = static_cast<GLFWwindow*>(m_window->getNativeHandle());
	ImGui_ImplGlfw_InitForOpenGL(window, true);

	// TODO: This selects a core OpenGL 4.10 profile according to my system. Change it for portability
	ImGui_ImplOpenGL3_Init("#version 410");
}

void Application::pushLayers() {
	auto demoLayer = std::make_unique<DemoLayer>();
	pushLayer(std::move(demoLayer));
}

void Application::shutdown() {
	m_windowCloseSub.reset();

	// ***
	// TODO: awful design... make sure to take those out.
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	// ***

	m_window.reset();
	m_eventBus.reset();
}

int Application::run() {
	while (m_running) {
		if (!m_window) {
			break;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		for (auto it = m_layerStack.begin(); it != m_layerStack.end(); ++it) {
			if (*it) {
				(*it)->onImGuiRender();
			}
		}

		ImGui::Render();

		glViewport(0, 0,
				   static_cast<int>(m_window->getWidth()),
				   static_cast<int>(m_window->getHeight()));
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup);
		}

		m_window->onUpdate();
	}

	return 0;
}

void Application::pushLayer(LayerPtr layer) {
	m_layerStack.pushLayer(std::move(layer));
}

void Application::pushOverlay(LayerPtr overlay) {
	m_layerStack.pushOverlay(std::move(overlay));
}

} // namespace aiecad