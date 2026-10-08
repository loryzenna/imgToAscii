#define _USE_MATH_DEFINES                           // M_PI su Windows (MSVC)
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>                                 // isatty, usleep, write (anche in MinGW)

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <onnxruntime_c_api.h>
#ifdef _WIN32
#include <windows.h>                                // dopo onnxruntime (macro SAL); console: abilita le sequenze ANSI
#endif

// Copertura dei caratteri ASCII 32..126 in Menlo, 2 colonne x 3 righe (indice riga*2+colonna).
// Misurata una volta con CoreText su macOS: stessa forma su ogni sistema.
static const float GLYPH[95][6] = {
    {0.000, 0.000, 0.000, 0.000, 0.000, 0.000},   //  
    {0.105, 0.189, 0.148, 0.274, 0.102, 0.162},   // !
    {0.365, 0.365, 0.154, 0.154, 0.000, 0.000},   // "
    {0.194, 0.305, 0.920, 0.923, 0.228, 0.157},   // #
    {0.233, 0.325, 0.499, 0.623, 0.323, 0.424},   // $
    {0.400, 0.033, 0.749, 0.781, 0.025, 0.378},   // %
    {0.335, 0.175, 0.752, 0.596, 0.348, 0.424},   // &
    {0.164, 0.210, 0.068, 0.087, 0.000, 0.000},   // '
    {0.147, 0.238, 0.550, 0.041, 0.102, 0.226},   // (
    {0.273, 0.113, 0.107, 0.485, 0.254, 0.075},   // )
    {0.029, 0.033, 0.766, 0.793, 0.075, 0.083},   // *
    {0.020, 0.031, 0.484, 0.597, 0.045, 0.070},   // +
    {0.000, 0.000, 0.050, 0.041, 0.454, 0.178},   // ,
    {0.000, 0.000, 0.334, 0.357, 0.000, 0.000},   // -
    {0.000, 0.000, 0.097, 0.057, 0.235, 0.138},   // .
    {0.000, 0.296, 0.307, 0.306, 0.371, 0.000},   // /
    {0.359, 0.379, 0.865, 0.845, 0.292, 0.296},   // 0
    {0.340, 0.230, 0.120, 0.482, 0.243, 0.377},   // 1
    {0.328, 0.364, 0.226, 0.474, 0.400, 0.293},   // 2
    {0.299, 0.360, 0.149, 0.712, 0.338, 0.313},   // 3
    {0.081, 0.396, 0.722, 0.793, 0.000, 0.226},   // 4
    {0.403, 0.231, 0.376, 0.588, 0.337, 0.291},   // 5
    {0.346, 0.264, 0.802, 0.595, 0.292, 0.313},   // 6
    {0.313, 0.450, 0.142, 0.494, 0.228, 0.016},   // 7
    {0.373, 0.376, 0.702, 0.706, 0.324, 0.325},   // 8
    {0.381, 0.356, 0.530, 0.805, 0.265, 0.276},   // 9
    {0.014, 0.010, 0.290, 0.205, 0.196, 0.138},   // :
    {0.014, 0.010, 0.285, 0.207, 0.418, 0.178},   // ;
    {0.000, 0.013, 0.626, 0.621, 0.000, 0.127},   // <
    {0.000, 0.000, 0.686, 0.691, 0.000, 0.000},   // =
    {0.013, 0.000, 0.615, 0.631, 0.126, 0.000},   // >
    {0.260, 0.380, 0.169, 0.419, 0.128, 0.082},   // ?
    {0.240, 0.307, 0.860, 0.773, 0.493, 0.601},   // @
    {0.241, 0.245, 0.737, 0.742, 0.238, 0.239},   // A
    {0.439, 0.372, 0.751, 0.747, 0.372, 0.331},   // B
    {0.326, 0.327, 0.647, 0.000, 0.261, 0.327},   // C
    {0.463, 0.327, 0.609, 0.648, 0.396, 0.253},   // D
    {0.421, 0.308, 0.733, 0.289, 0.355, 0.320},   // E
    {0.399, 0.326, 0.713, 0.286, 0.228, 0.000},   // F
    {0.351, 0.315, 0.637, 0.483, 0.281, 0.362},   // G
    {0.295, 0.295, 0.774, 0.777, 0.228, 0.228},   // H
    {0.345, 0.348, 0.301, 0.307, 0.312, 0.314},   // I
    {0.163, 0.373, 0.002, 0.610, 0.368, 0.245},   // J
    {0.297, 0.370, 0.921, 0.502, 0.228, 0.282},   // K
    {0.294, 0.000, 0.608, 0.000, 0.343, 0.344},   // L
    {0.453, 0.455, 0.927, 0.823, 0.211, 0.212},   // M
    {0.498, 0.306, 1.000, 0.881, 0.239, 0.371},   // N
    {0.375, 0.378, 0.633, 0.632, 0.307, 0.309},   // O
    {0.418, 0.407, 0.731, 0.509, 0.226, 0.000},   // P
    {0.375, 0.378, 0.633, 0.630, 0.306, 0.513},   // Q
    {0.454, 0.359, 0.769, 0.715, 0.227, 0.249},   // R
    {0.371, 0.282, 0.430, 0.519, 0.331, 0.322},   // S
    {0.442, 0.449, 0.296, 0.312, 0.111, 0.117},   // T
    {0.294, 0.293, 0.606, 0.604, 0.309, 0.315},   // U
    {0.306, 0.306, 0.589, 0.590, 0.176, 0.180},   // V
    {0.281, 0.279, 0.968, 0.970, 0.273, 0.273},   // W
    {0.327, 0.325, 0.577, 0.595, 0.258, 0.253},   // X
    {0.324, 0.326, 0.447, 0.451, 0.113, 0.115},   // Y
    {0.288, 0.518, 0.333, 0.347, 0.387, 0.363},   // Z
    {0.118, 0.395, 0.174, 0.385, 0.101, 0.356},   // [
    {0.297, 0.000, 0.382, 0.232, 0.000, 0.370},   // backslash
    {0.299, 0.215, 0.244, 0.315, 0.274, 0.183},   // ]
    {0.299, 0.303, 0.207, 0.206, 0.000, 0.000},   // ^
    {0.000, 0.000, 0.000, 0.000, 0.392, 0.392},   // _
    {0.227, 0.069, 0.000, 0.000, 0.000, 0.000},   // `
    {0.036, 0.027, 0.612, 0.792, 0.337, 0.374},   // a
    {0.326, 0.031, 0.730, 0.671, 0.361, 0.311},   // b
    {0.008, 0.045, 0.630, 0.255, 0.243, 0.298},   // c
    {0.030, 0.328, 0.667, 0.733, 0.307, 0.365},   // d
    {0.018, 0.029, 0.804, 0.637, 0.282, 0.303},   // e
    {0.162, 0.373, 0.495, 0.388, 0.132, 0.077},   // f
    {0.030, 0.027, 0.670, 0.735, 0.531, 0.696},   // g
    {0.323, 0.036, 0.701, 0.649, 0.209, 0.210},   // h
    {0.041, 0.168, 0.209, 0.489, 0.227, 0.384},   // i
    {0.051, 0.168, 0.228, 0.482, 0.334, 0.351},   // j
    {0.322, 0.020, 0.733, 0.618, 0.215, 0.268},   // k
    {0.303, 0.177, 0.245, 0.315, 0.034, 0.346},   // l
    {0.047, 0.034, 0.888, 0.883, 0.282, 0.296},   // m
    {0.025, 0.036, 0.701, 0.649, 0.209, 0.210},   // n
    {0.024, 0.024, 0.662, 0.665, 0.296, 0.299},   // o
    {0.029, 0.030, 0.733, 0.666, 0.655, 0.310},   // p
    {0.025, 0.027, 0.661, 0.741, 0.304, 0.667},   // q
    {0.016, 0.048, 0.614, 0.336, 0.210, 0.000},   // r
    {0.025, 0.036, 0.561, 0.541, 0.271, 0.286},   // s
    {0.240, 0.048, 0.661, 0.238, 0.129, 0.267},   // t
    {0.014, 0.015, 0.560, 0.571, 0.307, 0.361},   // u
    {0.017, 0.017, 0.595, 0.596, 0.181, 0.184},   // v
    {0.015, 0.015, 0.791, 0.792, 0.280, 0.281},   // w
    {0.018, 0.018, 0.594, 0.601, 0.255, 0.255},   // x
    {0.017, 0.017, 0.602, 0.604, 0.498, 0.235},   // y
    {0.033, 0.037, 0.414, 0.555, 0.351, 0.253},   // z
    {0.104, 0.416, 0.483, 0.270, 0.084, 0.383},   // {
    {0.089, 0.213, 0.155, 0.372, 0.149, 0.359},   // |
    {0.380, 0.140, 0.218, 0.535, 0.352, 0.114},   // }
    {0.000, 0.000, 0.521, 0.504, 0.000, 0.000},   // ~
};

