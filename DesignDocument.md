# Render Layers O3DE Gem Design Document

## Purpose

The Render Layers Gem provides a rendering feature called `Render Layers`: a programmer-facing abstraction for controlling render order without exposing game code directly to low-level render pass complexity.

The goal is to let developers break a scene into ordered rendering steps so that certain content can appear in front of or behind other content regardless of normal 3D occlusion. This makes it possible to create effects commonly seen in games and animation, such as first-person weapon rendering, impact-frame focus effects, foreground character reveals, stylized backdrops, and other intentional violations of normal 3D visibility.

The mental model is inspired by layer-based art tools, but this system is designed for real-time games where ordering must remain valid while gameplay systems modify rendering at runtime.

## MVP Goals

The MVP should focus on a small, safe, programmer-first system.

Primary goals:

* Provide low-level C++ control over render layers, render elements, and render layer sequences.
* Provide reusable render layer definitions for common layer configuration, and have sequence definitions compose them into ordered render layers.
* Preserve the game's existing visual output after integration until the developer intentionally makes desired changes.
* Use pure layer-relative ordering. Developers do not directly edit absolute sequence indices.
* Reject invalid ordering changes instead of silently choosing a fallback order.
* Allow render elements to render in zero, one, or many compatible layers when explicitly configured.
* Provide simple default behavior where each element is explicitly assignable to at most one layer unless the developer opts into more.
* Provide `Basis` fallback as a type of implicit assignment so existing unassigned render elements can continue rendering through a default layer.
* Allow render elements to opt out of implicit assignments.
* Use isolated depth between scene render layers so later layers appear visually in front of earlier layers.
* Keep UI rendering outside of this system for the MVP.

Non-goals for the MVP:

* Full artist/designer tooling.
* UI integration.
* Mod injection support.
* Scoped runtime state handles.
* General-purpose compositor features such as blend modes, masks, and shared depth domains.

These may be added later if the core system proves useful and stable.

## Design Principles

* Valid committed state only: the active render layer sequence must always be valid.
* No tie-breakers: if ordering is ambiguous, the system reports the error instead of choosing an arbitrary order.
* Layer-relative ordering only: ordering intent is expressed relative to other layers.
* Programmer first: expose a clear low-level C++ model before building higher-level editor or gameplay conveniences.
* Simple by default: default usage should protect against unintentional multi-layer element rendering or missing fallback rendering.
* Composition over inheritance: use concepts such as, layer sources, layer behaviors, effects, and policies instead of creating many special layer subclasses.

## Core Terminology

### Render Element

A `Render Element` is any object instantiated from content that can be rendered by this system.

MVP render element categories:

* `Entity Render Element`: an entity object within the scene.
* `Shader Render Element`: a screen-space shader element.
* `Image Render Element`: a screen-space image element.
* `Video Render Element`: a screen-space video element.

UI render elements are excluded from the MVP. O3DE's existing UI rendering remains separate.

Each render element has layer-assignment configuration used to validate assignment edits and resolve the element's effective layer assignments.

Layer-assignment configuration:

* Assignment policies
* Explicit layer assignments
* Whether implicit assignments are allowed

Default layer-assignment configuration:

* Assignment policies: `MaxExplicitAssignments(1)`
* Explicit layer assignments: none
* Allow implicit assignments: true

These defaults preserve the game's rendering before Gem integration by allowing compatible unassigned elements to render through `Basis` fallback.

Every render element starts from these defaults, and overrides are per element. Implementations may store the defaults once and only materialize per-element override data when an element differs from those defaults.

Entity render elements should expose these overrides through a render element configuration component. This component should provide an integrated editor workflow while using the same lower-level assignment API available to non-entity render elements such as shader, image, and video elements.

### Render Layer Source

A `Render Layer Source` defines what kind of content a layer renders and which render element categories are compatible with that layer.

MVP render layer sources:

* `Scene Source`: renders compatible entity render elements.
* `Shader Source`: renders compatible shader render elements.
* `Image Source`: renders compatible image render elements.
* `Video Source`: renders compatible video render elements.

Examples may use the shortened source labels `Scene`, `Shader`, `Image`, and `Video` when showing layer definitions.

A render layer can render multiple compatible render elements. For example, an image layer can render multiple image elements, and a scene layer can render multiple entity elements.

The MVP may begin with simple full-screen behavior for shader, image, and video sources, but the abstraction should not prevent future screen-space layout controls such as position, scale, anchoring, cropping, or masking.

### Render Layer

A `Render Layer` is a named rendering step in a render layer sequence.

