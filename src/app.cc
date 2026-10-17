/*
 * app.h 구현(시작 함수는 startup.cc). Win32 창 의존은 이 파일에만 둔다.
 * - 구현할 것: 카메라 이동(WASD + 마우스), 진단 오버레이, 벤치마크(warm-up 120 + 600 프레임), 시작 보고서.
 */
#include "app.h"
#include "runtime.h"
#include "scene.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <windows.h>

namespace RD {

PilotConfig PilotConfig::fromArgs(int argc, char **argv) {
    PilotConfig config;
#ifdef RD_DEFAULT_ASSET
    config.assetPath = RD_DEFAULT_ASSET;
#endif
#ifdef RD_DEFAULT_PLUGIN_DIR
    config.pluginDir = RD_DEFAULT_PLUGIN_DIR;
#endif
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if(arg == "--asset")
            config.assetPath = next();
        else if(arg == "--plugins")
            config.pluginDir = next();
        else if(arg == "--width")
            config.width = static_cast<uint32_t>(std::atoi(next().c_str()));
        else if(arg == "--height")
            config.height = static_cast<uint32_t>(std::atoi(next().c_str()));
        else if(arg == "--frames")
            config.frames = std::atoi(next().c_str());
        else if(arg == "--toggle-every")
            config.toggleEvery = std::atoi(next().c_str());
        else if(arg == "--no-vsync")
            config.vsync = false;
        else if(arg == "--validation")
            config.validation = true;
        else if(arg == "--no-outline")
            config.outline = false;
        else if(arg == "--capture")
            config.capturePath = next();
        else if(arg == "--capture-frame")
            config.captureFrame = std::atoi(next().c_str());
        else
            std::printf("unknown option '%s'\n", arg.c_str());
    }
    return config;
}

namespace {

struct WindowState {
    bool closed = false;
    bool resized = false;
    bool toggle = false;
    uint32_t width = 0;
    uint32_t height = 0;
};

LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto *state = reinterpret_cast<WindowState *>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if(state != nullptr) {
        switch(message) {
        case WM_CLOSE:
            state->closed = true;
            return 0;
        case WM_SIZE:
            state->width = LOWORD(lparam);
            state->height = HIWORD(lparam);
            state->resized = true;
            return 0;
        case WM_KEYDOWN:
            if(wparam == 'O')
                state->toggle = true;
            else if(wparam == VK_ESCAPE)
                state->closed = true;
            return 0;
        default:
            break;
        }
    }
    return DefWindowProcA(hwnd, message, wparam, lparam);
}

class WindowHost {
public:
    ~WindowHost() {
        if(_hwnd != nullptr)
            DestroyWindow(_hwnd);
    }

    bool create(uint32_t width, uint32_t height, const char *title) {
        _instance = GetModuleHandleA(nullptr);
        WNDCLASSA wc{};
        wc.lpfnWndProc = windowProc;
        wc.hInstance = _instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "RendererPilot";
        RegisterClassA(&wc);

        RECT rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        _hwnd = CreateWindowExA(0, wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, _instance, nullptr);
        if(_hwnd == nullptr)
            return false;
        SetWindowLongPtrA(_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&_state));
        _state.width = width;
        _state.height = height;
        ShowWindow(_hwnd, SW_SHOW);
        return true;
    }

    bool poll() {
        MSG message;
        while(PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        return !_state.closed;
    }

    void setTitle(const std::string &title) { SetWindowTextA(_hwnd, title.c_str()); }
    WindowState &state() { return _state; }
    HWND hwnd() const { return _hwnd; }
    HINSTANCE instance() const { return _instance; }

private:
    HINSTANCE _instance = nullptr;
    HWND _hwnd = nullptr;
    WindowState _state;
};

void printDiagnostics(const std::vector<Diagnostic> &diagnostics) {
    for(const Diagnostic &diagnostic : diagnostics)
        std::printf("%s\n", toString(diagnostic).c_str());
}

}

int runPilot(const PilotConfig &config, const PassRegistry &registry) {
    WindowHost window;
    if(!window.create(config.width, config.height, "RendererPilot")) {
        std::printf("window creation failed\n");
        return -1;
    }

    std::string error;
    GpuRenderer renderer;
    if(!renderer.initialize(window.instance(), window.hwnd(), config.width, config.height, config.vsync,
                            config.validation, error)) {
        std::printf("gpu: %s\n", error.c_str());
        return -1;
    }
    std::printf("gpu: %s\n", renderer.deviceName().c_str());

    // 에셋이 잘못되면 그래프를 만들기 전에 끝낸다.
    SceneManager scene;
    if(!scene.load(config.assetPath, error)) {
        std::printf("asset: %s\n", error.c_str());
        return -1;
    }
    std::printf("%s\n", scene.auditText().c_str());

    SceneGeometry geometry{&scene.view()};
    CameraFrame camera = scene.defaultCamera(static_cast<float>(config.width) / static_cast<float>(config.height));
    LightFrame light = scene.defaultLight();
    auto bindAppInputs = [&](Graph &graph) {
        graph.bindInput(kSceneGeometry, &geometry);
        graph.bindInput(kCameraFrame, &camera);
        graph.bindInput(kLightFrame, &light);
    };

    PilotSettings settings;
    settings.outline = config.outline;
    std::vector<Diagnostic> diagnostics;
    std::unique_ptr<Graph> graph = BuildGraph(registry, settings, diagnostics);
    printDiagnostics(diagnostics);
    if(graph == nullptr) {
        std::printf("graph build failed\n");
        return -1;
    }
    bindAppInputs(*graph);
    renderer.prepare(*graph, error);
    std::printf("graph (outline %s, %zu passes):\n%s", settings.outline ? "on" : "off", graph->nodes().size(),
                graph->dump().c_str());

    int frame = 0;
    int result = 0;
    while(window.poll()) {
        WindowState &state = window.state();
        if(state.resized) {
            state.resized = false;
            renderer.resize(state.width, state.height);
        }

        bool toggle = state.toggle || (config.toggleEvery > 0 && frame > 0 && frame % config.toggleEvery == 0);
        state.toggle = false;
        if(toggle) {
            // 새 그래프를 CPU에서 먼저 다 만든 뒤에만 교체한다. 실패하면 기존 그래프로 계속 그린다.
            PilotSettings next = settings;
            next.outline = !settings.outline;
            std::unique_ptr<Graph> candidate = BuildGraph(registry, next, diagnostics);
            printDiagnostics(diagnostics);
            if(candidate != nullptr) {
                renderer.releaseGraph();
                graph = std::move(candidate);
                bindAppInputs(*graph);
                renderer.prepare(*graph, error);
                settings = next;
                std::printf("graph switched: outline %s, %zu passes\n", settings.outline ? "on" : "off",
                            graph->nodes().size());
            } else {
                std::printf("graph switch failed, keeping the current graph\n");
            }
        }

        if(!config.capturePath.empty() && frame == config.captureFrame)
            renderer.requestCapture(config.capturePath);
        if(!renderer.renderFrame(*graph, error)) {
            std::printf("render: %s\n", error.c_str());
            result = -1;
            break;
        }
        ++frame;
        if(frame % 60 == 0)
            window.setTitle("RendererPilot | " + renderer.deviceName() + " | outline " +
                            (settings.outline ? "on" : "off") + " | " + std::to_string(graph->nodes().size()) +
                            " passes | frame " + std::to_string(frame));
        if(config.frames > 0 && frame >= config.frames)
            break;
    }

    renderer.waitIdle();
    graph.reset();
    renderer.shutdown();
    std::printf("frames rendered: %d\n", frame);
    return result;
}

}
