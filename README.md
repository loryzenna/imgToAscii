# img-to-ascii

Turns a photo (or an animated GIF) into ASCII art, automatically cropping out the subject with
a U²-Netp network (the background disappears). Written in C, no dependency besides ONNX Runtime.

```
                                  %%%%%
                         ###%%%%%%%%%%%%%%%%%%%
                    %###%#%%#####%%%%%%%%%%%%%%%#%
                ###########**####%%%%%%%#####%%#%#%#%
              ##########%###%######%%%%#########%####%
            **######***##*#*########%%%%%##%##%%%%%%%%%%
          ****#***********#########%%%%%%#%##%#%%%%**##%%
        ##**=*#**********#######%%##%%%%%%%%%***%#%%#####
      #****++++****+*****###%#%%#%%%#+**####***#**#*.:==-+%
    #***++==**#*++**+++-+*%%%%%-:::..:=-*****=*%+*+:-:-::..:%
   ****++==++**+-*+*++==-*#%#:::::..::---*+*==+**%%#--=:-::--#%
  *+++++++=++*=+*=#*****+*%::-:-:-===-:::--++---*  .+*+=+==-+#%
 +++++++=+=++++=*+++++==+#-----==-++++-:::-++==+ . :.+*+=+++*#%
 +++++++==++**+*+++==-:=+##--.:::===+=----:++++= . ...#+-==+**%
*+++++++++++++****+*--:-+*#%-==:-: .--=-=+***+#  .....=#%##*++%%
*+++=+==++++*******++=.:-*#**%*==.  :=*%###%###  . .  :#*****+++*
*++=++==++**+**+***++=-::**###%##%%##*%##**#**#:   ....#*==*+***+%
 +++==+=+==+-+*+****++--++***##**=+*+*#+=-=**##= .:==-==*****#***#
 +++++++++********++==*+#+=+*#%*=#######*=+*++*%*::+==##%+*+++****
 ***+=++*##**+++=-++++=+=-=++=#*#%######%##+*+=*#%##*######+%%%
  ***+=+****+++=.=+==++....-=**##++-++*####*##*#%%%%%%%%%%##%
   ***+=++*++===--*==-: . +**       .:+*=*+#*#%*#%%#%*%####%##%
    ***=+++++-:  :**  ..  =+**       :-=****+=**+##%#--+*.+%:*#%
     *++-*+*+=.    *=#+. .*++#-      :-=+*++=#=- *##:%#+%%=%%#+%%
     #++=**++**:-...****+==+++%       ===:.- # ##**#=#%*%%%#%#*%
      %***++***+---.:+**++*+***+..  .. ##.=#.+=*=%*-##-#++*=*#%
        ****#***++-.. +*++++++*+*  .   ++*:*%=+=:*+-*:=**#*+=-%
          =+****+*.:.  ++++=++***#--=:-.-..:-.=*=+#+*###%#####%
           #=---=+-::...+++++***+++*+*+******#*+**##*##%%%###%%
              =--:::... ++=+*********##*****##+*###%#%%#%%####%
                 =---::::==++******++**######++***###%%%%######%
                      ::::=+++*****+*+*#***#####%##%%#%#%######%
                         --=+*+*****+**#####*#****%#%##%##*++*#*%
                             #*=-:::-+********#%*#*#****+##****#

```

## Usage

``` shell
main [-c] [-f] img [style] [columns] [edge threshold / shape contrast] [subject 1/0] [subject threshold 0..1]
```

- **-c**: color each character with the average color of the image under it (24-bit ANSI, any position)
- **-f**: fill the terminal (width and height, keeping proportions, centered) and follow window resizes; ignores columns. When output is not a terminal it falls back to columns
- **style**: `symbols` (default), `skull`, `shapes`, `edges`
- **columns**: drawing width in characters (default 100)
- **threshold/contrast**: for `edges` it's the edge threshold (default 100, lower = more edges);
  for the other styles it's the contrast (default 2, higher = sharper edges)
- **subject**: 1 = crop the subject (default), 0 = keep the whole image
- **subject threshold**: mask threshold 0..1 (default 0.5)

Examples:

``` shell
main cat.jpg skull 80
main cat.jpg edges 100 100
main cat.jpg symbols 60 3 0
main cat.jpg skull > drawing.txt
main -c cat.jpg shapes 80
main -c -f earth.gif
```

Supported formats: jpg, png, bmp, gif (including animated), tga, psd built in; anything else ffmpeg can decode
(mp4, webm, mov, webp, ...) if `ffmpeg` is in the PATH. Videos play at 15 fps, decoded at most 480 px wide,
and are kept entirely in memory (about 1 MB per frame), so keep them short.

## Code layout

| File | What it does |
|------|--------------|
| `main.c` | arguments and the pipeline: load → subject → crop → luminance → render, plus the playback loop |
| `load.c` | decodes the input: images and GIFs with stb_image, everything else through ffmpeg |
| `subject.c` | U²-Netp subject mask (the only file that uses ONNX Runtime) |
| `image.c` | pixel operations: crop to the subject, luminance with contrast stretch, resampling for `-f` |
| `render.c` | picks a character per block (shape matching or Sobel edges) and prints it, with optional color |
| `glyph.h` | measured shape of each ASCII character, used by `render.c` |
| `term.c` | terminal size, ANSI setup on Windows, Ctrl+C cleanup |

## Download

Prebuilt zips for Linux (x64), macOS (Apple Silicon) and Windows (x64) are in
[Releases](https://github.com/loryzenna/imgToAscii/releases): unzip and run `main` from
inside the folder (it looks for `u2netp.onnx` in the current directory).
On macOS, the first time: `xattr -dr com.apple.quarantine img-to-ascii`.

## Dependencies

You need [ONNX Runtime](https://github.com/microsoft/onnxruntime/releases) and the
[`u2netp.onnx`](https://github.com/danielgatis/rembg) model in the project folder.

## Build

### Linux

``` bash
sudo apt install libonnxruntime-dev   # Debian/Ubuntu
make
```

### macOS

``` bash
brew install onnxruntime
make
```

### Windows (MinGW)

Extract `onnxruntime-win-x64-*.zip` into `./onnxruntime`, then:

``` powershell
make
copy onnxruntime\lib\onnxruntime.dll .
```

The resulting binary only needs `onnxruntime.dll` and `u2netp.onnx` in the same folder.

## License

See `onnxruntime/LICENSE` for the ONNX Runtime terms (not included in this repo, must be
downloaded separately).
