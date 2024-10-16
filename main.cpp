#include <alpine/alpine.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <numbers>
#include <vector>
#include <string_view>
#include <chrono>
#include <thread>

// https://github.com/richgel999/fpng
#include "fpng.h"

int32_t main(int32_t argc, const char* argv[]) {
    // レンダラー起動時間を取得。
    using clock = std::chrono::high_resolution_clock;
    const clock::time_point appStartTp = clock::now();

    uint32_t timeLimit = UINT32_MAX;
    uint32_t frameCount = 0;
    std::string_view gltfName = "";
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t spp = 0;

    for (int argIdx = 1; argIdx < argc; ++argIdx) {
        std::string_view arg = argv[argIdx];
        if (arg == "--time-limit") {
            if (argIdx + 1 >= argc) {
                printf("missing value.\n");
                return -1;
            }
            timeLimit = static_cast<uint32_t>(atoi(argv[argIdx + 1]));
            argIdx += 1;
        }
        else if (arg == "--frameCount")
        {
            if (argIdx + 1 >= argc)
            {
                return -1;
            }
            frameCount = static_cast<uint32_t>(atoi(argv[argIdx + 1]));
            argIdx++;
        }
        else if (arg == "--gltf")
        {
            if (argIdx + 1 >= argc)
            {
                return -1;
            }
            gltfName = argv[argIdx + 1];
            argIdx++;
        }
        else if (arg == "--width")
        {
            if (argIdx + 1 >= argc)
            {
                return -1;
            }
            width = static_cast<uint32_t>(atoi(argv[argIdx + 1]));
            argIdx++;
        }
        else if (arg == "--height")
        {
            if (argIdx + 1 >= argc)
            {
                return -1;
            }
            height = static_cast<uint32_t>(atoi(argv[argIdx + 1]));
            argIdx++;
        }
        else if (arg == "--spp")
        {
            if (argIdx + 1 >= argc)
            {
                return -1;
            }
            spp = static_cast<uint32_t>(atoi(argv[argIdx + 1]));
            argIdx++;
        }
        else {
            printf("Unknown argument %s.\n", argv[argIdx]);
            return -1;
        }
    }

    const uint32_t maxDepth = 8;

    alpine::initialize(width, height, maxDepth);
    alpine::setBackgroundColor(0.0f, 0.0f, 0.0f);

    const float eye[] = { 9.636078f, -0.777524f, -8.820151f };
    const float target[] = { 0.0f, 2.0f, 0.0f };
    const float up[] = { 0.0f, 1.0f, 0.0f };
    const float fovy = std::numbers::pi_v<float> / 2.0f;
    float aspect = float(width) / float(height);

    auto* camera = alpine::getCamera();
    camera->setLookAt(eye, target, up, fovy, aspect);

    bool loaded = alpine::load(gltfName, alpine::FileType::Gltf);
    if (!loaded)
    {
        printf("ERROR: failed to load %s\n", gltfName.data());
        return -1;
    }

    alpine::buildLightSampler(alpine::LightSamplerType::Bvh);

    fpng::fpng_init();

    float delta = 1.0f / static_cast<float>(std::max(frameCount - 1, 1u));
    float theta = -1.5f * std::numbers::pi_v<float> * delta;
    float phi = 0.30f * std::numbers::pi_v<float> * delta;

    camera->orbit(-0.5f * std::numbers::pi_v<float>, 0.0f);

    for (uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
        const clock::time_point frameStartTp = clock::now();
        printf("Frame %u ... ", frameIndex);

        alpine::resetAccumulation();
        alpine::render(spp);
        alpine::resolve(true);

        // 起動からの時刻とフレーム時間を計算。
        const clock::time_point now = clock::now();
        const clock::duration frameTime = now - frameStartTp;
        const clock::duration totalTime = now - appStartTp;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count() > 1e+3f * timeLimit)
            break;
        printf(
            "Done: %.3f [ms] (total: %.3f [s])\n",
            std::chrono::duration_cast<std::chrono::microseconds>(frameTime).count() * 1e-3f,
            std::chrono::duration_cast<std::chrono::milliseconds>(totalTime).count() * 1e-3f);

        // 3桁連番で画像出力。
        char filename[256];
        sprintf_s(filename, "%03u.png", frameIndex);
        const void* pixels = alpine::getFrameBuffer();
        fpng::fpng_encode_image_to_file(filename, pixels, width, height, 3, 0);

        camera->orbit(theta, phi);
    }

    return 0;
}
