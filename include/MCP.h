#pragma once
#include "Events.h"
#include "SKSEMCP/SKSEMenuFramework.hpp"


static void HelpMarker(const char* desc);

namespace UI {
    struct Instance {
        std::pair<StageNo,StageNo> stage_number;
        std::string stage_name;
        Count count;
        float start_time;
        Duration duration;
        float delay_magnitude;
        std::string delayer;
        bool is_fake;
        bool is_transforming;
        bool is_decayed;
    };
    using InstanceMap = std::map<std::string, std::vector<Instance>>;
    using LocationMap = std::map<std::string, InstanceMap>;

    inline std::string log_path = GetLogPath().string();
    inline std::vector<std::string> logLines;

    inline LocationMap locations;

    inline std::string last_generated = "";
    inline std::string item_current = "##current";
    inline std::string sub_item_current = "##item"; 
    inline bool is_list_box_focused = false;
    inline ImGuiTextFilter* filter;
    inline ImGuiTextFilter* filter2;
    inline ImGuiTableFlags table_flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;

    inline Manager* M;
    void __stdcall RenderSettings();
    void __stdcall RenderStatus();
    void __stdcall RenderInspect();
    void __stdcall RenderLog();
    void Register(Manager* manager);

};