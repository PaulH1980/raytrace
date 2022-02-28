#include "UiIncludes.h"
#include "FilePaths.h"
#include "HwTexture.h"
#include "IO.h"
#include "Common.h"
#include "DragDrop.h"
#include "ResourceExplorer.h"

namespace RayTrace
{
   
    bool IsShader(const std::string _fileName)
    {
        if (ToLower(GetExtension(_fileName)) == ".vert")
            return true;      
        if (ToLower(GetExtension(_fileName)) == ".frag")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".comp")
            return true;
        return false;
    }

    bool IsAudio(const std::string& _fileName)
    {
        if (ToLower(GetExtension(_fileName)) == ".ogg")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".wav")
            return true;        
        return false;
    }

    bool IsImage(const std::string& _fileName)
    {
        if (ToLower(GetExtension(_fileName)) == ".jpg")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".png")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".hdr")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".tga")
            return true;

        return false;
    }

    bool IsMaterial(const std::string& _fileName)
    {
        if (ToLower(GetExtension(_fileName)) == ".mtl")
            return true;
        
        return false;
    }

    bool IsModel(const std::string& _fileName)
    {
        if (ToLower(GetExtension(_fileName)) == ".gltf")
            return true;
        if (ToLower(GetExtension(_fileName)) == ".obj")
            return true;
        return false;
    }

    eResourceType GetResourceType(const std::string& _fileName)
    {
        if (IsAudio(_fileName))
            return eResourceType::RESOURCE_AUDIO;
        if (IsImage(_fileName))
            return eResourceType::RESOURCE_IMAGE;
        if(IsMaterial(_fileName))
            return eResourceType::RESOURCE_MATERIAL;
        if (IsShader(_fileName))
            return eResourceType::RESOURCE_SHADER;
        if (IsModel(_fileName))
            return eResourceType::RESOURCE_MODEL;
        return RESOURCE_UNKNOWN;
    }



    FileDisplayNodeUPtr  GetNodes( FileDisplayNode* _pParent,  const std::string& _curDir)
    {
        auto rootNode        = std::make_unique<FileDisplayNode>();
        rootNode->m_fullPath = _curDir;       
        if (_pParent) {
            rootNode->m_shortName = RelativePath(_pParent->m_fullPath, _curDir);
            rootNode->m_parent = _pParent;
        }
        else
            rootNode->m_shortName = RelativePath(GetWorkDirectory(), _curDir);
        
        const auto files = GetAllFiles(_curDir);
        for (const auto& file : files)
        {
            if (IsFile(file)) {

                FileType type;
                type.m_fullPath     = file;
                type.m_shortName    = FileName(file);
                type.m_resourceType = GetResourceType(file);
                if( type.m_resourceType != RESOURCE_UNKNOWN )
                    rootNode->m_files.push_back(type);
            }
            else if (IsDirectory(file))
                rootNode->m_subFolders.push_back(GetNodes( rootNode.get(), file));
        }
        return std::move(rootNode);
    }
   



   

    //////////////////////////////////////////////////////////////////////////
    //ResourceExplorer
    //////////////////////////////////////////////////////////////////////////
    ResourceExplorer::ResourceExplorer(const WindowInfo& _hint) 
        : Window(_hint) 
        
    {

    }


    void ResourceExplorer::Layout()
    {

    }

    void ResourceExplorer::Resize(int _width, int _height)
    {

    }

    void ResourceExplorer::PreDraw()
    {

    }

    void ResourceExplorer::Draw()
    {
        bool visible = IsVisible();

        if (!m_assetRootNode)
        {
            auto node = GetNodes(nullptr, GetAssetDirectory());
            m_assetRootNode = std::move(node);
            m_pActiveDirectory = m_assetRootNode.get();
        }


        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin(m_info.m_name.c_str(), &visible);   

        ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
        const float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		const auto size = ImGui::GetContentRegionAvail();
		const auto colCount = Ceil2Int(size.x / 128.0f);
		//loaded resources
		if (ImGui::BeginTable("split", colCount))
		{
           if (m_pActiveDirectory)
                DisplayCurrentFolder();

			ImGui::EndTable();
		}        
        ImGui::PopStyleVar();        
        ImGui::End();
        SetVisible(visible);
    }

    void ResourceExplorer::PostDraw()
    {

    }

    //void ResourceExplorer::DrawLoadedResources()
    //{
    //  //  const auto size = ImGui::GetContentRegionAvail();
    //  //  const auto colCount = Ceil2Int(size.x / 96.0f);
    //    //loaded resources
    //   // if (ImGui::BeginTable("split", colCount))
    //    {
    //        auto textures = GetContext().GetResourceManager().GetTextures(true, true);

    //        for (auto pTex : textures)
    //        {
    //            if (!pTex->Is2D())
    //                continue;

    //         //   if (ImGui::TableNextColumn())
    //         //   {
    //                const auto size = ImGui::GetContentRegionAvail();
    //                ImGui::Image((ImTextureID)pTex->m_texInfo.m_id, ImVec2(size.x, size.x));
    //         //   }
    //        }
    //        //ImGui::EndTable();
    //    }
    //}

    void ResourceExplorer::DisplayKnownResources(FileDisplayNode* _pNode)
    {
		for (auto& fileInfo : _pNode->m_files)
		{
            if (fileInfo.m_resourceType == RESOURCE_IMAGE)
            {

            }
            else
            {

            }
            if (fileInfo.m_resourceType == RESOURCE_IMAGE &&
				fileInfo.m_resourceStatus == RESOURCE_STATUS_PENDING)
			{
				auto result = AddImage(fileInfo.m_fullPath);
				if (result)
					fileInfo.m_resourceStatus = RESOURCE_STATUS_LOADED;
				else
					fileInfo.m_resourceStatus = RESOURCE_STATUS_ERROR;
			}
			if (fileInfo.m_resourceStatus == RESOURCE_STATUS_LOADED)
			{
				//if (ImGui::TableNextColumn())
                if (ImGui::TableNextColumn())
                {
                    const auto size    = ImGui::GetContentRegionAvail();
                    const auto& imgInfo = m_info.m_manager->GetImageFromCache(fileInfo.m_fullPath);
                    ImGui::ImageButton((ImTextureID)imgInfo.m_pTexture->m_texInfo.m_id, ImVec2(size.x, size.x));
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text(fileInfo.m_shortName.c_str());
                        ImGui::EndTooltip();
                    }
                    if (ImGui::BeginDragDropSource())//drag n drop
                    {
                        ImGui::SetDragDropPayload(TEXTURE_DND, fileInfo.m_fullPath.c_str(), fileInfo.m_fullPath.length() + 1 );
                        ImGui::EndDragDropSource();
                    }
                }
			}
		}	
	}

	void ResourceExplorer::DisplayFolders(FileDisplayNode* _pNode)
	{

		if (ImGui::TableNextColumn())
		{
			const auto size = ImGui::GetContentRegionAvail();
            auto texPtr = GetContext().GetResourceManager().GetTexture("Folder");
			ImGui::ImageButton((ImTextureID)texPtr->m_texInfo.m_id, ImVec2(size.x, size.x));
			if (ImGui::IsItemHovered() && ImGui::IsItemClicked())
			{
				m_pActiveDirectory = _pNode;
			}
            ImGui::Text(_pNode->m_shortName.c_str());
			
		}
	}

    bool ResourceExplorer::AddImage(const std::string& _fileName)
    {

        return m_info.m_manager->AddImageToCache(_fileName);
    }

	void ResourceExplorer::DisplayCurrentFolder()
	{
        if (!m_pActiveDirectory->isRoot()) //go back to parent folder
		{
			//  ImGui::TableNextRow();
			if (ImGui::TableNextColumn())
			{
				const auto size = ImGui::GetContentRegionAvail();
                const auto pos  = ImGui::GetWindowPos();
                const auto textSize = ImGui::CalcTextSize("..");
                auto texPtr = GetContext().GetResourceManager().GetTexture("ParentFolder");
				ImGui::ImageButton((ImTextureID)texPtr->m_texInfo.m_id, ImVec2(size.x, size.x));
				if (ImGui::IsItemHovered() && ImGui::IsItemClicked())
				{
					m_pActiveDirectory = m_pActiveDirectory->m_parent;
					return;
				}
                ImGui::Text("..");
				
			}
		}
		for (auto& subNode : m_pActiveDirectory->m_subFolders)
		{
			DisplayFolders(subNode.get());
		}
		if (m_pActiveDirectory->m_files.size())
		{
			DisplayKnownResources(m_pActiveDirectory);
		}
	}

}

