
#include <RenderLayers/RenderLayersTypeIds.h>
#include <RenderLayersModuleInterface.h>
#include "RenderLayersEditorSystemComponent.h"

namespace RenderLayers
{
    class RenderLayersEditorModule
        : public RenderLayersModuleInterface
    {
    public:
        AZ_RTTI(RenderLayersEditorModule, RenderLayersEditorModuleTypeId, RenderLayersModuleInterface);
        AZ_CLASS_ALLOCATOR(RenderLayersEditorModule, AZ::SystemAllocator);

        RenderLayersEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RenderLayersEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<RenderLayersEditorSystemComponent>(),
            };
        }
    };
}// namespace RenderLayers

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), RenderLayers::RenderLayersEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_RenderLayers_Editor, RenderLayers::RenderLayersEditorModule)
#endif
