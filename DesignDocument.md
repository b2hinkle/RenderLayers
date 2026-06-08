# Render Layers Gem Design Document
This gem will provide a rendering feature called `render layers`, which is a rendering abstraction designed to hide the complexities of render passes making up user-defined render order. Renderers (such as Atom) commonly implement a single main pass to render the entire scene. This feature's abstraction allows developers to break it up into multiple steps (`render layers`) to achieve ordering desires at both designtime and runtime.

We should introduce this feature in a very non-opinionated way, allowing developers of any game to integrate it seamlessly with no artifacts. Render layers will allow game developers to achieve certain looks that are normally impossible in a 3D space. Developers can manage their own `render layers`, which introduces the concepts of in-front and behind at the rendering level. The reasoning for doing this at the rendering level is to allow for entities which are occluded by other entities to be able to appear on top if the developer so chooses. The best way to think of this feature is from the perspective of an artist using layering features in art software such as photoshop. They assign content to different layers, giving control over what content is shown in front or behind other content. This gem will provide game developers with this same ability, but for realtime games rather then static images. Having this ability allows developers to break the constraints of the 3d scene to achieve particular looks/effects, useful for both gameplay and cutscenes. 

## Terminology
* `Render Element` - Any object instantiated from content which is capable of being rendered, whether in-scene (entity) or out-of-scene. Out-of-scene elements are important, as this allows developers to avoid the complexity and potential error that comes with alignment in a 3D space.
  * `Entity Render Element` - Entity object within the scene.
  * `Shader Render Element` - Shader object.
  * `Image Render Element` - Image object.
  * `Video Render Element` - Video object.
  * `UI Render Element` - TODO: Do we want to go this far or do we even want this? May be irrelevant.

* `Render Layer` - A single render layer instance which contributes to a `render layer sequence`. `Render element`s can be associated with `render layers`. A single `Render element` can be associated with only one `render layer` at a time.

* `Render Layer Sequence` - A collection of `render layer`s, which we render in order (index 0 to end). A sequence of no layers will result in nothing being rendered. Every camera gets its own individual `render layer sequence` associated to it, regardless of whether it's of the same layer setup or not.

* `Render Layer Type` - Different `render layer`s can have different types. Types exist to allow the developer to choose what to render. This is the particular implementation of the `render layer`, which is instanced for use in a `render layer sequence`. The gem itself should provide these types, however we should eventually support the game developer being able to write their own types. The following are the types that should be provided by them gem.
  * `Scene Render Layer Type` - A normal render of the game scene (`Entity Render Element`s). This shouldn't look any different from O3DE's default render of the game scene, and should replace it so that developers may use multiple of these as a depth effect. This is the main `render layer type` of the gem.
  * `Shader Render Layer Type` - A fullscreen render of a shader (`shader render element`).
  * `Image Render Layer Type` - A fullscreen render of an image (`image render element`).
  * `Video Render Layer Type` - A fullscreen render of a video (`video render element`).
  * `UI Render Layer Type` - TODO: Do we want to go this far or do we even want this? May be irrelevant.

* `Order Constraint` - One of potentially many rules that makes up the definition of a layer's layer-relative placement within the `render layer sequence`. The `render layer sequence` uses these rules for determining its sequence order. For example, a "Backdrop" `render layer` may use `order constraint` "Immediately Before Interesting", where "Interesting" is a different `render layer`. A single `render layer` can be assigned multiple `order constraint`s to keep intent unbreakable.

`Default Render Layer Sequence` - A global project setting specified by the game project, which the gem uses as the `render layer sequence` for cameras that do not have any `render layer sequence override`s associated with them.

`Render Layer Sequence Override` - A `render layer sequence` which maps to a specific camera, for use in place of the `default render layer sequence`.

## Basis Render Layers
Optionally, a `render layer type`'s `render layers` may have one layer set as the basis layer. Basis layers have the added capability of existing as a fallback layer for unassigned `render elements`. Only `render elements` with a (TODO: finish)

## Starting Point `Render Layer Sequence`
After developer integration of the gem, their game's behavior should operate identical to before. The gem should not affect how the game behaves, but rather the developer who decides how to use the gem's features should affect how the game behaves.

The gem should provide a starting point `render layer sequence` as a neutral and non-intrusive sequence to the game to effectively keep the same look before as integration of the gem. This minimal sequence will probably end up just being a single scene render layer. The gem should assign this starting point sequence as a default value to the `default render layer sequence` global setting. This plays a large role in enabling seamless integration of the gem. If game developers want to build off of this starting sequence, they should make sure not to edit the provided sequence in the gem's content (should make duplicate instead).

## Operations
 The following are common operations that should be possible both at designtime (json/in-editor/asset whatever is most convenient implementation wise) and runtime (C++). Runtime is essential to allow us to create dynamic effects based on gameplay.
* `Render Layer`
  * `Order constraint` Addition
  * `Order constraint` Removal
  * `Render Element` Addition TODO: How does elements assignment work
  * `Render Element` Removal
* `Render Layer Sequence`
  * `Render layer` Addition
  * `Render layer` Removal

`Render Layer` dynamic addition/removal to `render layer sequence` may be commonly used for guaranteeing a particular render order of certain `render element`s.