A render layer can be a sequence-local use of a render layer definition, an inline created layer within a sequence definition, or dynamically created through code.

A render layer has:

* A sequence-local name unique to the layer's instance.
* A reference to a render layer definition if instanced from one.
* A render layer source.
* One layer behavior.
* Zero or more render layer effects.
* A set of effective element assignments evaluated for rendering.

### Render Layer Definition

A `Render Layer Definition` is a reusable asset or data object that describes the reusable properties of a render layer without defining where that layer appears in a sequence.

A render layer definition has:

* A name.
* A render layer source.
* One layer behavior.
* Zero or more render layer effects and effect parameters.

Render layer definitions do not contain ordering constraints or element assignments. Constraints and ordering must remain the responsibility of the render layer sequence. Assignments remain owned by render elements, or runtime APIs.

Render layer sequence definitions reference render layer definitions and provide sequence-specific data such as ordering constraints. This makes common layer configurations reusable across multiple sequences.

### Render Layer Sequence Definition

A `Render Layer Sequence Definition` is a shareable asset that describes a set of render layers and their relative ordering constraints. The render layers used for the sequence definition are configured via the addition or removal of render layer definition asset references.

The definition is resolved into a flat contiguous array of instanced layers for rendering called a render layer sequence. This resolved array is derived state and is not the authoritative editing model; the sequence definition and runtime API remain the editing model.

A sequence with zero layers renders nothing. A single-layer sequence is valid without ordering constraints. A sequence with multiple layers must include enough immediate ordering constraints to resolve to exactly one valid total order.

### Default Render Layer Sequence Definition

The `Default Render Layer Sequence Definition` is the project-level definition used by cameras that do not specify an override.

The Gem should provide a neutral sequence definition as a starting point so that enabling the Gem does not change the game's appearance. This neutral sequence definition should contain one render layer created from a neutral render layer definition with `Scene Source` and `Basis` behavior so normal unassigned scene content continues to render.

Projects are expected to duplicate or create their own sequence definition assets instead of editing Gem-provided content directly.

### Render Layer Sequence Definition Override

A `Render Layer Sequence Definition Override` is a camera-level reference to a render layer sequence definition used instead of the project default.

Multiple cameras may reference the same render layer sequence definition. Camera-level overrides are not required to duplicate sequence definitions.

### Layer Behavior

A `Layer Behavior` modifies how a render layer participates in assignment or rendering.

MVP layer behaviors:

* `None`: default behavior. The layer only renders elements explicitly assigned to it.
* `Basis`: the layer may render compatible unassigned elements through a type of implicit assignment called `Basis` fallback. This fallback assignment is not explicit.

The MVP treats behavior as a single setting on a layer. Future versions may add more behaviors if needed.

### Render Layer Effect

A `Render Layer Effect` is an optional modification applied to a render layer.

Effects may only be compatible with certain render layer sources.

Initial effect to support:

* `Field of View`: overrides the FOV for a scene render layer. This is important for first-person rendering.

Future versions may allow games to define custom render layer effects.

### Layer Assignment

A `Layer Assignment` is the relationship that causes a render element to be rendered by a render layer.

There are two kinds of layer assignment:

* `Explicit Layer Assignment`: a stored assignment made by the developer, editor, asset, or runtime API.
* `Implicit Layer Assignment`: a computed assignment produced by system behavior, such as `Basis` fallback.

`Effective Layer Assignments` are the assignments used for rendering after combining explicit assignments and implicit assignments.

Each render element has an `allow implicit assignments` setting. This setting is enabled by default so Gem integration remains visually neutral. When disabled, no implicit assignments may occur for that element. In the MVP, implicit assignments only occur through `Basis` fallback as a layer behavior.

### Element Assignment Policy

An `Element Assignment Policy` is a rule attached to a render element that validates explicit layer assignment changes.

Policies are safeguards. They validate whether an explicit layer assignment operation is allowed for that element.

MVP policy:

* `MaxExplicitAssignments(count)`

Default policy for every render element:

* `MaxExplicitAssignments(1)`

This default means render elements can only be explicitly assigned to one compatible layer at a time. If a developer wants an element to have multiple explicit layer assignments, they must intentionally remove or replace the default policy. This default policy keeps common usage simple without sacrificing advanced use cases.

## Layer Assignment Model

Render elements may have zero, one, or many explicit compatible layer assignments. The default policy restricts this to one explicit assignment unless the developer opts into more.

Explicit layer assignment rules:

