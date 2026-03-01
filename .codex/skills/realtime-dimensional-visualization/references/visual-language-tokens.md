Title: Visual Language Tokens for Realtime 2D/3D/4D UI

Use this reference to keep LocusQ visualization surfaces cohesive, modern, and operationally clear.

## Direction
- Build for operator clarity first, then stylistic richness.
- Avoid default UI look-and-feel; choose a distinct art direction per surface.
- Keep motion meaningful and tied to state change, not decorative loops.

## Typography Tokens

| Token | Intent | Suggested Family |
|---|---|---|
| `--font-display` | Hero labels, section titles | `"Space Grotesk", "Avenir Next", sans-serif` |
| `--font-ui` | Controls, status, inline metrics | `"IBM Plex Sans", "Avenir Next", sans-serif` |
| `--font-mono` | Timecode, IDs, numeric diagnostics | `"IBM Plex Mono", "SF Mono", monospace` |
| `--type-scale-1` | Dense metadata | `12px / 16px` |
| `--type-scale-2` | Standard controls | `14px / 20px` |
| `--type-scale-3` | Key state labels | `18px / 24px` |
| `--type-scale-4` | Major dashboard headings | `28px / 34px` |

## Color Tokens (Semantic)

| Token | Meaning | Value |
|---|---|---|
| `--color-bg-0` | Base background | `#06131A` |
| `--color-bg-1` | Panel surface | `#0C202A` |
| `--color-bg-2` | Elevated card | `#14303D` |
| `--color-text-strong` | Primary text | `#F2F7FA` |
| `--color-text-muted` | Secondary text | `#A8BCC8` |
| `--color-state-ok` | Stable/pass | `#2DD08A` |
| `--color-state-warn` | Warning/degraded | `#F7B955` |
| `--color-state-risk` | Critical/error | `#F56C75` |
| `--color-state-focus` | Active selection | `#57C7FF` |
| `--color-density-low` | Low density/energy | `#3A6EA5` |
| `--color-density-mid` | Mid density/energy | `#46A3A7` |
| `--color-density-high` | High density/energy | `#D5E86D` |

## Motion Tokens

| Token | Intent | Value |
|---|---|---|
| `--motion-fast` | Tap/hover feedback | `90ms` |
| `--motion-base` | State transition | `180ms` |
| `--motion-slow` | Layer reveal/history morph | `320ms` |
| `--easing-standard` | Most transitions | `cubic-bezier(0.2, 0.8, 0.2, 1)` |
| `--easing-emphasis` | Focus shifts/camera moves | `cubic-bezier(0.16, 1, 0.3, 1)` |
| `--history-fade-window` | 4D trail decay | `1.2s` |

## Spacing and Layout Tokens

| Token | Intent | Value |
|---|---|---|
| `--space-1` | Micro gap | `4px` |
| `--space-2` | Tight stack | `8px` |
| `--space-3` | Standard stack | `12px` |
| `--space-4` | Section rhythm | `20px` |
| `--radius-sm` | Buttons/inputs | `6px` |
| `--radius-md` | Cards/panels | `12px` |
| `--radius-lg` | Overlay modules | `18px` |

## Data-Vis Mapping Tokens

| Token | Channel | Guidance |
|---|---|---|
| `--viz-pos-x/y/z` | Spatial position | Use for geometry placement only, not category encoding |
| `--viz-size-energy` | Audio/simulation magnitude | Log-scaled normalization to prevent blowout |
| `--viz-color-confidence` | Confidence/quality | Low -> muted, high -> saturated with stable luminance |
| `--viz-alpha-history` | Time decay (4D) | Exponential fade; avoid instant disappear |
| `--viz-motion-jitter-cut` | Smoothing threshold | Deadband small fluctuations to reduce flicker |

## Quality Tiers

| Tier | Intended Runtime | Rules |
|---|---|---|
| `high` | Standalone / strong GPU | Full post effects + dense history trails |
| `balanced` | Most plugin hosts | Reduced trail depth + selective postprocessing |
| `safe` | CPU/GPU constrained hosts | No heavy post, minimal overdraw, capped history |

## CSS Starter Snippet

```css
:root {
  --font-display: "Space Grotesk", "Avenir Next", sans-serif;
  --font-ui: "IBM Plex Sans", "Avenir Next", sans-serif;
  --font-mono: "IBM Plex Mono", "SF Mono", monospace;

  --color-bg-0: #06131a;
  --color-bg-1: #0c202a;
  --color-bg-2: #14303d;
  --color-text-strong: #f2f7fa;
  --color-text-muted: #a8bcc8;
  --color-state-ok: #2dd08a;
  --color-state-warn: #f7b955;
  --color-state-risk: #f56c75;
  --color-state-focus: #57c7ff;
}
```

## Validation Checklist
- Text remains readable at realtime update rates.
- Color semantics remain consistent across 2D, 3D, and time-history views.
- Motion does not hide critical alarms or operator controls.
- `safe` tier preserves meaning while reducing render cost.
