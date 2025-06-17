#pragma once

#ifndef IN_UTIL_HPP
#warning "do not include me directly! use util.hpp"
#endif

// TODO: if supporting OpenGL targets, this will break things!
#define GLM_FORCE_LEFT_HANDED

#ifndef SHADER_TARGET_glsl
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define DEPTH_ZERO_TO_ONE
#elif
#define DEPTH_NEGATIVE_ONE_TO_ONE
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_EXPLICIT_CTOR
#define GLM_FORCE_INLINE

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/quaternion.hpp>
