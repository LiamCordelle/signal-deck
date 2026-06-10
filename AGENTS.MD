# Signal Deck Agent Notes

Signal Deck is a Pebble Time 2 / Emery watchface. Keep the first-class target
at 200x228 color e-paper unless the user explicitly asks for another platform.

## Project Shape

- Main watchface code: `src/c/main.c`
- Phone/config/weather code: `src/pkjs/index.js`
- Clay config schema: `src/pkjs/config.js`
- PDC icon generator: `tools/make_icons.py`
- Build artifact: `build/signal-deck.pbw`
- Preview screenshots: `screenshots/`

Use `rg` for searches and `apply_patch` for manual source edits.

## Build And Verify

Always build before calling a change done:

```sh
pebble build
```

For visual checks on the Emery emulator:

```sh
pebble install --emulator emery --force --throttle 0.01 build/signal-deck.pbw
pebble screenshot --emulator emery --no-open screenshots/<descriptive-name>.png
```

If the emulator is in a restart loop or refuses installs, ask the user to reset
it rather than doing broad destructive cleanup. After a reset, the install path
usually works again.

## Design Direction

The watchface should feel like a light-mode aviation/dashboard instrument panel:
compact, calm, glanceable, high contrast, and not gadgety for its own sake.
Avoid large decorative flourishes and keep spacing deliberate.

Current main-row direction:

- Main clock: system `FONT_KEY_LECO_42_NUMBERS`
- Temperature: system `FONT_KEY_LECO_20_BOLD_NUMBERS`
- Temperature is number-only in the main weather block; do not draw the tiny
  `C/F` there unless the user asks for it again.
- Current tuned positions in `canvas_update_proc` are intentional:
  `draw_time_display(ctx, GRect(7, 46, 138, 50))`,
  `draw_temperature_display(ctx, GRect(width - 51, 50, 39, 24))`, and
  `draw_weather_pdc(ctx, GPoint(width - 40, 70), s_weather_code)`.

Other font findings from prior visual tests:

- Custom seven-segment time rendered cleanly but felt too techy/gadget-like.
- Pebble Bitham worked nicely for the main time, but the weather temperature
  variants never balanced well.
- `FONT_KEY_BITHAM_34_LIGHT_SUBSET` rendered broken glyphs for this use.
- Gothic was crisp but too small for the main clock.
- Inter Display rasterised surprisingly well, but the `8` in the temperature
  looked odd to the user.
- Smooth custom TTFs get mushy at very small sizes on Pebble. Silkscreen is
  crisp at native grid sizes, especially 8px and 16px.

For the time separator, keep drawing hours/minutes separately with a manually
placed colon. Monospace/custom fonts often give `:` too much cell width.

## Existing Visual System

- Background is warm off-white in light mode and deep navy/black in dark mode.
- Small labels use Silkscreen. Keep date at 16px regular and footer/battery
  labels at 8px bold unless there is a strong reason to change.
- Gauge values use system `FONT_KEY_GOTHIC_18_BOLD` with the value box at
  `GRect(center.x - 21, center.y - 6, 42, 18)`.
- Weather and sun icons are PDC resources in `resources/icons`.
- Footer is one dark bar split into three cells.
- Battery color thresholds are intentional: normal green `0x00aa55`, low amber
  `0xffaa00` below 40%, critical red `0xff0000` below 10%.
- Step gauge text uses decimal `k` formatting below 10,000 steps (`8.6k`) and
  whole `k` formatting at 10,000+ (`10k`) to avoid truncation inside the gauge.

## Configuration And Data

Configuration uses `@rebble/clay` with `enableMultiJS: true` and the
`configurable` capability. Keep Clay storage compatibility in mind when adding
or renaming settings.

Weather comes from Open-Meteo via phone-side JS and uses location permission.
The app requests weather every 30 minutes when weather is enabled. Demo
fallbacks exist so emulator screenshots remain useful when sensors or weather
data are unavailable.

The tick timer should stay on `MINUTE_UNIT` unless seconds are explicitly added.

## Versioning And Release

Use semver in both `package.json` and `package-lock.json`.

- Local design previews do not need a version bump.
- Before publishing a PBW to RePebble, bump at least the patch version.
- The LECO font/layout pass is a patch-level visual change from `1.2.0` and
  is versioned as `1.2.1`.

Do not commit or push design-facing changes until the user explicitly signs off.
When committing, use the user's personal email:

```sh
git config user.email "liamcordelle1234@gmail.com"
```

## Licensing

The project itself is MIT licensed. Bundled third-party font and Clay notices
live in `THIRD_PARTY_LICENSES.md`; update it whenever adding or removing
bundled dependencies. System Pebble fonts such as LECO are referenced by key and
are not bundled resources.