* An explicit layer assignment is stored state.
* Explicit assignment to an incompatible layer is invalid.
* Assignment policies are checked before assignment state is changed.
* If a policy is violated, the assignment change is rejected.
* Explicit assignment to a layer with `Basis` behavior is allowed because it is still a normal layer.

`Basis` fallback rules:

* `Basis` fallback only applies to elements with zero explicit compatible layer assignments.
* `Allow implicit assignments` should be enabled by default for render elements so Gem integration remains visually neutral.
* If an element has an explicit assignment to any compatible layer, `Basis` fallback is not evaluated for that element.
* If an element has an explicit assignment to a layer with `Basis` behavior, it renders there explicitly and does not also render there through `Basis` fallback.
* If an element has zero explicit compatible layer assignments and implicit assignment is allowed for that element, the system creates an implicit layer assignment to the compatible layer with `Basis` behavior if one exists.
* If an element has zero explicit compatible layer assignments and implicit assignment is not allowed for that element, no implicit `Basis` assignment occurs.
* If an element has zero explicit compatible layer assignments, implicit assignment is allowed, and no compatible layer with `Basis` behavior exists, the element does not render because it has no assigned layer.
* Any kind of implicit assignment (e.g. `Basis` fallback) does not mutate explicit assignment state.

Sequence validity rules for `Basis` behavior:

* A render layer sequence may contain at most one compatible `Basis` behavior layer per render layer source.
* Multiple compatible layers with `Basis` behavior for the same source are invalid.

Example:

```text
Example sequence definition:
SceneBasisLayer ImmediatelyBefore FirstPersonLayer

Layers:
SceneBasisLayer
  Source: Scene
  Behavior: Basis

FirstPersonLayer
  Source: Scene
  Behavior: None

Unassigned entity:
  Explicit layer assignments: none
  Allow implicit assignments: true
  Effective layer assignment: implicit assignment to SceneBasisLayer

First-person weapon:
  Explicit layer assignments: FirstPersonLayer
  Allow implicit assignments: true
  Effective layer assignment: only explicit assignment to FirstPersonLayer
```

## Element Assignment Policy Handling

Policies validate explicit layer assignment changes.

Design-time behavior:

* Invalid assignment edits should be rejected.
* The editor or validation tool should show a clear error explaining which policy was violated.

Runtime development behavior:

* Invalid assignment changes should assert.
* The system should log a detailed error.
* The attempted change should be rejected.
* The previous valid assignment state should remain active.

Runtime release behavior:

* Invalid assignment changes should not crash the game.
* The attempted change should be rejected.
* The system should log or report the failure.
* The previous valid assignment state should remain active.

Future assignment policies may include:

* `AllowedExplicitAssignments(layerIds)`
* `DisallowedExplicitAssignments(layerIds)`
* `RequireExplicitAssignment`

These are quality-of-life safeguards and are not required for the MVP unless they are trivial to add.

## Ordering Model

Render layer sequence ordering is pure layer-relative ordering.

Developers do not directly set absolute layer indices. Instead, they specify constraints between sequence-local render layers. The system resolves those constraints into one flat ordered array for rendering. Reusable render layer definitions do not carry ordering constraints.

Primitive MVP constraints:

* `ImmediatelyBefore(targetLayer)`
* `ImmediatelyAfter(targetLayer)`

Convenience operation:

* `ImmediatelyBetween(previousLayer, nextLayer)`

`ImmediatelyBetween` is not a primitive ordering concept. It is a higher-level convenience that can be implemented by updating the underlying primitive constraints between two adjacent layers.

### Sequence Solver

The `Sequence Solver` is the system responsible for validating a render layer sequence's order constraints and resolving them into the final render layer sequence: a single flat layer order used for rendering. It evaluates candidate sequence changes before they are committed so the active sequence never enters an invalid or ambiguous ordering state.

For a committed render layer sequence with more than one layer, the constraints must resolve into exactly one contiguous chain containing every layer. A layer added to a multi-layer sequence must be connected as part of the same atomic change that introduces it.

Validity requirements:

* Each layer may have at most one immediate predecessor.
* Each layer may have at most one immediate successor.
* The sequence must have exactly one head layer.
* The sequence must have exactly one tail layer.
* Every layer must be connected into the same chain.
* Cycles are invalid.
* Conflicting constraints are invalid.
* Disconnected chains are invalid.
* Any ordering ambiguity is invalid.
* Any solver result requiring a tie-breaker is invalid.

Examples of invalid ordering:

```text
LayerA ImmediatelyAfter Backdrop
LayerB ImmediatelyAfter Backdrop
```

