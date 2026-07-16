# Render Layers O3DE Gem Design Document
This gem will provide a rendering feature called `render layers`, which is an abstraction of render passes designed to hide the complexities of achieving user-defined render order. Renderers (such as Atom) commonly implement a single main pass to render the entire scene. This feature's abstraction allows developers the ability to break it up into multiple steps (`render layers`) to achieve ordering desires at both designtime and runtime.

We should introduce this feature in a very non-opinionated way, allowing developers of any game to integrate it seamlessly with no artifacts. Render layers will allow game developers to achieve certain looks that are normally impossible in a 3D space. Developers can manage their own `render layers`, which introduces the concepts of in-front and behind at the rendering level. The reasoning for doing this at the rendering level is to allow for entities to appear in a specific order regardless of occlusion. The best way to think of this feature is from the perspective of an artist using layering features in art software such as photoshop. They assign content to different layers, giving control over what content is shown in front or behind other content. This gem will provide game developers with this same ability, but for realtime games rather then static images. Having this ability allows developers to break the constraints of the 3d scene to achieve particular looks/effects.

## Terminology
* `Render Element` - Any object instantiated from content which is capable of being rendered, whether in-scene (entity) or out-of-scene. Out-of-scene elements are important, as this allows developers to avoid the complexity and potential error that comes with alignment in a 3D space. A `render element` is of a certain type, and a `render layer` must be compatible with that type via the layer's `render layer type` for it to be placed within that layer. Here is a list of possible render elements types.
  * `Entity Render Element` - An entity object within the scene.
  * `Shader Render Element` - A shader object.
  * `Image Render Element` - An image object.
  * `Video Render Element` - A video object.
  * `UI Render Element` - TODO: Do we want to go this far or do we even want this? May be irrelevant.

* `Render Layer` - A single render layer instance which contributes to a `render layer sequence`. A `render element` can be assigned to a single `render layer`.

* `Render Layer Sequence` - A collection of `render layer`s, which we render in order (index 0 to end). A sequence of no layers will result in nothing being rendered. Every camera gets its own individual `render layer sequence` associated to it, regardless of whether it's of the same layer setup or not.

* `Render Layer Type` - A `render layer` has a certain type. These types are essentially strategies of rendering, providing specialized ways to handle the rendering of the different types of render elements. It is the particular implementation of the `render layer`, which is instanced for use in a `render layer sequence`. The gem itself should provide these, however we should eventually support the game developer being able to write their own types. The following are the types that should be provided by the gem.
  * `Scene Render Layer Type` - A normal render of the game scene (`Entity Render Element`s). This shouldn't look any different from O3DE's default render of the game scene, and should replace it so that developers may use multiple of these as a depth effect. This is the main `render layer type` of the gem.
  * `Shader Render Layer Type` - A fullscreen render of a shader (`shader render element`).
  * `Image Render Layer Type` - A fullscreen render of an image (`image render element`).
  * `Video Render Layer Type` - A fullscreen render of a video (`video render element`).
  * `UI Render Layer Type` - TODO: Do we want to go this far or do we even want this? May be irrelevant.

* `Order Constraint` - One of potentially many rules that makes up the definition of a layer's layer-relative placement within the `render layer sequence`. The `render layer sequence` uses these rules for actively determining its sequence order. For example, a "Backdrop" `render layer` may use `order constraint` "Immediately Before Interesting", where "Interesting" is a different `render layer`. A single `render layer` can be assigned multiple `order constraint`s. In short, constraints exist to keep intent clear and guard against mis-orders.

`Default Render Layer Sequence` - A global project setting specified by the game project, which the gem uses as the `render layer sequence` for cameras that do not have any `render layer sequence override`s associated with them.

`Render Layer Sequence Override` - A `render layer sequence` which maps to a specific camera, for use in place of the `default render layer sequence`.

## Basis Render Layers
Optionally, a `render layer type`'s `render layers` may have one of its layers set as the basis layer. The basis layer has the added capability of existing as a fallback layer for unassigned `render elements` (the elements compatible with the `render layer type`).

## Starting Point `Render Layer Sequence`
After developer integration of the gem, the game should operate and appear identical to before. Appearance should change only when the developer uses the gem's features to achieve that change.

The gem should provide a starting point `render layer sequence` as a neutral and non-intrusive sequence to the game to effectively keep the same look from before integration of the gem. This minimal sequence will probably end up just being a single scene render layer. The gem should assign this starting point sequence as a default value to the `default render layer sequence` global setting. This plays a large role in enabling seamless integration of the gem. If game developers want to build off of this starting sequence, they should make sure not to edit the provided sequence in the gem's content (should make duplicate instead).

