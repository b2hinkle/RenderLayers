
#include <RenderLayers/RenderLayersTypeIds.h>
#include <RenderLayersModuleInterface.h>
#include "RenderLayersSystemComponent.h"

namespace RenderLayers
{
    class RenderLayersModule
        : public RenderLayersModuleInterface
    {
    public:
        AZ_RTTI(RenderLayersModule, RenderLayersModuleTypeId, RenderLayersModuleInterface);
        AZ_CLASS_ALLOCATOR(RenderLayersModule, AZ::SystemAllocator);
    };
}// namespace RenderLayers

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), RenderLayers::RenderLayersModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RenderLayers, RenderLayers::RenderLayersModule)
#endif
