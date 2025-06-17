#include "util/camera.hpp"
#include "util/log.hpp"
#include "util/util.hpp"
#include "gfx/program.hpp"
#include "gfx/uniform.hpp"

void Camera::update() {
    this->view_proj = this->proj * this->view;
    this->inv_proj = math::inverse(this->proj);
    this->inv_view = math::inverse(this->view);
    this->inv_view_proj = math::inverse(this->view_proj);
    this->position = (inv_view_proj * vec4(0.5f, 0.5f, 0.0f, 1.0f)).xyz(),
    this->direction =
        math::normalize(
            (this->inv_view_proj * vec4(0.5f, 0.5f, 0.1f, 1.0f)).xyz()
                - this->position);
}

UniformValueList &Camera::uniforms(
    UniformValueList &dst,
    std::string_view prefix) const {
    const auto p = std::string(prefix);
    dst.set(p + "_view", this->view);
    dst.set(p + "_proj", this->proj);
    dst.set(p + "_viewProj", this->view_proj);
    dst.set(p + "_invView", this->inv_view);
    dst.set(p + "_invProj", this->inv_proj);
    dst.set(p + "_invViewProj", this->inv_view_proj);
    dst.set(p + "_position", this->position);
    dst.set(p + "_direction", this->direction);
    return dst;
}
