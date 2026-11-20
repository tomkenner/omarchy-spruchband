# Spruchband for Omarchy

Every few minutes a figure pulls a saying across your screens: a biplane with
a tow banner, a stork with bunting, a group of walkers with a placard, an angel
with a scroll. With an external monitor attached, it flies from one screen
across to the other.

The plugin adds a little plane to the bar:

| On the plane | Does |
|---|---|
| Click | Panel: what it is doing, on/off, show now, pause, settings |
| Right-click | Send a banner across the screens now |
| Middle-click | Pause for an hour, or resume |

Figures, styles, sayings, timing and size are set in the settings window
(Settings in the panel). Nothing flies while the screen is locked or off, while
a video or game runs full screen, during Do Not Disturb or quiet hours; a banner
that falls due meanwhile follows 15 seconds later.

For key bindings: `spruchband now`, `spruchband toggle`, `spruchband status`.

## How it works

`Service.qml` runs `bin/spruchband daemon --stdio` with the shell and talks to it
over stdin and stdout. The program draws one transparent, click-through
layer-shell strip per screen, so the drawing never runs inside the shell.
Settings live in `~/.config/spruchband.conf`, sayings in
`~/.local/share/spruchband/collections/` (one per line, first line `# Name`).

## Install

```sh
omarchy plugin add https://github.com/tomkenner/omarchy-spruchband.git --enable
```

The plane goes into the bar's right section. For key bindings, put the
program on your path:

```sh
ln -sfn ~/.config/omarchy/plugins/tkenner.spruchband/bin/spruchband ~/.local/bin/spruchband
```

Update with `omarchy plugin update tkenner.spruchband`.

## Remove

```sh
omarchy plugin remove tkenner.spruchband
rm -f ~/.local/bin/spruchband                                 # if you linked it
rm -rf ~/.config/spruchband.conf ~/.local/share/spruchband    # your settings and sayings
```

Spruchband changes nothing else on the system and does not touch your Omarchy
or Hyprland configuration.

## Requirements

`bin/spruchband` is an x86_64 program linked against the system's Qt 6.
It needs `qt6-base`, `qt6-declarative` and `layer-shell-qt`, which an Omarchy
install normally already has; if banners never appear, run
`omarchy pkg add layer-shell-qt`.

## What the program touches

- **Network:** none. It opens no network connections; Qt Network is linked
  only for local sockets.
- **Its own files:** reads and writes `~/.config/spruchband.conf` and
  `~/.local/share/spruchband/collections/`.
- **Omarchy:** reads the current theme (`~/.local/state/omarchy/current/`), the
  language (`~/.config/omarchy/language` and
  `$OMARCHY_PATH/default/i18n/locales.json`) and the notification quiet state
  (`~/.local/state/omarchy/notifications/quiet.json`; on older Omarchy versions
  it runs `makoctl mode` instead).
- **Hyprland:** asks Hyprland's sockets in `$XDG_RUNTIME_DIR/hypr/` for the
  monitors and windows (full screen, locked, screens off) and runs
  `hyprctl -j getoption decoration:rounding` for the corner rounding.
- **Local sockets:** listens on `$XDG_RUNTIME_DIR/spruchband.sock` for
  `spruchband now|pause|resume|toggle|status` and on `spruchband-ui.sock` so only one
  settings window opens.
- **Desktop:** only when you ask for it in the settings window: to import
  sayings it opens the file chooser through xdg-desktop-portal (D-Bus) and
  reads the file you pick; "open folder" shows the sayings folder in your file
  manager.

## Source and build

The program's source is in this repository: `src/` (C++, and the QML of the
settings window), `tests/` and `spruchband.pro`. `bin/spruchband` is not built
on a developer machine. The workflow in `.github/workflows/build.yml` builds it
from `src/` in an Arch Linux container that takes Qt from Omarchy's package
mirror, runs the tests, records a signed build provenance attestation and
commits the binary on its own as `Build bin/spruchband from <source commit>`.
To check that your copy came from that workflow:

```sh
gh attestation verify bin/spruchband --repo tomkenner/omarchy-spruchband
```

To build it yourself (needs `base-devel`, `qt6-base`, `qt6-declarative` and
`layer-shell-qt`):

```sh
mkdir -p build && cd build
qmake6 ../spruchband.pro && make -j"$(nproc)"
install -m 755 -s spruchband ../bin/spruchband
```

## License

Spruchband is MIT licensed, see `LICENSE`. The program bundles these fonts from `src/fonts/` (see `NOTICE`)
under their own licenses:

| Font | Author | License |
|---|---|---|
| Fredoka | The Fredoka Project Authors | SIL OFL 1.1 |
| Caveat | The Caveat Project Authors | SIL OFL 1.1 |
| Tilt Neon | The Tilt Project Authors | SIL OFL 1.1 |
| Press Start 2P | The Press Start 2P Project Authors (Reserved Font Name "Press Start 2P") | SIL OFL 1.1 |
| Cormorant Garamond | The Cormorant Project Authors | SIL OFL 1.1 |
| IM FELL English | Igino Marini | SIL OFL 1.1 |
| Bangers | The Bangers Project Authors | SIL OFL 1.1 |
| Permanent Marker | Font Diner, Inc. | Apache License 2.0 |

OFL: https://openfontlicense.org · Apache 2.0: https://www.apache.org/licenses/LICENSE-2.0
