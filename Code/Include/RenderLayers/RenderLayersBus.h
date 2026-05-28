
#pragma once

#include <RenderLayers/RenderLayersTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace RenderLayers
{
    class RenderLayersRequests
    {
    public:
        AZ_RTTI(RenderLayersRequests, RenderLayersRequestsTypeId);
        virtual ~RenderLayersRequests() = default;
        // Put your public methods here
    };

    class RenderLayersBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using RenderLayersRequestBus = AZ::EBus<RenderLayersRequests, RenderLayersBusTraits>;
    using RenderLayersInterface = AZ::Interface<RenderLayersRequests>;

} // namespace RenderLayers
