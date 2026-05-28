
#pragma once

namespace RenderLayers
{
    // System Component TypeIds
    inline constexpr const char* RenderLayersSystemComponentTypeId = "{DD23462B-29E8-45A5-811D-F5FD3BA114D9}";
    inline constexpr const char* RenderLayersEditorSystemComponentTypeId = "{0C6E550D-7AE7-485B-AF30-EFEEA460870A}";

    // Module derived classes TypeIds
    inline constexpr const char* RenderLayersModuleInterfaceTypeId = "{46B4DEDF-56D5-4CA7-8464-EA89A83F4A25}";
    inline constexpr const char* RenderLayersModuleTypeId = "{126820EA-F8EA-4350-BD39-45002750F725}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* RenderLayersEditorModuleTypeId = RenderLayersModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* RenderLayersRequestsTypeId = "{53C46004-4834-48A5-9DA0-BDAD628949E2}";
} // namespace RenderLayers
