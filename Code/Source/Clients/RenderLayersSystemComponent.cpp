
#include "RenderLayersSystemComponent.h"

#include <RenderLayers/RenderLayersTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace RenderLayers
{
    AZ_COMPONENT_IMPL(RenderLayersSystemComponent, "RenderLayersSystemComponent",
        RenderLayersSystemComponentTypeId);

    void RenderLayersSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RenderLayersSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void RenderLayersSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RenderLayersService"));
    }

    void RenderLayersSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RenderLayersService"));
    }

    void RenderLayersSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void RenderLayersSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    RenderLayersSystemComponent::RenderLayersSystemComponent()
    {
        if (RenderLayersInterface::Get() == nullptr)
        {
            RenderLayersInterface::Register(this);
        }
    }

    RenderLayersSystemComponent::~RenderLayersSystemComponent()
    {
        if (RenderLayersInterface::Get() == this)
        {
            RenderLayersInterface::Unregister(this);
        }
    }

    void RenderLayersSystemComponent::Init()
    {
    }

    void RenderLayersSystemComponent::Activate()
    {
        RenderLayersRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void RenderLayersSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        RenderLayersRequestBus::Handler::BusDisconnect();
    }

    void RenderLayersSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace RenderLayers