// Maschera del soggetto con U²-Netp (u2netp.onnx nella cartella corrente): w*h*nf byte, 1 = soggetto. NULL se fallisce.
// img: nf fotogrammi RGBA w*h uno dopo l'altro (GIF animata). t: soglia tra min (0) e max (1) della previsione;
// più bassa = tiene più pixel come soggetto
static unsigned char *subject_mask(const unsigned char *img, int w, int h, int nf, float t) {
    enum { S = 320 };                               // lato dell'input del modello
    const float mean[3] = {.485f, .456f, .406f}, sd[3] = {.229f, .224f, .225f};
    const OrtApi *ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!ort) { fprintf(stderr, "soggetto: versione di onnxruntime diversa dall'header\n"); return NULL; }
    OrtEnv *env = NULL; OrtSessionOptions *so = NULL; OrtSession *ses = NULL; OrtMemoryInfo *mi = NULL;
    OrtValue *in = NULL, *out = NULL; OrtAllocator *al = NULL; OrtStatus *st;
    char *iname = NULL, *oname = NULL;
    unsigned char *m = malloc((size_t)w * h * nf);
    float *x = malloc(sizeof *x * 3 * S * S), *pred;
#define CK(e) if ((st = (e))) { fprintf(stderr, "soggetto: %s\n", ort->GetErrorMessage(st)); ort->ReleaseStatus(st); free(m); m = NULL; goto end; }

    CK(ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "ascii", &env));
    CK(ort->CreateSessionOptions(&so));
    CK(ort->CreateSession(env, ORT_TSTR("u2netp.onnx"), so, &ses));
    CK(ort->GetAllocatorWithDefaultOptions(&al));
    CK(ort->SessionGetInputName(ses, 0, al, &iname));
    CK(ort->SessionGetOutputName(ses, 0, al, &oname));
    CK(ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &mi));
    const int64_t shape[4] = {1, 3, S, S};
    CK(ort->CreateTensorWithDataAsOrtValue(mi, x, sizeof *x * 3 * S * S, shape, 4, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &in));

    for (int f = 0; f < nf; f++) {                  // il tensore in legge da x: basta riscrivere x a ogni fotogramma
        const unsigned char *fr = img + (size_t)f * w * h * 4;
        float mx = 1;
        // ponytail: ridimensionamento nearest-neighbour in entrambi i versi, bilineare se i bordi del soggetto escono seghettati
        for (int c = 0; c < 3; c++)
            for (int k = 0; k < S * S; k++) {
                x[c * S * S + k] = fr[((long)(k / S * h / S) * w + k % S * w / S) * 4 + c];
                if (x[c * S * S + k] > mx) mx = x[c * S * S + k];
            }
        for (int c = 0; c < 3; c++)
            for (int k = 0; k < S * S; k++) x[c * S * S + k] = (x[c * S * S + k] / mx - mean[c]) / sd[c];

        if (nf > 1) fprintf(stderr, "\rsoggetto: fotogramma %d/%d", f + 1, nf);
        CK(ort->Run(ses, NULL, (const char *const *)&iname, (const OrtValue *const *)&in, 1, (const char *const *)&oname, 1, &out));
        CK(ort->GetTensorMutableData(out, (void **)&pred));

        float lo = pred[0], hi = pred[0];           // il modello non è calibrato: soglia relativa tra min e max
        for (int k = 1; k < S * S; k++) { if (pred[k] < lo) lo = pred[k]; if (pred[k] > hi) hi = pred[k]; }
        unsigned char *mf = m + (size_t)f * w * h;
        for (int y = 0; y < h; y++)
            for (int i = 0; i < w; i++) mf[(long)y * w + i] = pred[y * S / h * S + i * S / w] > lo + (hi - lo) * t;
        ort->ReleaseValue(out); out = NULL;
    }
    if (nf > 1) fputc('\n', stderr);
