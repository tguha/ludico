#include "gfx/renderer_resource.hpp"
#include "gfx/renderer.hpp"
#include "global.hpp"

void detail::add_renderer_resource(IManagedResource *resource) {
    Renderer::get().managed_resources.push_back(resource);
}
