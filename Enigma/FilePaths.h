#pragma once

#include "FrontEndDef.h"
namespace RayTrace
{
    using FileFilter = std::function<bool(const std::string&)>;

    bool                        Exists(const std::string& _path);

    bool                        IsDirectory(const std::string& _path);

    bool                        IsFile(const std::string& _path);

    std::size_t                 GetFileSize(const std::string& _path);

    std::string                 GetWorkDirectory();

    std::string                 GetAssetDirectory();

    std::string                 GetShaderDirectory();

    std::string                 GetTextureDirectory();

    std::string                 GetModelDirectory();

    std::string                 ParentPath( const std::string& _path );

    std::string                 RelativePath(const std::string& _parent, const std::string& _fullPath);

    std::string                 ChangeExtension(const std::string& _path, const std::string& _new);

    std::string                 GetExtension(const std::string& _path);

    std::vector<std::string>    GetAllFiles(const std::string& _path, bool _recurse = false, FileFilter _pFilter = nullptr);

    std::vector<std::string>    GetFiles(const std::string& _path, bool _recurse = false, FileFilter _pFilter = nullptr);

    std::string                 ConvertPath(const std::string _path);

    std::string                 FileName(const std::string& _path);
}