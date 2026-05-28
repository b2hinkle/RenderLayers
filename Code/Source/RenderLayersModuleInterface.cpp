
#include "RenderLayersModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <RenderLayers/RenderLayersTypeIds.h>

#include <Clients/RenderLayersSystemComponent.h>

namespace RenderLayers
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(RenderLayersModuleInterface,
        "RenderLayersModuleInterface", RenderLayersModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(RenderLayersModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(RenderLayersModuleInterface, AZ::SystemAllocator);

    RenderLayersModuleInterface::RenderLayersModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            RenderLayersSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList RenderLayersModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<RenderLayersSystemComponent>(),
        };
    }
} // namespace RenderLayers
