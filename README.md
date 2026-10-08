# img-to-ascii

Trasforma una foto (o un GIF animato) in disegno ASCII, ritagliando da solo il soggetto con
una rete U²-Netp (lo sfondo sparisce). Scritto in C, nessuna dipendenza oltre ONNX Runtime.

![demo](cat-dance.gif)

## Uso

```
main img [stile] [colonne] [soglia bordi / contrasto forme] [soggetto 1/0] [soglia soggetto 0..1]
```

- **stile**: `simboli` (predefinito), `teschio`, `forme`, `bordi`
- **colonne**: larghezza del disegno in caratteri (predefinito 100)
- **soglia/contrasto**: per `bordi` è la soglia dei contorni (predefinito 100, più basso = più
  contorni); per gli altri stili è il contrasto (predefinito 2, più alto = contorni più netti)
- **soggetto**: 1 = ritaglia il soggetto (predefinito), 0 = tieni tutta l'immagine
- **soglia soggetto**: soglia 0..1 della maschera (predefinito 0.5)

Esempi:

```
main cat.jpg teschio 80
main cat.jpg bordi 100 100
main cat.jpg simboli 60 3 0
main cat.jpg teschio > disegno.txt
```

Formati di immagine supportati: jpg, png, bmp, gif (anche animati), tga, psd.

## Build

Serve [ONNX Runtime](https://github.com/microsoft/onnxruntime/releases) e il modello
[`u2netp.onnx`](https://github.com/danielgatis/rembg) nella cartella del progetto.

### macOS

```
brew install onnxruntime
make
```

### Windows (MinGW)

Estrai `onnxruntime-win-x64-*.zip` in `./onnxruntime`, poi:

```
make
copy onnxruntime\lib\onnxruntime.dll .
```

Il binario risultante richiede solo `onnxruntime.dll` e `u2netp.onnx` nella stessa cartella.

## Licenza

Vedi `onnxruntime/LICENSE` per i termini di ONNX Runtime (non incluso in questo repo, va
scaricato separatamente).