Invalid because two layers claim the same immediate position after `Backdrop`.

```text
LayerA ImmediatelyBefore LayerB
LayerC ImmediatelyBefore LayerD
```

Invalid because this creates two disconnected chains. The system cannot know whether `LayerA -> LayerB` should come before or after `LayerC -> LayerD`.

```text
LayerA ImmediatelyBefore LayerB
LayerB ImmediatelyBefore LayerA
```

Invalid because this creates a cycle.

### Candidate Changes and Committed State

The active committed sequence must always remain valid.

Workflow:

* The developer, editor, or runtime API requests a change.
* The system applies the change to a candidate sequence state.
* The solver validates the candidate.
* If valid, the candidate is committed and the render layer sequence is updated.
* If invalid, the candidate is rejected and the previous valid render layer sequence remains active.

Runtime APIs should support atomic changes so callers can add layers and constraints together without exposing partially invalid intermediate state.

Design-time tools may display invalid candidate states while the user is editing, but invalid states must not become the active committed render layer sequence.

## Depth Model

For the MVP, scene render layers use isolated cross-layer depth.

Rules:

* Within a scene render layer, normal scene depth behavior applies.
* Across scene render layers, depth is isolated.
* A later scene layer is not occluded by depth from an earlier scene layer.
* The color result of a later layer composites over the previous layers according to sequence order.
* Objects that need natural 3D intersection should be placed in the same scene render layer.

This makes render layer order the primary mechanism for cross-layer visibility. It supports first-person rendering and focus/highlight effects without requiring developers to manually fight the previous layer's depth buffer.

Future versions may add configurable depth policies such as shared depth, depth-only layers, mask layers, or separate named depth domains. These are out of scope for the MVP.

## Runtime and Design-Time Operations // TODO: Split into runtime and design-time operations.

The following operations should be available through C++ at runtime. Design-time tooling or assets should use the same model and validation rules.

Render element operations:

* Add explicit layer assignment.
* Remove explicit layer assignment.
* Query explicit layer assignments.
* Query implicit layer assignments.
* Query effective layer assignments.
* Allow or disallow implicit assignments for the element.
* Add or remove assignment policies.

Render layer operations:

* Create render layer.
* Set render layer source at creation time.
* Set name.
* Set layer behavior.
* Add render layer effect.
* Remove render layer effect.
* Configure render layer effect parameters.

Render layer sequence operations:

* Add render layer from layer definition or already instantiated layer.
* Remove render layer.
* Add order constraint.
* Remove order constraint.
* Validate candidate sequence.
* Query resolved sequence order.
* Apply atomic sequence change batch.
* Insert render layer between two adjacent layers.

Camera operations:

* Set render layer sequence definition override.
* Clear render layer sequence definition override.

## Debugging and Validation Tooling

Because ordering is constraint-solved, the Gem should provide strong debugging support.

The debug tool should be available at design time and runtime.

It should show:

* The active resolved sequence order for a specified camera.
* Each layer's definition, source, behavior, effects, and constraints.
* Whether each element is rendered by an explicit or implicit assignment.
* Elements that do not render because they have no effective assignment.
* Invalid candidate ordering errors.
* Cycles.
* Conflicting immediate constraints.
* Disconnected chains.
* Assignment policy violations.
* `Basis` behavior conflicts.

The debug view may display ambiguous or disconnected groups for explanation, but these groups are diagnostic only. They are not part of the committed render layer sequence data model.

## Example Sequences for Certain Use Cases

### Neutral Render Layer Sequence

Sequence Definition:

```text
SceneBasis
```

Layers:

```text
SceneBasis
  Source: Scene
  Behavior: Basis
```

This is the default render layer sequence definition used after the Gem is integrated.

All compatible unassigned scene entities render through implicit `Basis` assignment. The game should look the same as it did before integrating the Gem.

### First-Person Render Layer Sequence

Sequence Definition:

```text
SceneBasis ImmediatelyBefore FirstPerson
```

Layers:

```text
SceneBasis
  Source: Scene
  Behavior: Basis

FirstPerson
  Source: Scene
  Behavior: None
  Effect: Field of View override
```

Typical assignment behavior:

* Normal world entities have no explicit layer assignment and render through `SceneBasis` by `Basis` fallback.
* First-person arms, weapons, or tools are explicitly assigned to `FirstPerson`.
* Because scene layers use isolated depth, the first-person layer renders visually in front of the world and avoids clipping against world geometry.

### Highlight Elements of Interest Render Layer Sequence

Sequence Definition:

