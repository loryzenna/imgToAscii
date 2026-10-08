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

```
main img [style] [columns] [edge threshold / shape contrast] [subject 1/0] [subject threshold 0..1]
```

- **style**: `simboli` (symbols, default), `teschio` (skull), `forme` (shapes), `bordi` (edges)
- **columns**: drawing width in characters (default 100)
- **threshold/contrast**: for `bordi` it's the edge threshold (default 100, lower = more edges);
  for the other styles it's the contrast (default 2, higher = sharper edges)
- **subject**: 1 = crop the subject (default), 0 = keep the whole image
- **subject threshold**: mask threshold 0..1 (default 0.5)

Examples:

```
main cat.jpg teschio 80
main cat.jpg bordi 100 100
main cat.jpg simboli 60 3 0
main cat.jpg teschio > drawing.txt
```

Supported image formats: jpg, png, bmp, gif (including animated), tga, psd.

## Build

You need [ONNX Runtime](https://github.com/microsoft/onnxruntime/releases) and the
[`u2netp.onnx`](https://github.com/danielgatis/rembg) model in the project folder.

### macOS

```
brew install onnxruntime
make
```

### Windows (MinGW)

Extract `onnxruntime-win-x64-*.zip` into `./onnxruntime`, then:

```
make
copy onnxruntime\lib\onnxruntime.dll .
```

The resulting binary only needs `onnxruntime.dll` and `u2netp.onnx` in the same folder.

## License

See `onnxruntime/LICENSE` for the ONNX Runtime terms (not included in this repo, must be
downloaded separately).
