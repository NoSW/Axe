#include "01Resource/Image/Image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace axe::resource
{

void comiple_test()
{
    stbi_image_free(nullptr);
}

}  // namespace axe::resource