#undef CK
end:
    if (iname) al->Free(al, iname);
    if (oname) al->Free(al, oname);
    if (out) ort->ReleaseValue(out);
    if (in) ort->ReleaseValue(in);
    if (mi) ort->ReleaseMemoryInfo(mi);
    if (ses) ort->ReleaseSession(ses);
    if (so) ort->ReleaseSessionOptions(so);
    if (env) ort->ReleaseEnv(env);
    free(x);
    return m;
}

// argomento numerico: esce con errore se s non è un numero intero o decimale
static double num(const char *s, const char *nome) {
    char *end;
    double v = strtod(s, &end);
    if (end == s || *end) { fprintf(stderr, "%s: \"%s\" non è un numero\n", nome, s); exit(1); }
    return v;
}

// Ctrl+C durante l'animazione: rimostra il cursore prima di uscire
static void stop(int sig) {
    (void)sig;
    write(1, "\033[?25h\n", 7);
    _exit(0);
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 7) {
        fprintf(stderr, "uso: %s img [stile] [colonne] [soglia bordi / contrasto forme] [soggetto 1/0] [soglia soggetto 0..1]\n"
                        "stili: simboli (predefinito), teschio, forme, bordi\n"
                        "soglia soggetto: predefinita 0.5, più bassa = ritaglio più largo (include vestiti scuri)\n"
                        "GIF animata: riprodotta in loop fino a Ctrl+C (se l'uscita è un file, ogni fotogramma una volta)\n", argv[0]);
        return 1;
    }
    // stile bordi: rampa di luminosità + Sobel; gli altri: forma dei caratteri del set
    const char *stile = argc > 2 ? argv[2] : "simboli";
    const char *set = !strcmp(stile, "simboli") ? " .,:;'`\"^-$"
                    : !strcmp(stile, "teschio") ? " .,:;'`\"^-_|/\\ijdkoLJISP7?4$"
                    : !strcmp(stile, "forme")   ? " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
                    : NULL;
    int bordi = !strcmp(stile, "bordi");
    if (!set && !bordi) { fprintf(stderr, "stile sconosciuto: %s (simboli, teschio, forme, bordi)\n", stile); return 1; }

    // controlli scritti come !(dentro l'intervallo): rifiutano anche nan
    double cols_ = argc > 3 ? num(argv[3], "colonne") : 100;
    if (!(cols_ >= 1 && cols_ <= 100000)) { fprintf(stderr, "colonne: deve essere tra 1 e 100000\n"); return 1; }
    int cols = (int)cols_;
    // bordi: modulo minimo del gradiente per contare come bordo; forme: contrasto, più alto = contorni più netti
    float e = argc > 4 ? num(argv[4], "soglia/contrasto") : bordi ? 100 : 2;
    if (!(e > 0)) { fprintf(stderr, "soglia/contrasto: deve essere maggiore di 0\n"); return 1; }
    double sogg = argc > 5 ? num(argv[5], "soggetto") : 1;
    if (sogg != 0 && sogg != 1) { fprintf(stderr, "soggetto: deve essere 1 (togli lo sfondo) o 0 (immagine intera)\n"); return 1; }
    float ts = argc > 6 ? num(argv[6], "soglia soggetto") : .5f;
    if (!(ts > 0 && ts < 1)) { fprintf(stderr, "soglia soggetto: deve essere tra 0 e 1 esclusi\n"); return 1; }

    // file intero in memoria: stb legge i fotogrammi delle GIF animate solo da memoria
    FILE *fp = fopen(argv[1], "rb");
    if (!fp) { perror(argv[1]); return 1; }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    unsigned char *buf = malloc(len > 0 ? len : 1);
    if (len <= 0 || fread(buf, 1, len, fp) != (size_t)len) { fprintf(stderr, "%s: lettura fallita\n", argv[1]); return 1; }
    fclose(fp);

    // sempre RGBA: trasparente = sfondo. GIF: nf fotogrammi w*h uno dopo l'altro; altro formato: nf = 1
    int w, h, ch, nf = 1, *delays = NULL;
    unsigned char *img = stbi_load_gif_from_memory(buf, (int)len, &delays, &w, &h, &nf, &ch, 4);
    if (!img) { nf = 1; img = stbi_load_from_memory(buf, (int)len, &w, &h, &ch, 4); }
    free(buf);
    if (!img) { fprintf(stderr, "Errore: %s\n", stbi_failure_reason()); return 1; }
    const long fpx = (long)w * h;                   // pixel per fotogramma prima del ritaglio

    // solo il soggetto: lo sfondo diventa trasparente
    unsigned char *m = sogg ? subject_mask(img, w, h, nf, ts) : NULL;
    for (long k = 0; m && k < fpx * nf; k++) if (!m[k]) img[k * 4 + 3] = 0;
    free(m);

    // ritaglia sul riquadro dei pixel opachi, uguale per tutti i fotogrammi: l'animazione non salta
    int x0 = w, y0 = h, x1 = -1, y1 = -1;
    for (int f = 0; f < nf; f++)
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                if (img[(f * fpx + (long)y * w + x) * 4 + 3] >= 128) {
                    if (x < x0) x0 = x;
                    if (x > x1) x1 = x;
                    if (y < y0) y0 = y;
                    if (y > y1) y1 = y;
                }
    if (x1 >= 0) {                                  // tutto trasparente: lascia com'è
        int cw = x1 - x0 + 1, chh = y1 - y0 + 1;   // la destinazione non supera mai la sorgente: memmove in avanti
        for (int f = 0; f < nf; f++)
            for (int j = y0; j <= y1; j++)
                memmove(img + ((long)f * cw * chh + (long)(j - y0) * cw) * 4, img + (f * fpx + (long)j * w + x0) * 4, cw * 4);
        w = cw; h = chh;
    }

    float g[95][6], gmax = 0;                       // forma di ogni carattere di set
    int nset = set ? (int)strlen(set) : 0;
    for (int c = 0; c < nset; c++)
        for (int r = 0; r < 6; r++) { g[c][r] = GLYPH[set[c] - 32][r]; if (g[c][r] > gmax) gmax = g[c][r]; }
    for (int c = 0; c < nset; c++) for (int r = 0; r < 6; r++) g[c][r] /= gmax;   // il più pieno del set arriva a 1
    const char ramp[] = " .:-=+*#%@";               // stile bordi
    const int n = sizeof ramp - 2;                  // indice massimo
    const char edge[] = "|/_\\";                    // bordo per direzione del gradiente: 0°, 45°, 90°, 135°
    int sx = w / cols; if (sx < 2) sx = 2;          // almeno 2 px: servono 2 colonne di campionamento
    int sy = sx * 2;                                // correzione aspetto carattere

    int *Lall = malloc(sizeof *Lall * w * h * nf);  // luminosità per pixel, tutti i fotogrammi
    int lo = 255, hi = 0;                           // luminosità min/max su tutti i fotogrammi: niente sfarfallio
    for (long k = 0; k < (long)w * h * nf; k++) {
        unsigned char *p = img + k * 4;
        Lall[k] = p[3] < 128 ? 0 : (299 * p[0] + 587 * p[1] + 114 * p[2]) / 1000;
        if (p[3] < 128) continue;                   // contrasto calcolato solo sul soggetto
        if (Lall[k] < lo) lo = Lall[k];
        if (Lall[k] > hi) hi = Lall[k];
    }
    if (hi == lo) hi = lo + 1;                      // immagine uniforme: evita divisione per 0
    for (long k = 0; k < (long)w * h * nf; k++)
        if (img[k * 4 + 3] >= 128) Lall[k] = (Lall[k] - lo) * 255 / (hi - lo);  // stira il contrasto su tutta la scala

    // GIF sul terminale: ogni fotogramma riscrive il precedente (cursore in alto a sinistra) fino a Ctrl+C
    int loop = nf > 1 && isatty(1);
    if (loop) {
#ifdef _WIN32
        DWORD cm;
        HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
        if (GetConsoleMode(con, &cm)) SetConsoleMode(con, cm | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
        signal(SIGINT, stop);
        printf("\033[2J\033[?25l");                // pulisce lo schermo, nasconde il cursore
    }
    for (int f = 0; ; f = (f + 1) % nf) {
        int *L = Lall + (long)f * w * h;
        const unsigned char *fr = img + (long)f * w * h * 4;
        if (loop) printf("\033[H");
        for (int y = 0; y + sy <= h; y += sy) {
            for (int x = 0; x + sx <= w; x += sx) {
                if (bordi) {
                    // Sobel su ogni pixel del blocco: ogni pixel di bordo vota una delle 4 direzioni
                    int votes[4] = {0};
                    for (int j = y; j < y + sy; j++) {
                        if (j < 1 || j >= h - 1) continue;
                        for (int i = x; i < x + sx; i++) {
                            if (i < 1 || i >= w - 1) continue;
                            int *c = L + j * w + i;
                            int gx = (c[-w+1] + 2*c[1] + c[w+1]) - (c[-w-1] + 2*c[-1] + c[w-1]);
                            int gy = (c[w-1] + 2*c[w] + c[w+1]) - (c[-w-1] + 2*c[-w] + c[-w+1]);
                            if (gx * gx + gy * gy < e * e) continue;
                            double a = atan2(gy, gx) * 180 / M_PI;  // y verso il basso
                            if (a < 0) a += 180;                     // il verso non conta
                            votes[(int)((a + 22.5) / 45) % 4]++;
                        }
                    }
                    int best = 0;
                    for (int d = 1; d < 4; d++) if (votes[d] > votes[best]) best = d;

                    // ponytail: soglia voti fissa a 1/8 del blocco, rendila un argomento se serve tararla
                    if (votes[best] * 8 >= sx * sy) putchar(edge[best]);
                    else if (fr[((long)y * w + x) * 4 + 3] < 128) putchar(' ');  // sfondo trasparente
                    else putchar(ramp[L[y * w + x] * n / 255]);  // un solo pixel per blocco
                    continue;
                }

                // luminosità media del blocco in 2 colonne x 3 righe, come le forme dei caratteri
                float v[6] = {0}, mx = 0;
                int cnt[6] = {0};
                for (int j = y; j < y + sy; j++)
                    for (int i = x; i < x + sx; i++) {
                        int r = (j - y) * 3 / sy * 2 + (i - x) * 2 / sx;
                        v[r] += L[j * w + i];
                        cnt[r]++;
                    }
                for (int r = 0; r < 6; r++) { v[r] /= cnt[r] * 255.f; if (v[r] > mx) mx = v[r]; }
                // le regioni più scure del blocco scendono verso 0: il carattere segue il contorno
                for (int r = 0; r < 6; r++) if (mx > 0) v[r] = powf(v[r] / mx, e) * mx;

                int best = 0;                           // carattere con la forma più vicina
                float bd = 1e9f;
                for (int c = 0; c < nset; c++) {
                    float d = 0;
                    for (int r = 0; r < 6; r++) d += (v[r] - g[c][r]) * (v[r] - g[c][r]);
                    if (d < bd) { bd = d; best = c; }
                }
                putchar(set[best]);
            }
            putchar('\n');
        }
        if (!loop && nf > 1) putchar('\n');         // nel file: riga vuota tra i fotogrammi
        if (!loop && f == nf - 1) break;            // file o immagine fissa: ogni fotogramma una volta
        if (loop) {
            fflush(stdout);
            int d = delays[f];
            usleep((d < 20 ? 100 : d) * 1000);      // come i browser: ritardi sotto 20 ms valgono 100 ms
        }
    }
    free(Lall);
    free(delays);
    stbi_image_free(img);
    return 0;
}
