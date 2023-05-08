import os
from os.path import *
PJ_ROOT = join(split(realpath(__file__))[0], "..")
with open(join(PJ_ROOT, "Source", "Generated", "ShaderHeader.inl"), "w") as f:
    f.write("#pragma once\n\n")
    generate_dir = join(PJ_ROOT, "Source", "Generated")
    for root, dirs, files in os.walk(join(generate_dir, "Spv")):
        for filename in files:
            rela_path = relpath(join(root, filename), generate_dir).replace(os.path.sep, "/")
            f.write(f"#include <{rela_path}>\n")
    
    f.write("\n")
    for root, dirs, files in os.walk(join(generate_dir, "Dxil")):
        for filename in files:
            rela_path = relpath(join(root, filename), generate_dir).replace(os.path.sep, "/")
            f.write(f"#include <{rela_path}>\n")

    f.write("\n#include<unordered_map>\n")
    f.write("#include<string_view>\n\n")
    f.write(
        "struct ShaderByte\n"
        "{\n"
        "    struct ShaderByteItem\n"
        "    {\n"
        "        uint8_t* mpData     = nullptr;\n"
        "        uint32_t mByteCount = 0;\n"
        "    };\n"
        "\n"
        "    ShaderByteItem mSpv;\n"
        "    ShaderByteItem mDxil;\n"
        "};\n\n"
    )

    f.write("\nconst static std::unordered_map<std::string_view, ShaderByte> gs_SHADER_BYTE_MAP\n{\n")
    for root, dirs, files in os.walk(join(generate_dir, "Spv")):
        for filename in files:
            original_fn = rela_path = relpath(join(root, filename), join(generate_dir, "Spv")).replace(os.path.sep, "/")[:-6] + ".glsl"
            variable_name = filename[:-6].replace(".", "_")
            dxil_bytes = f"(uint8_t*)DXIL_{variable_name}, sizeof(DXIL{variable_name})" if exists(join(root, filename).replace("Spv", "Dxil").replace(".spv", ".dxil")) else ""
            f.write("    {" + f"\"{original_fn}\", " + "ShaderByte{ .mSpv{ " + f"(uint8_t*)SPV_{variable_name}, sizeof(SPV_{variable_name})" + "}, .mDxil{" + f"{dxil_bytes}" + "}}},\n")
    f.write("};\n")