## Ordering Behavior of the `Render Layer Sequence`
 We will not allow for absolute ordering of layers since maintaining this workflow is error-prone. Unlike photoshop software, realtime games are expected to dynamically modify ordering in isolated contexts, which can easily lead to original ordering intentions being broken. The experience of using css z-index feature demonstrates this pain. Layer-relative ordering addresses this pain, as developers no longer need to know every intention of the current sequence order when modifying the order.

 Developers do not modify the absolute order in a `render layer sequence`. Instead, each layer defines their placement relative to others via their set of `order constraint`s. The `render layer sequence` is completely read-only in terms of sequence order. This enforces layer-relative ordering. Layer addition/removal and `order constraint` changes to the `render layer sequence` will trigger a re-evaluation of the `render layer sequence`'s order to maintain correctness. This process involves placing each layer in their relative position based on their `order constraint`s.

### Debug Tool
 Because the `render layer sequence` order is automatically determined based on the layers' `order constraints`, we should provide a debug tool to visualize the current absolute state of the sequence order. This tool should be available at both designtime and runtime for the developer. This visualization will essentially be a dumbed down version of Atom Renderer's pass debug visualization, since it's essentially visualizing an abstraction of some of its passes.
 
### Circular Dependencies
 Due to the nature of relative based ordering, developers may unknowingly create a nonsensical order by mistake (circular dependency). We must make sure we handle these cases by raising awareness to the developer to keep these mistakes easy to track down for them. The code must also handle these cases in a safe manner.

 Circular dependency example - LayerA has `order constraint` "Immediately Before LayerB" and LayerB has `order constraint` "Immediately Before LayerA". 

 Ideally, we make it impossible for developers to make this mistake at design time. Just prevent them from assigning the `order constraint` to a layer that would cause a circular dependency. Circular dependencies created at runtime should probably trigger an assertion as soon as detected and have a safe fallback behavior.

## Render Layer Effects
The gem should provide a set of effects that can optionally be applied to `Render layer`s. The following is a list of effects that come with the gem.
* Field of View - Specifies a FOV for the applied `render layer`

The gem should also provide a way for developers to create their own `Render layer` effects which they can use for themselves.

### Operations - Available at designtime and runtime
* `Render layer` effect addition
* `Render layer` effect removal
* `Render layer` effect value(s) set

## `Render Element` Assignment to Layer
Assignment to `render layer`s should occur at the `render element` level. Elements themselves decide which layer they are in, at runtime and/or at designtime. (TODO: finish) An element is allowed to not specify the layer it's assigned to, and this results in it existing in no layer (not rendered). 



## Example Usages Commonly Seen In Media
### Highlight Render Element(s) of Interest
This effect essentially shows some `render element`(s) as a backdrop which covers the entire scene temporarily while still keeping the elements(s) of interest in front. This lets us break the constraints of a 3d scene to direct the player's attention to the important elements(s) in that moment.

The `render layer sequence` could be structured with three `render layers`. A basis scene layer for all entities (named "Scene Basis"), a scene layer (named "Backdrop"), and a scene layer (named "Interesting"). We could also add an optional fourth layer called "Foreground" for `render element`s that should be rendered in front of the interesting layer.

I would also like to experiment with doing a widescreen effect which uses black bar `render element`s as the "Backdrop" layer, and objects of interest would appear to pop out of the screen.

#### Specific examples
* Super Smash Brothers
  * Character reveal trailers
  * Finishing blow
* Zenless Zone Zero
  * Impact frames
  * Ultimate Animations
* Into the Spider Verse
  * Lots of impact frames, sometimes in slow motion

### First Person Rendering
First-person games implement a common system where first-person objects are rendered on top of the rest of the scene to avoid clipping artifacts (e.g. gun clipping through wall). The `render layer sequence` could simply be structured with two `render layer`s. A basis scene layer for all entities ("Scene Basis"), and a scene layer ("First Person"). The latter would simply contain any first person entities and would be the last to be rendered. We could even use the `render layer` effect "Field of View" to give a unique FOV to the "First Person" `render layer`.

#### Specific examples
* Doom Eternal
* Most first person games that solves weapon clipping at the render level 

## Game Project Integration
This gem's render feature should seamlessly integrate into any game project. I expect we will require some work from the developer's end for integration, but would like to keep it simple. Ideally, it integrates with minimal to no effort without being too intrusive to the game project. We must avoid disrupting the game project's original rendering.

## Edge Cases to Consider
A `render element` should only be assignable to a single `render layer` (TODO: Do we want this? Or maybe we want the ability for certain render elements or layers to be constrained by this idea?). Any other state should be considered invalid, and we must take cautious measures to avoid this. Otherwise, we may run into issues such as double rendering.

What about portal-type rendering effects? What are the implications of this feature being used in combination with a portal effect that combines 2 cameras? It may be fine.

This feature needs to avoid any potential problems with translucent object rendering.


## Performance Considerations


## Future Improvements
We could possibly implement blend modes for `render layer`s (e.g. multiply, screen, overlay), similar to Photoshop. Maybe `render layer effects` can be used to implement this? Ideally we implement our solution in a way which can support this for future development.