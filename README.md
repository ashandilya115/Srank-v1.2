# Srank Mod Menu

![Srank](logo.png)

A Mega Hack style menu for Geometry Dash (Windows and Android, Geode).

## Opening the menu

- **Windows:** press **Tab** (or the **Hacks** button in the pause menu). Tab again closes it.
- **Android:** tap the Srank icon (top-left of the main menu, bottom-left in levels) or the **Hacks** button in the pause menu.

## Features

**Gameplay:** noclip (player 1 / player 2), noclip accuracy label, die below accuracy %, jump hack, speedhack (+ music pitch), auto pickup coins, frame stepper, safe mode

**Practice:** auto checkpoints, StartPos switcher, custom respawn time

**Visual:** no particles, no screen shake, hide player, hide UI

**HUD:** CPS counter, click indicator, FPS counter, total clicks, session timer, accurate percentage

**General:** FPS change, unlock all icons and colors

Safe Mode (on by default) marks runs as test runs while noclip, speedhack, jump hack or auto coins are active.

## Controls

| Feature | Windows | Android |
|---|---|---|
| Open menu | Tab | Srank icon |
| Frame stepper: advance one frame | `.` key | STEP button |
| StartPos switcher | Left / Right arrows | `<` `>` buttons |

The STEP and `<` `>` buttons appear on screen in levels when those features are on.

## Building

Push this folder to GitHub, open **Actions**, run **Build Mod**, and download **Build Output**. It contains a `.geode` file (one file for both platforms).

- Windows: put it in `%LOCALAPPDATA%\GeometryDash\geode\mods`
- Android: put it in the Geode `mods` folder on your phone, or install it from the Geode mod list.

Both platforms need Geometry Dash 2.2081.

## Notes

- For FPS above your monitor's refresh rate, turn off VSync in GD's video settings.
- Speedhack scales the whole game's clock, menus included.
- Turn Frame Stepper off before leaving a level (the level stays frozen until you step).
- Unlock All Icons is local only; it doesn't change your account.
