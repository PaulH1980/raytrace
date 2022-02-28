#include <filesystem>

#include "FilePaths.h"
namespace fs = std::filesystem;
namespace RayTrace
{

    std::string ConvertPath(const std::string _path)
    {
        return fs::u8path(_path.data()).make_preferred().string();
    }


    std::string FileName(const std::string& _path)
    {
        return fs::u8path(_path.data()).filename().string();
    }

    bool Exists(const std::string& _path)
    {
        return fs::exists(fs::u8path(_path.data()));
    }

    bool IsDirectory(const std::string& _path)
    {
        return fs::is_directory(fs::u8path(_path.data()));
    }

    bool IsFile(const std::string& _path)
    {
        return fs::is_regular_file(ConvertPath(_path));     
    }

    std::size_t GetFileSize(const std::string& _path)
    {
        return fs::file_size(fs::u8path(_path.data()));
    }

    std::string GetWorkDirectory()
    {
        auto myPath = fs::current_path();
        return ConvertPath( myPath.string() + "/");
    }

    std::string GetAssetDirectory()
    {
        return  ConvertPath(GetWorkDirectory() + "assets/");
    }

    std::string GetShaderDirectory()
    {
        return  ConvertPath(GetAssetDirectory() + "shaders/");
    }

    std::string GetTextureDirectory()
    {
        return  ConvertPath(GetAssetDirectory() + "textures/");
    }

    std::string GetModelDirectory()
    {
        return  ConvertPath(GetAssetDirectory() + "models/");
    }

    std::string ParentPath(const std::string& _path)
    {
        return ConvertPath(fs::u8path(_path.data()).parent_path().string() + "/" );
    }

    std::string RelativePath(const std::string& _parent, const std::string& _fullPath)
    {
        if (!Exists(_parent) || !IsDirectory(_parent))
            return "";
        if (!Exists(_fullPath))
            return "";
        const auto parentPath = fs::u8path(_parent.data());
        const auto fullPath = fs::u8path(_fullPath);
        const std::string suffix = IsDirectory(_fullPath) ? "/" : "";
        const auto result  = fs::relative(fullPath, parentPath).string() + suffix;
        return ConvertPath(result);
    }

    std::string ChangeExtension(const std::string& _path, const std::string& _new)
    { 
        const auto newPath = fs::u8path(_path.data()).replace_extension(fs::u8path(_new));
       
        
        return ConvertPath(newPath.string());
    }

    std::string GetExtension(const std::string& _path)
    {
        return fs::u8path(_path.data()).extension().string();
    }

    std::vector<std::string> GetAllFiles(const std::string& _path, bool _recurse, FileFilter _pFilter)
    {
        std::vector<std::string> ret;

        auto AddFileTest = [&](const fs::directory_entry& _entry) {
            if (_entry.is_regular_file() || _entry.is_directory() ) {
                bool add = true;
                if (_pFilter)
                    add = _pFilter(_entry.path().string());
                if (add)
                    ret.push_back(ConvertPath(_entry.path().string()));
            }
        };

        const auto startPath = fs::u8path(_path.data());
        if (!_recurse) {
            for (auto const& entry : std::filesystem::directory_iterator{ startPath })
                AddFileTest(entry);
        }
        else
        {
            for (auto& entry : fs::recursive_directory_iterator(startPath))
                AddFileTest(entry);
        }
        return ret;
    }

    std::vector<std::string> GetFiles(const std::string& _path, bool _recurse /*= false*/, FileFilter _pFilter /*= nullptr*/)
    {
        if (!IsDirectory(_path))
            return {};
        
        
        std::vector<std::string> ret;

        auto AddFileTest = [&](const fs::directory_entry& _entry) {
            if (_entry.is_regular_file()) {
                bool add = true;
                if (_pFilter)
                    add = _pFilter(_entry.path().string());
                if (add)
                    ret.push_back(ConvertPath(_entry.path().string()));
            }
        };

        const auto startPath = fs::u8path(_path.data());
        if (!_recurse) {
            for (auto const& entry : std::filesystem::directory_iterator{ startPath })
               AddFileTest(entry);   
        }
        else
        {
            for (auto& entry : fs::recursive_directory_iterator(startPath))
                AddFileTest(entry);   
        }       
        return ret;
    }

}

