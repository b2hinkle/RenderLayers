# Render Layers O3DE Gem Design Document

## Purpose

The Render Layers Gem provides a rendering feature called `render layers`: a programmer-facing abstraction for controlling render order without exposing game code directly to low-level render pass complexity.

The goal is to let developers break a scene into ordered rendering steps so that certain content can appear in front of or behind other content regardless of normal 3D occlusion. This makes it possible to create effects commonly seen in games and animation, such as first-person weapon rendering, impact-frame focus effects, foreground character reveals, stylized backdrops, and other intentional violations of normal 3D visibility.

The mental model is inspired by layer-based art tools, but this system is designed for real-time games where ordering must remain valid while gameplay systems modify rendering at runtime.

## MVP Goals

The MVP should focus on a small, safe, programmer-first system.

Primary goals:

* Provide low-level C++ control over render layers, render elements, and render layer sequences.
* Preserve the game's existing visual output after integration until the developer intentionally makes desired changes through use of render element assignments.
* Use pure layer-relative ordering. Developers do not directly edit absolute sequence indices.
* Reject invalid ordering changes instead of silently choosing a fallback order.
* Allow render elements to render in zero, one, or many compatible layers when explicitly configured.
* Provide simple default behavior where each element is explicitly assignable to at most one layer unless the developer opts into more.
* Provide basis fallback so existing unassigned render elements can continue rendering through a default layer.
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
* Composition over inheritance: use concepts such as layer sources, layer behaviors, effects, and policies instead of creating many special layer subclasses.

## Core Terminology

### Render Element

A `Render Element` is any object instantiated from content that can be rendered by this system.

MVP render element categories:

* `Entity Render Element`: an entity object within the scene.
* `Shader Render Element`: a screen-space shader element.
* `Image Render Element`: a screen-space image element.
* `Video Render Element`: a screen-space video element.

UI render elements are excluded from the MVP. O3DE's existing UI rendering remains separate.

### Render Layer Source

A `Render Layer Source` defines what kind of content a layer renders and which render element categories are compatible with that layer.

MVP render layer sources:

* `Scene Source`: renders compatible entity render elements.
* `Shader Source`: renders compatible shader render elements.
* `Image Source`: renders compatible image render elements.
* `Video Source`: renders compatible video render elements.

A render layer can contain multiple compatible render elements. For example, an image layer can render multiple image elements, and a scene layer can render multiple entity elements.

The MVP may begin with simple fullscreen behavior for shader, image, and video sources, but the abstraction should not prevent future screen-space layout controls such as position, scale, anchoring, cropping, or masking.

### Render Layer

A `Render Layer` is a named rendering step in a render layer sequence.

A render layer has:

* A name.
* A render layer source.
* One layer behavior.
* Zero or more render layer effects.
* A set of effective element assignments evaluated for rendering.

### Render Layer Sequence Definition

A `Render Layer Sequence Definition` is a shareable asset that describes a set of render layers and their relative ordering constraints.

The definition is resolved into a flat contiguous array of layers for rendering called a render layer sequence. This resolved array is derived state, and is not the authoritative editing model (the definition is the editing model).

A sequence with zero layers renders nothing. A sequence with multiple layers must resolve to exactly one valid total order.

### Default Render Layer Sequence Definition

The `Default Render Layer Sequence Definition` is the project-level definition used by cameras that do not specify an override.

The Gem should provide a neutral sequence definition as a starting point so that enabling the Gem does not change the game's appearance. This neutral sequence definition should only contain one scene layer with `Basis` behavior so normal unassigned scene content continues to render.

Projects are expected to duplicate or create their own sequence definition assets instead of editing Gem-provided content directly.

### Render Layer Sequence Definition Override

A `Render Layer Sequence Definition Override` is a camera-level reference to a render layer sequence definition used instead of the project default.

Multiple cameras may reference the same render layer sequence definition. Camera-level overrides are not required to duplicate sequence definitions.

### Layer Behavior

A `Layer Behavior` modifies how a normal render layer participates in element assignment or rendering behavior.

MVP layer behaviors:

