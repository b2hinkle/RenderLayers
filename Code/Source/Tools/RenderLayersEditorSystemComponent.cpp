
#include <AzCore/Serialization/SerializeContext.h>
#include "RenderLayersEditorSystemComponent.h"

#include <RenderLayers/RenderLayersTypeIds.h>

namespace RenderLayers
{
    AZ_COMPONENT_IMPL(RenderLayersEditorSystemComponent, "RenderLayersEditorSystemComponent",
        RenderLayersEditorSystemComponentTypeId, BaseSystemComponent);

    void RenderLayersEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderLayersEditorSystemComponent, RenderLayersSystemComponent>()
                ->Version(0);
        }
    }

    RenderLayersEditorSystemComponent::RenderLayersEditorSystemComponent() = default;

    RenderLayersEditorSystemComponent::~RenderLayersEditorSystemComponent() = default;

    void RenderLayersEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("RenderLayersEditorService"));
    }

    void RenderLayersEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("RenderLayersEditorService"));
    }

    void RenderLayersEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void RenderLayersEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void RenderLayersEditorSystemComponent::Activate()
    {
        RenderLayersSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void RenderLayersEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        RenderLayersSystemComponent::Deactivate();
    }

} // namespace RenderLayers
