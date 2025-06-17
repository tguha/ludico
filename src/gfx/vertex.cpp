#include "gfx/vertex.hpp"

VertexTypeManager &VertexTypeManager::instance() {
    static VertexTypeManager instance;
    return instance;
}

void VertexTypeManager::init() {
    VertexTypeRegistry::instance().initialize(*this);
}

template <> VertexTypeRegistry &VertexTypeRegistry::instance() {
    static VertexTypeRegistry instance;
    return instance;
}

DECL_VERTEX_TYPE(VertexDummy16) {
    VertexDummy16::layout
        .begin()
        .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
        .end();
}

DECL_VERTEX_TYPE(VertexPosition) {
    VertexPosition::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .end();
}

DECL_VERTEX_TYPE(VertexTexture) {
    VertexTexture::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
}

DECL_VERTEX_TYPE(VertexColor) {
    VertexColor::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .end();
}

DECL_VERTEX_TYPE(VertexColorNormal) {
    VertexColorNormal::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
        .end();
}

DECL_VERTEX_TYPE(VertexTextureNormal) {
    VertexTextureNormal::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
        .end();
}


DECL_VERTEX_TYPE(VertexTextureSpecularNormal) {
    VertexTextureSpecularNormal::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
        .end();
}

DECL_VERTEX_TYPE(VertexTextureColor) {
    VertexTextureColor::layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
        .end();
}
