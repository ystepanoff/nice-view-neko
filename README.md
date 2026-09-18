# nice!view Neko

A custom [nice!view](https://nicekeyboards.com/nice-view) status screen for
[ZMK](https://zmk.dev), starring the classic **Neko** desktop cat. A ZMK port
of the pet from my [QMK Sofle config](https://github.com/ystepanoff/sofle-rev2-qmk).

## Layout

The central (left) screen is the cat's playground on top, with output status
(USB/BLE) and battery in the middle, and BLE profile dots plus the active
layer name at the bottom.

The peripheral (right) screen shows connection and battery status on top —
and a napping twin of the cat at the bottom.

## What the cat does

The pet runs on the central half, where everything it reacts to is directly
observable — no state crosses the split. In priority order:

| State | Trigger |
|---|---|
| Caps pose | Caps lock is on (host HID indicator) |
| Braced pose (N/E/S/W) | Ctrl is held; direction is remembered from the last encoder turn |
| Chase (8 directions) | Encoder rotation: left encoder = east/west, right = north/south, both = diagonals; stops ~2 s after the last tick |
| Sleep | WPM at or below the low threshold |
| Walk | WPM between the thresholds |
| Run | WPM above the high threshold |
| Jump | Space pressed while typing (WPM above the low threshold) |

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

Caps-lock detection needs `CONFIG_ZMK_HID_INDICATORS=y`, implied by the
shield on the central half by default.

## Options

| Kconfig | Default | Meaning |
|---|---|---|
| `CONFIG_NICE_VIEW_NEKO_FRAME_MS` | 200 | Animation frame duration |
| `CONFIG_NICE_VIEW_NEKO_WPM_LOW` | 10 | Idle at or below this (estimated) WPM |
| `CONFIG_NICE_VIEW_NEKO_WPM_HIGH` | 40 | Run above this (estimated) WPM |
| `CONFIG_NICE_VIEW_NEKO_SCROLL_TIMEOUT_MS` | 2000 | Chase linger after encoder ticks |
| `CONFIG_NICE_VIEW_WIDGET_INVERTED` | n | Invert display colors |

## Regenerating the art

Frames are generated from the QMK pet source (SSD1306 page format) into
LVGL 9 `I1` images by `tools/convert_qmk_pet.py`:

```sh
python3 tools/convert_qmk_pet.py path/to/qmk/pets/neko.c \
    boards/shields/nice_view_neko/assets --scale 1.6
```

Use `--preview <array>` to eyeball frames as ASCII art before committing.

## Credits

- Screen plumbing and status widgets based on
  [nice-view-gem](https://github.com/M165437/nice-view-gem) (MIT, © Michael
  Schmidt) and the ZMK nice!view shield (MIT, © The ZMK Contributors).
- [Pixel Operator](https://www.dafont.com/pixel-operator.font) font (CC0).
- Neko sprites trace back to the classic
  [oneko](https://en.wikipedia.org/wiki/Neko_(software)) desktop pet.
