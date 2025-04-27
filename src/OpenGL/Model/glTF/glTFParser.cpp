#include "glTFParser.h"


std::optional<fastgltf::Asset> glTFParser::Parse(const std::filesystem::path& p)
{
    auto mappedFile = fastgltf::MappedGltfFile::FromPath(p);
    if (!mappedFile)
    {
        wprintf(L"[glTFParser] Failed to open %s!\n", p.wstring().c_str());
        return std::nullopt;
    }

    fastgltf::Parser parser(supportedExtensions);

    auto result = parser.loadGltf(mappedFile.get(), p.parent_path(), gltfOptions);
    if (result.error() != fastgltf::Error::None)
    {
        std::cerr << "[glTFParser] Parse error:" << fastgltf::getErrorMessage(result.error()) << std::endl;
        return std::nullopt;
    }

    return std::optional<fastgltf::Asset>{std::move(result.get())};
}