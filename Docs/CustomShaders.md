# Custom Shaders (.rshader)

Quick reference for RvelaEngine's `.rshader` files.

---

## Template

```glsl
shader_type spatial;
render_mode cull_back; // unshaded, cull_disabled, depth_draw_disabled, blend_mix, etc.

uniform float speed : hint_range(0.0, 5.0) = 1.0;
uniform vec3 tint : source_color = vec3(1.0);
uniform sampler2D tex : filter_linear_mipmap, repeat_enable;

void vertex() {
    VERTEX.y += sin(TIME * speed + VERTEX.x) * 0.1;
}

void fragment() {
    ALBEDO = texture(tex, UV).rgb * tint;
    ROUGHNESS = 0.5;
    METALLIC = 0.0;
}
```

---

## Render Modes

Declare after `shader_type spatial;`:
```glsl
render_mode unshaded, cull_disabled, blend_add;
```

| Category | Options |
| :--- | :--- |
| **Shading** | `unshaded` / `unlit` *(disables PBR)* |
| **Culling** | `cull_back` *(default)*, `cull_front`, `cull_disabled` |
| **Depth** | `depth_test_disabled`, `depth_draw_disabled`, `depth_draw_always` |
| **Blend** | `blend_mix` *(default)*, `blend_add`, `blend_sub`, `blend_mul` |
| **Raster** | `wireframe` |

---

## Uniforms & Hints

**Syntax:** `uniform <type> <name> [: <hints>] [= <default>];`

### Types
`float`, `int`, `bool`, `vec2`, `vec3`, `vec4`, `sampler2D`

### Hints
- **UI:** `: hint_range(min, max)`, `: source_color`
- **Engine Textures:** `: hint_screen_texture`, `: hint_depth_texture`, `: hint_normal_texture`
- **Sampler Filter:** `filter_linear`, `filter_nearest`, `filter_linear_mipmap`, `filter_nearest_mipmap`
- **Sampler Wrap:** `repeat_enable`, `repeat_disable` / `clamp_to_edge`

---

## Built-ins

### Vertex (`void vertex()`)
| Variable | Type | Description |
| :--- | :--- | :--- |
| `VERTEX` | `vec3` | Object-space position (read/write) |
| `NORMAL` | `vec3` | Vertex normal (read/write) |
| `TANGENT` | `vec3` | Vertex tangent (read/write) |
| `BINORMAL` / `BITANGENT` | `vec3` | Bitangent (read/write) |
| `UV` | `vec2` | UV coordinates (read/write) |
| `COLOR` | `vec4` | Vertex color (read/write) |
| `POSITION` | `vec4` | Clip-space override (`gl_Position`) |

### Fragment (`void fragment()`)
| Outputs | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `ALBEDO` | `vec3` | `vec3(1.0)` | Base color |
| `ALPHA` | `float` | `1.0` | Opacity |
| `ALPHA_SCISSOR`| `float` | `0.0` | Discard threshold if `> 0.0` |
| `METALLIC` | `float` | `0.0` | Metallic factor `[0 - 1]` |
| `ROUGHNESS` | `float` | `1.0` | Roughness `[0 - 1]` |
| `SPECULAR` | `float` | `0.5` | Specular multiplier |
| `EMISSION` | `vec3` | `vec3(0.0)` | Emissive color |
| `AO` | `float` | `1.0` | Ambient occlusion |
| `NORMAL_MAP` | `vec3` | `vec3(0,0,1)`| Tangent-space normal map |

| Inputs | Type | Description |
| :--- | :--- | :--- |
| `VERTEX` | `vec3` | World-space fragment position |
| `NORMAL` | `vec3` | World normal |
| `VIEW` | `vec3` | Direction from fragment to camera (`normalize(CAMERA_POS - VERTEX)`) |
| `TANGENT`, `BITANGENT` | `vec3` | World tangent / bitangent |
| `UV` | `vec2` | Interpolated UV |
| `FRONT_FACING` | `bool` | `gl_FrontFacing` |
| `SCREEN_UV` | `vec2` | Normalized screen coordinate `[0 - 1]` |
| `FRAGCOORD` | `vec4` | `gl_FragCoord` |

### Globals & Textures
- **Environment:** `TIME`, `CAMERA_POS`, `VIEWPORT_SIZE`, `NEAR_PLANE`, `FAR_PLANE`
- **Matrices:** `MODEL_MATRIX`, `VIEW_MATRIX`, `PROJECTION_MATRIX`, `INV_VIEW_MATRIX`, `INV_PROJECTION_MATRIX`
- **Built-in Samplers:** `SCREEN_TEXTURE`, `DEPTH_TEXTURE`, `NORMAL_TEXTURE`, `CONTACT_SHADOW_TEXTURE`
