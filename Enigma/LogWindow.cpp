#include "UiIncludes.h"
#include "LogWindow.h"

namespace RayTrace
{
    struct LogWindow::WindowRecipient : public LogRecipient
    {
    
            WindowRecipient(LogWindow* _pWindow)
                : m_logWindow(_pWindow)
                , m_autoScroll(true)
            {

            }

            LogWindow* m_logWindow = nullptr;
            void Receive(const LogMessage& _msg) {
                m_logWindow->AddLog("[%s] [%s] [Time] [%f]\n", LogLevelToString(_msg.m_level).c_str(), _msg.m_msg.c_str(), _msg.m_time);
            }

            ImGuiTextBuffer     m_textBuffer;
            ImGuiTextFilter     m_filter;
            ImVector<int>       m_lineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
            bool                m_autoScroll;  // Keep scrolling if already at the bottom.

        
    };




    //////////////////////////////////////////////////////////////////////////
    //LogWindow
    //////////////////////////////////////////////////////////////////////////
    LogWindow::LogWindow(const WindowInfo& _hint)
        : Window(_hint)
        , m_recipient(std::make_shared<WindowRecipient>(this))
    {
        m_recipient->m_autoScroll = true;
        m_info.m_manager->GetContext().GetLogger().AddRecipient(m_info.m_name, m_recipient);

        Clear();
    }

    void LogWindow::Layout()
    {

    }

    void LogWindow::Resize(int _width, int _height)
    {
        (_width);
        (_height);
    }

    void LogWindow::PreDraw()
    {

    }

    void LogWindow::Draw()
    {
        bool visible = IsVisible();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1);
        ImGui::Begin(m_info.m_name.c_str(), &visible);
        ImGui::PopStyleVar();

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &m_recipient->m_autoScroll);
            ImGui::EndPopup();
        }

        // Main window
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        m_recipient->m_filter.Draw("Filter", -100.0f);

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        if (clear)
            Clear();
        if (copy)
            ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = m_recipient->m_textBuffer.begin();
        const char* buf_end = m_recipient->m_textBuffer.end();
        if (m_recipient->m_filter.IsActive())
        {
            // In this example we don't use the clipper when Filter is enabled.
            // This is because we don't have a random access on the result on our filter.
            // A real application processing logs with ten of thousands of entries may want to store the result of
            // search/filter.. especially if the filtering function is not trivial (e.g. reg-exp).
            for (int line_no = 0; line_no < m_recipient->m_lineOffsets.Size; line_no++)
            {
                const char* line_start = buf + m_recipient->m_lineOffsets[line_no];
                const char* line_end = (line_no + 1 < m_recipient->m_lineOffsets.Size) ? (buf + m_recipient->m_lineOffsets[line_no + 1] - 1) : buf_end;
                if (m_recipient->m_filter.PassFilter(line_start, line_end))
                    ImGui::TextUnformatted(line_start, line_end);
            }
        }
        else
        {
            // The simplest and easy way to display the entire buffer:
            //   ImGui::TextUnformatted(buf_begin, buf_end);
            // And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward
            // to skip non-visible lines. Here we instead demonstrate using the clipper to only process lines that are
            // within the visible area.
            // If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them
            // on your side is recommended. Using ImGuiListClipper requires
            // - A) random access into your data
            // - B) items all being the  same height,
            // both of which we can handle since we an array pointing to the beginning of each line of text.
            // When using the filter (in the block of code above) we don't have random access into the data to display
            // anymore, which is why we don't use the clipper. Storing or skimming through the search result would make
            // it possible (and would be recommended if you want to search through tens of thousands of entries).
            ImGuiListClipper clipper;
            clipper.Begin(m_recipient->m_lineOffsets.Size);
            while (clipper.Step())
            {
                for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
                {
                    const char* line_start = buf + m_recipient->m_lineOffsets[line_no];
                    const char* line_end = (line_no + 1 < m_recipient->m_lineOffsets.Size) ? (buf + m_recipient->m_lineOffsets[line_no + 1] - 1) : buf_end;
                    ImGui::TextUnformatted(line_start, line_end);
                }
            }
            clipper.End();
        }
        ImGui::PopStyleVar();

        if (m_recipient->m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();

        SetVisible(visible);
    }

    void LogWindow::PostDraw()
    {

    }


    void LogWindow::Clear()
    {
        m_recipient->m_textBuffer.clear();
        m_recipient->m_lineOffsets.clear();
        m_recipient->m_lineOffsets.push_back(0);
    }

    void LogWindow::AddLog(const char* fmt, ...) IM_FMTARGS(3)
    {
        int old_size = m_recipient->m_textBuffer.size();
        va_list args;
        va_start(args, fmt);
        m_recipient->m_textBuffer.appendfv(fmt, args);
        va_end(args);
        for (int new_size = m_recipient->m_textBuffer.size(); old_size < new_size; old_size++)
            if (m_recipient->m_textBuffer[old_size] == '\n')
                m_recipient->m_lineOffsets.push_back(old_size + 1);
    }
}