* `None`: default behavior. The layer only renders elements explicitly assigned to it.
* `Basis`: the layer may render compatible unassigned elements through basis fallback. This fallback assignment is not an explicit assignment but instead an implicit assignment.

The MVP treats behavior as a single setting on a layer. Future versions may add more behaviors if needed.

### Render Layer Effect

A `Render Layer Effect` is an optional modification applied to a render layer.

Effects may only be compatible with certain render layer sources.

Initial effect to support:

* `Field Of View`: overrides the FOV for a scene render layer. This is important for first-person rendering.

Future versions may allow games to define custom render layer effects.

### Layer Assignment

A `Layer Assignment` is the relationship that causes a render element to be rendered by a render layer.

There are two kinds of layer assignment:

* `Explicit Layer Assignment`: a stored assignment made by the developer, editor, asset, or runtime API.
* `Implicit Layer Assignment`: a computed assignment produced by system behavior (e.g. basis layer fallback).

`Effective Layer Assignments` are the assignments used for rendering after combining explicit assignments and implicit assignments.

### Element Assignment Policy

An `Element Assignment Policy` is a rule attached to a render element that validates explicit layer assignment changes.

Policies are safeguards. They validate whether an explicit layer assignment operation is allowed for that element.

MVP policy:

* `MaxExplicitAssignments(count)`

Default added policy for every render element:

* `MaxExplicitAssignments(1)`

This default means render elements can only be explicitly assigned to one compatible layer at a time. If a developer wants an element to have multiple explicit layer assignments, they must intentionally remove or replace the default policy. This default policy exists to allow the developer to approach the usage of this gem with simplicity without sacrificing advanced usages.

## Layer Assignment Model

Render elements may have zero, one, or many explicit compatible layer assignments. The default policy restricts this to one explicit assignment unless the developer opts into more.

Explicit layer assignment rules:

* An explicit layer assignment is stored state.
* Explicit assignment to an incompatible layer is invalid.
* Assignment policies are checked before assignment state is changed.
* If a policy is violated, the assignment change is rejected.
* Explicit assignment to a layer with `Basis` behavior is allowed because it is still a normal layer.

Basis fallback rules:

* Basis fallback only applies to elements with zero explicit compatible layer assignments.
* If an element has an explicit assignment to any compatible layer, basis fallback is not evaluated for that element.
* If an element has an explicit assignment to the basis-behavior layer, it renders there explicitly and does not also render there through basis fallback.
* If an element has zero explicit compatible layer assignments and basis fallback is enabled for that element, the system creates an implicit layer assignment to the compatible basis-behavior layer if one exists.
* If an element has zero explicit compatible layer assignments and basis fallback is disabled for that element, no implicit basis assignment occurs.
* If an element has zero explicit compatible layer assignments, basis fallback is enabled, and no compatible basis-behavior layer exists, the element simply doesn't render due to not having an assigned layer.
* Basis fallback does not mutate explicit assignment state.

Sequence validity rules for basis behavior:

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
  Basis fallback: enabled
  Effective layer assignment: implicit assignment to SceneBasisLayer

First-person weapon:
  Explicit layer assignments: FirstPersonLayer
  Basis fallback: enabled
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

Developers do not directly set absolute layer indices. Instead, they specify constraints between layers. The system resolves those constraints into one flat ordered array for rendering.

Primitive MVP constraints:

* `ImmediatelyBefore(targetLayer)`
* `ImmediatelyAfter(targetLayer)`

Convenience operation:

* `ImmediatelyBetween(previousLayer, nextLayer)`

`ImmediatelyBetween` is not a primitive ordering concept. It is a higher-level convenience that can be implemented by updating the underlying primitive constraints.

### Sequence Solver

The `Sequence Solver` is the system responsible for validating a render layer sequence's order constraints and resolving them into the final render layer sequence (single flat layer order used for rendering). It evaluates candidate sequence changes before they are committed so the active sequence never enters an invalid or ambiguous ordering state.

For a committed render layer sequence with more than one layer, the constraints must resolve into exactly one contiguous chain containing every layer.

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

### Candidate Changes And Committed State

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

## Runtime And Design-Time Operations