## Operations
 The following are common operations that should be possible both at designtime (json/in-editor/asset whatever is most convenient implementation wise) and runtime (C++). Runtime is essential to allow us to create dynamic effects based on gameplay.
* `Render Element`
  * Associate to layer
  * Disassociate from layer
* `Render Layer`
  * `Order constraint` Addition
  * `Order constraint` Removal
* `Render Layer Sequence`
  * `Render layer` Addition
  * `Render layer` Removal

## Ordering Behavior of the `Render Layer Sequence`
 We will not allow for absolute ordering of layers since maintaining this workflow is error-prone. Unlike photoshop software, realtime games are expected to dynamically modify ordering in isolated contexts, which can easily lead to original ordering intentions being broken. The experience of using css z-index feature demonstrates this pain. Layer-relative ordering addresses this pain, as developers no longer need to know every intention of the current sequence order when modifying the order.

 Developers do not modify the absolute order in a `render layer sequence`. Instead, each layer defines their placement relative to others via their set of `order constraint`s. The `render layer sequence` is completely read-only in terms of sequence order. This enforces layer-relative ordering. Layer addition/removal and `order constraint` changes to the `render layer sequence` will trigger a re-evaluation of the `render layer sequence`'s order to maintain correctness. This process involves placing each layer in their relative position based on their `order constraint`s.

### Debug Tool
 Because the `render layer sequence` order is automatically determined based on the layers' `order constraints`, we should provide a debug tool to visualize the current absolute state of the sequence order. This tool should be available at both designtime and runtime for the developer. This visualization will essentially be a dumbed down version of Atom Renderer's pass debug visualization, since it's visualizing an abstraction of some of its passes.
 
### Circular Dependencies
 Due to the nature of relative based ordering, developers may unknowingly create a nonsensical order by mistake (circular dependency). We must make sure we handle these cases by raising awareness to the developer to keep these mistakes easy to track down for them. The code must also handle these cases in a safe manner.

 Circular dependency example - LayerA has `order constraint` "Immediately Before LayerB" and LayerB has `order constraint` "Immediately Before LayerA". 

 Ideally, we make it impossible for developers to make this mistake at design time. Just prevent them from assigning the `order constraint` to a layer that would cause a circular dependency. Circular dependencies created at runtime should probably trigger an assertion as soon as detected and have a safe fallback behavior.

## Render Layer Effects
The gem should provide a set of effects that can optionally be applied to `Render layer`s. The following is a list of effects that come with the gem.
* Field of View - Specifies a FOV for the applied `render layer`

Just as with `render element`s, `render layer effect`s may only be compatible with certain `render layer type`s since effects may be specific to a render strategy.

The gem should also provide a way for developers to create their own `Render layer` effects which they can use for themselves.

### Operations - Available at designtime and runtime
* `Render layer` effect addition
* `Render layer` effect removal
* `Render layer` effect value set

## `Render Element` Assignment
A `render element` can be assigned to a single `render layer`. We should have a way to associate a `render element` with a `render layer`. An element to layer map is my first thought, but we should weigh our options before settling on this. We choose this direction of mapping so that elements themselves decide which layer they are in. This decision should be able to be made at both runtime and at designtime.

### No Assignment
It is possible for an element to not have specified a layer to be assigned to. In this case, there are two possibilities. By default, the element will fall back on the potentially existent basis layer for rendering. If there is no basis layer, or the developer specifies for it to not fall back on the potentially existent basis layer, the element will simply not render.

## Example Usages Commonly Seen In Media
### Highlight Render Element(s) of Interest
This effect essentially shows some `render element`(s) as a backdrop which covers the entire scene temporarily while still keeping the elements(s) of interest in front. This lets us break the constraints of a 3d scene to direct the player's attention to the important elements(s) in that moment.

The `render layer sequence` could be structured with three `render layers`. A basis scene layer for all entities (named "Scene Basis"), a layer (named "Backdrop"), and a scene layer (named "Interesting"). We could also add an optional fourth layer called "Foreground" for `render element`s that should be rendered in front of the interesting layer.

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
What about portal-type rendering effects? What are the implications of this feature being used in combination with a portal effect that combines 2 cameras? It may be fine.

This feature needs to avoid any potential problems with translucent object rendering.

## Performance Considerations
* We should prefer data-oriented design practices to mitigate cache misses where possible.
* We should favor value-semantics over reference semantics where it benefits us. Less heap allocations, improves cache locality, addresses lifetime safety concerns.

## Future Improvements
We could possibly implement blend modes for `render layer`s (e.g. multiply, screen, overlay), similar to Photoshop. Maybe `render layer effects` can be used to implement this? Ideally we implement our solution in a way which can support this for future development.