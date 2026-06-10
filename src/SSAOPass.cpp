#include "SSAOPass.hpp"

#include "SSAOShaders.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

float rand01()
{
    static std::mt19937 gen(42);
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen);
}

float lerp(float a, float b, float t) { return a + t * (b - a); }

} // namespace

SSAOPass::SSAOPass()
    : shader_(SSAOShaders::vertex, SSAOShaders::fragment)
{
    // ── Generate 64-sample hemisphere kernel (tangent space, +Z = normal) ──
    // Samples are biased toward the horizon (low Z) for more lateral occlusion.
    kernel_.reserve(128);
    for (int i = 0; i < 128; ++i) {
        Vec3 sample(rand01() * 2.0f - 1.0f,
                    rand01() * 2.0f - 1.0f,
                    rand01());
        sample = normalize(sample);

        // Scale: more samples close to core, but biased toward horizon.
        float scale = static_cast<float>(i) / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample = sample * scale;

        // Bias Z toward horizon: more samples look sideways.
        float horizonBias = lerp(0.01f, 1.0f, rand01() * rand01());
        sample.z = std::max(sample.z, 0.001f);  // ensure hemisphere
        sample = sample * horizonBias;

        kernel_.push_back(sample);
    }

    // ── Generate 4x4 random-rotation noise texture ──
    std::vector<float> noiseData(4 * 4 * 2);
    for (size_t i = 0; i < noiseData.size(); i += 2) {
        Vec3 v(rand01() * 2.0f - 1.0f, rand01() * 2.0f - 1.0f, 0.0f);
        v = normalize(v);
        noiseData[i]     = v.x;
        noiseData[i + 1] = v.y;
    }

    glGenTextures(1, &noiseTexture_);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 4, 4, 0, GL_RG, GL_FLOAT, noiseData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAOPass::render(const SSAOBuffer& output, const GeometryBuffer& gbuffer,
                    const Mat4& view, const Mat4& proj,
                    int width, int height) const
{
    output.bind();
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    shader_.use();

    gbuffer.bindPosition(GL_TEXTURE0);
    gbuffer.bindNormal(GL_TEXTURE1);
    gbuffer.bindDepth(GL_TEXTURE3);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);

    shader_.setInt("uPosition", 0);
    shader_.setInt("uNormal", 1);
    shader_.setInt("uDepth", 3);
    shader_.setInt("uNoise", 6);

    shader_.setMat4("uView", view);
    shader_.setMat4("uProjection", proj);

    for (size_t i = 0; i < kernel_.size(); ++i) {
        shader_.setVec3(("uSamples[" + std::to_string(i) + "]").c_str(), kernel_[i]);
    }

    shader_.setFloat("uRadius", 60.0f);
    shader_.setFloat("uBias", 0.5f);
    shader_.setFloat("uNear", 10.0f);
    shader_.setFloat("uFar", 2000.0f);
    shader_.setFloat("uScreenWidth", static_cast<float>(width));
    shader_.setFloat("uScreenHeight", static_cast<float>(height));

    fullscreenTriangle_.draw();

    glBindVertexArray(0);
}