The following operations should be available through C++ at runtime. Design-time tooling or assets should use the same model and validation rules.

Render element operations:

* Add explicit layer assignment.
* Remove explicit layer assignment.
* Query explicit layer assignments.
* Query implicit layer assignments.
* Query effective layer assignments.
* Enable or disable basis fallback for the element.
* Add, remove assignment policies.

Render layer operations:

* Create render layer.
* Remove render layer.
* Set render layer source at creation time.
* Set layer behavior.
* Add render layer effect.
* Remove render layer effect.
* Set render layer effect value.

Render layer sequence operations:

* Add render layer.
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

## Debugging And Validation Tooling

Because ordering is constraint-solved, the Gem should provide strong debugging support.

The debug tool should be available at design time and runtime.

It should show:

* The active resolved sequence order for a specified camera.
* Each layer's source, behavior, effects, and constraints.
* Whether each element is rendered by explicit assignment or implicit.
* Elements that do not render because they have no effective assignment.
* Invalid candidate ordering errors.
* Cycles.
* Conflicting immediate constraints.
* Disconnected chains.
* Assignment policy violations.
* Basis behavior conflicts.

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

This is the default render layer sequence definition that users will notice is being used once the gem is integrated.

All compatible unassigned scene entities render through implicit basis assignment. The game should look the same as it did before integrating the Gem.

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
  Effect: Field Of View override
```

Typical assignment behavior:

* Normal world entities have no explicit layer assignment and render through `SceneBasis` by basis fallback.
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
  Source: Scene
  Behavior: None
  
Interesting
  Source: Scene
  Behavior: None

Foreground
  Source: Scene
  Behavior: None
```

Possible layer roles:

* `SceneBasis`: normal scene content using basis fallback.
* `Backdrop`: shader, image, video, or scene content used to stylize the moment.
* `Interesting`: entities the game wants to emphasize.
* `Foreground`: optional elements that should appear above the interesting layer.

This can support effects such as impact frames, character reveal moments, finishing blows, or cinematic focus effects.

## Game Project Integration

The Gem should integrate without changing the game's appearance until the developer uses it intentionally.

Expected integration behavior:

* The project has a default render layer sequence definition setting.
* The Gem provides a neutral default render layer sequence definition.
* Cameras use the project default unless they reference a sequence definition override.
* Existing scene entities render through basis fallback.
* Existing UI rendering remains outside the render layer sequence and uses O3DE's current UI rendering path.

## Performance Considerations

* We should prefer data-oriented design practices to mitigate cache misses where possible.
* We should favor value-semantics over reference semantics where it benefits us. Less heap allocations, improves cache locality, addresses lifetime safety concerns.
* Committed render layer sequences should be pre-resolved into flat runtime arrays so ordering constraints are not solved during rendering, unless we are required to solve at runtime due to change at runtime.
* Assignment policies, layer compatibility, and sequence validity should be checked when state changes, not in the render hot path.
* Debug and validation diagnostics should be available, but should not add cost to normal rendering unless enabled.

## Future Improvements

The following ideas are intentionally outside the MVP but should not be blocked by the design.

### Injection Points

Game developers may define named injection points that external systems or mods can use to request sequence changes without knowing the full project sequence.

Injection points should remain under game developer control. Mods should not be expected to freely modify arbitrary render ordering unless the game explicitly allows it.

### Shared Depth Domains

Future versions may allow multiple scene layers to share depth behavior so content in different layers can naturally intersect.

This should be explicit. Shared depth should not be used as an automatic ambiguity fallback.

### Batched Operations

Future convenience APIs may allow for user-defined grouped operations, such as moving elements x, y, and z to certain layers. This creates a structured way to apply changes.

### Scoped Runtime States

Future convenience APIs may allow temporary runtime changes that automatically restore previous state via RAII.

Example future workflow:

```text
ScopedAddExplicitLayerAssignment myScopedLayerAssignment; // Adds on construction, removes on destruction.
```

It would also be beneficial to support batched operations.

### Artist And Designer Tooling

Future tools may create a similar experience to photoshop as an abstracted layer workflow for non-engineers.

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

Some of these features may potentially be implemented as render layer effects.
