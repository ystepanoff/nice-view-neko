# nice!view Neko

A custom [nice!view](https://nicekeyboards.com/nice-view) status screen for
[ZMK](https://zmk.dev), starring the classic **Neko** desktop cat. A ZMK port
of the pet from my [QMK Sofle config](https://github.com/ystepanoff/sofle-rev2-qmk).

## Layout

The central (left) screen keeps the familiar
[nice-view-gem](https://github.com/M165437/nice-view-gem) layout: output
status (USB/BLE) and battery on top, the WPM gauge and chart in the middle,
BLE profile dots and the active layer name at the bottom.

The peripheral (right) screen shows connection and battery status on top —
and the cat's playground at the bottom.

## What the cat does

The pet runs on the peripheral, so it reacts to what that half can observe,
in priority order:

| State | Trigger |
|---|---|
| Caps pose | Caps lock, via HID indicators synced from the central half |
| Chase (north/south) | Rotating the pet's own (right-half) encoder; stops ~2 s after the last tick |
| Sleep | Estimated WPM at or below the low threshold |
| Walk | Estimated WPM between the thresholds |
| Run | Estimated WPM above the high threshold |

Typing speed is estimated from key presses on the pet's own half (scaled ×2
to approximate the whole keyboard) over a 10-second window — real WPM and
keycodes never reach the peripheral in ZMK. For the same reason the ctrl
braced poses, the space-bar jump, and east/west chasing are dormant in this
layout; the frames and hooks remain in `widgets/pet.c` should ZMK ever sync
more state across the split.

## Usage

Add this module to `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: ystepanoff
      url-base: https://github.com/ystepanoff
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: nice-view-neko
      remote: ystepanoff
      revision: main
  self:
    path: config
```

Use the shield in `build.yaml` after the nice!view adapter:

```yaml
include:
  - board: nice_nano//zmk
    shield: sofle_left nice_view_adapter nice_view_neko
  - board: nice_nano//zmk
    shield: sofle_right nice_view_adapter nice_view_neko
```

Caps-lock detection needs `CONFIG_ZMK_HID_INDICATORS=y` and
`CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS=y` — both implied by the shield
on both halves by default.

## Options

| Kconfig | Default | Meaning |
|---|---|---|
| `CONFIG_NICE_VIEW_NEKO_FRAME_MS` | 200 | Animation frame duration |
| `CONFIG_NICE_VIEW_NEKO_WPM_LOW` | 10 | Idle at or below this (estimated) WPM |
| `CONFIG_NICE_VIEW_NEKO_WPM_HIGH` | 40 | Run above this (estimated) WPM |
| `CONFIG_NICE_VIEW_NEKO_SCROLL_TIMEOUT_MS` | 2000 | Chase linger after encoder ticks |
| `CONFIG_NICE_VIEW_NEKO_WPM_FIXED_RANGE` | y | Fixed range for the central WPM gauge/chart |
| `CONFIG_NICE_VIEW_NEKO_WPM_FIXED_RANGE_MAX` | 100 | Maximum of that fixed range |
| `CONFIG_NICE_VIEW_WIDGET_INVERTED` | n | Invert display colors |

## Regenerating the art

Frames are generated from the QMK pet source (SSD1306 page format) into
LVGL 9 `I1` images by `tools/convert_qmk_pet.py`:

```sh
python3 tools/convert_qmk_pet.py path/to/qmk/pets/neko.c \
    boards/shields/nice_view_neko/assets
```

Use `--preview <array>` to eyeball frames as ASCII art before committing.

## Credits

- Screen plumbing and status widgets based on
  [nice-view-gem](https://github.com/M165437/nice-view-gem) (MIT, © Michael
  Schmidt) and the ZMK nice!view shield (MIT, © The ZMK Contributors).
- [Pixel Operator](https://www.dafont.com/pixel-operator.font) font (CC0).
- Neko sprites trace back to the classic
  [oneko](https://en.wikipedia.org/wiki/Neko_(software)) desktop pet.
