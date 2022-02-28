#pragma once

#include "Window.h"

namespace RayTrace
{
    
    enum eResourceType
    {
        RESOURCE_UNKNOWN,
        RESOURCE_IMAGE,
        RESOURCE_AUDIO,
        RESOURCE_MATERIAL,
        RESOURCE_SHADER,
        RESOURCE_MODEL
    };

    enum eResourcePreviewStatus
    {
        RESOURCE_STATUS_PENDING,
        RESOURCE_STATUS_LOADED,
        RESOURCE_STATUS_ERROR    //error don't attempt loading again
    };


    struct FileDisplayNode;
    using FileDisplayNodeUPtr = std::unique_ptr<FileDisplayNode>;
    struct FileType
    {
        std::string     m_fullPath;
        std::string     m_shortName;
        eResourceType   m_resourceType          = RESOURCE_UNKNOWN;
        eResourcePreviewStatus m_resourceStatus = RESOURCE_STATUS_PENDING;
    };

    struct FileDisplayNode
    {
        bool            isRoot() const {
            return m_parent == nullptr;
        }        
        std::string                         m_shortName;
        std::string                         m_fullPath;       
        FileDisplayNode*                    m_parent = nullptr;
        std::vector<FileType>               m_files;            //files in this folder
        std::vector<FileDisplayNodeUPtr>    m_subFolders;
    };

	

    
    
    //////////////////////////////////////////////////////////////////////////
    //ResourceExplorer 
    //////////////////////////////////////////////////////////////////////////
    class ResourceExplorer : public Window
    {
    public:

        ResourceExplorer(const WindowInfo& _hint);

        void Layout() override;
        void Resize(int _width, int _height) override;
        void PreDraw() override;
        void Draw() override;
        void PostDraw() override;
       
     

    private:
        void DisplayCurrentFolder();
		void DisplayKnownResources(FileDisplayNode* _pNode);
		void DisplayFolders(FileDisplayNode* _pNode);
		bool AddImage(const std::string& _fileName);

        

      //  std::mutex                           m_cacheMutex;
      //  LRUCache<std::string, ImageInfo>     m_imageCache;
        FileDisplayNodeUPtr                  m_assetRootNode;
        FileDisplayNode*                     m_pActiveDirectory = nullptr;
    };
}