```text
SceneBasis ImmediatelyBefore Backdrop
Backdrop ImmediatelyBefore Interesting
Interesting ImmediatelyBefore Foreground
```

Layers:

```text
SceneBasis
  Source: Scene
  Behavior: Basis

Backdrop
  Source: Any source the user may want
  Behavior: None
  
Interesting
  Source: Scene
  Behavior: None

Foreground
  Source: Any source the user may want
  Behavior: None
```

Possible layer roles:

* `SceneBasis`: normal scene content using `Basis` fallback.
* `Backdrop`: elements that should appear behind the interesting layer.
* `Interesting`: entities the game wants to emphasize.
* `Foreground`: optional elements that should appear in front of the interesting layer.

This can support effects such as impact frames, character reveal moments, finishing blows, or cinematic focus effects.

## Game Project Integration

The Gem should integrate without changing the game's appearance until the developer uses it intentionally.

Expected integration behavior:

* The project has a default render layer sequence definition setting.
* The Gem provides a neutral default render layer definition and a neutral default render layer sequence definition that uses it.
* Cameras use the project default unless they reference a sequence definition override.
* Existing scene entities render through `Basis` fallback.
* Existing UI rendering remains outside the render layer sequence and uses O3DE's current UI rendering path.

## Performance Considerations

* Prefer data-oriented design practices to mitigate cache misses where possible.
* Avoid data member bloat. Optimize class memory layouts, utilize structure of arrays rather than array of structures with contiguous memory iteration for prefetcher friendly code.
* Favor value semantics over reference semantics where beneficial. This reduces heap allocations, improves cache locality, and addresses lifetime safety concerns.
* Committed render layer sequences should be pre-resolved into flat runtime arrays so ordering constraints are not resolved during rendering, except when validating or committing runtime changes.
* Keep unnecessary validation logic out of release builds.
* Assignment policies, layer compatibility, and sequence validity should be checked when state changes, not in the render hot path.
* Debug and validation diagnostics should be available, but should not add cost to normal rendering unless enabled.

## Coding Styles

* Negative space programming is encouraged. This keeps our implementation simple and in-check. It also serves as a good example of self documenting code.
* Logging is encouraged where it benefits us. Warnings, errors, and normal status messages help us diagnose scenarios.

## Future Improvements

The following ideas are intentionally outside the MVP but should not be blocked by the design.

### Inline Layer Definitions

Future versions may allow render layer sequence definitions to contain inline layer definitions instead of requiring references to layer definition assets.

An inline layer definition would be owned by a single render layer sequence definition. It would use the same layer configuration model as an asset-backed render layer definition.

The purpose is to avoid unnecessary asset proliferation for one-off layers (the neutral render layer sequence definition provided by the gem could benefit from this).

Inline definitions should resolve exactly like asset-backed definitions. The runtime render layer sequence should not care whether a layer came from an external definition asset or from inline data.

### Injection Points

Game developers may define named injection points that external systems or mods can use to request sequence changes without knowing the full project sequence.

Injection points should remain under game developer control. Mods should not be expected to freely modify arbitrary render ordering unless the game explicitly allows it.

### Shared Depth Domains

Future versions may allow multiple scene layers to share depth behavior so content in different layers can naturally intersect.

This should be explicit. Shared depth should not be used as an automatic ambiguity fallback.

### Batched Operations

Future convenience APIs may allow user-defined grouped operations, such as moving a set of elements to specific layers. This creates a structured way to apply related changes together.

### Scoped Runtime States

Future convenience APIs may allow temporary runtime changes that automatically restore previous state via RAII.

Example future workflow:

```text
ScopedAddExplicitLayerAssignment myScopedLayerAssignment; // Adds on construction, removes on destruction.
```

### Artist and Designer Tooling

Future tools may create a similar experience to Photoshop as an abstracted layer workflow for non-engineers.

### UI Integration

UI is outside the MVP. Future work may define how O3DE UI rendering relates to render layer sequences.

Questions to answer later:

* Can UI render between render layers?
* Can a camera override UI ordering?
* How should debug UI and game UI differ?

### Additional Assignment Policies

Future policies may provide more safety around explicit layer assignment.

Examples:

* Allow assignment only to a specific set of layers.
* Disallow assignment to a specific set of layers.

### Compositing Features

Future versions may support Photoshop-like compositing behavior.

Examples:

* Opacity.
* Blend modes.
* Masks.
* Layer-local post-processing.
* Screen-space layout for image, video, and shader elements.

Some of these features may be implemented as render layer effects.
