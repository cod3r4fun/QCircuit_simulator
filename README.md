# Simulatore di Circuiti Quantistici (`qsim`)

**Autore:** Franco Della Negra  

---

## 1. Descrizione Generale
Il progetto consiste nella realizzazione di un simulatore di circuiti quantistici generici operanti su $n$ qubit, sviluppato interamente in linguaggio C. Il simulatore riceve in input la descrizione dello stato iniziale del sistema (vettore di ampiezze) e la sequenza di porte quantistiche (espresse come matrici complesse di dimensione $2^n \times 2^n$).

L'esecuzione si articola in due fasi consecutive:
1. **Evoluzione Unitaria:** Moltiplicazione parallela delle porte quantistiche per calcolare la matrice equivalente del circuito, che viene successivamente applicata allo stato iniziale per determinare lo stato finale.
2. **Misurazione (Opzionale):** Campionamento probabilistico multithread dello stato finale per stimare la distribuzione di probabilità dei risultati (frequentist probability) tramite ripetizioni multiple (shots).

Il software sfrutta la libreria standard **POSIX Threads (`pthread`)** per la parallelizzazione dei calcoli e la **GNU Scientific Library (`GSL`)** per il campionamento efficiente da distribuzioni discrete.

---

## 2. Architettura del Software
Il codice è strutturato in modo modulare per separare nettamente la gestione dei tipi di dato, la logica matematica e il motore di esecuzione multithread:

* **`main.c`**: Gestisce l'entry point del programma, il parsing degli argomenti da linea di comando tramite `getopt` e coordina il flusso sequenziale delle macro-fasi (caricamento file, calcolo evoluzione, campionamento e output).
* **`parser.c / parser.h`**: Dedicato alla lettura e sanificazione dei file di input, all'estrazione del numero di qubit, dello stato iniziale e alla corretta ricostruzione (tokenizzazione) delle porte nel circuito.
* **`engine.c / engine.h`**: Contiene il core multithread del simulatore. Implementa l'albero di riduzione binaria per il prodotto parallelo tra matrici (`mult_engine`) e il partizionamento delle misurazioni tra i worker thread (`meas_engine`).
* **`qmath.c / qmath.h`**: Fornisce le primitive algebriche di supporto per le operazioni complesse, il prodotto matrice-matrice e il prodotto matrice-vettore.
* **`types.c / types.h`**: Definisce le strutture dati opache per la gestione sicura di numeri complessi, vettori (`Vector`) e matrici (`Matrix`) tramite allocazione dinamica della memoria.

---

## 3. Scelte Progettuali e Algoritmi Chiave

### 3.1 Evoluzione Unitaria tramite Albero di Riduzione Binaria
Invece di moltiplicare le porte quantistiche in modo puramente lineare (operazione che lascerebbe la CPU parzialmente inutilizzata e degraderebbe l'efficienza dei thread nel tempo), `mult_engine` adotta un modello ad **albero di riduzione binaria**.

Ad ogni iterazione del ciclo di riduzione, i thread effettuano in parallelo il prodotto di coppie di matrici adiacenti (es. $M_{new} = M_{2k+1} \times M_{2k}$). Questo approccio garantisce due vantaggi fondamentali:
1. **Parallelismo Massimizzato:** Sfrutta appieno il numero di thread richiesti dall'utente riducendo progressivamente il problema a passi logaritmici $O(\log N)$.
2. **Sincronizzazione e Minimizzazione del Drift Numerico:** Riduce significativamente la catena di accumulo degli errori di arrotondamento intrinseci dei numeri a virgola mobile rispetto a una moltiplicazione seriale.

### 3.2 Motore di Misurazione Indipendente e Thread-Safe
Se il circuito prevede una direttiva `measure`, la computazione viene parallelizzata dividendo equamente il numero totale di campionamenti richiesti tra i thread disponibili.

Ogni thread alloca una struttura di pre-elaborazione discreta della GSL (`gsl_ran_discrete_preproc`) basata sul vettore delle probabilità ricavato dalle ampiezze dello stato finale ($P_j = | \alpha_j |^2$). Per garantire l'indipendenza statistica ed evitare campionamenti identici o correlati tra thread diversi, ogni worker thread inizializza il proprio generatore di numeri casuali (`gsl_rng`) usando come seed unico il proprio ID di thread (`pthread_self()`). I risultati parziali locali vengono infine accumulati in un vettore globale condiviso protetto da un mutex (`my_lock`).

---

## 4. Analisi Critica del Formato: Rimozione dei Whitespace e Strategia "Maximal Munch"

Come esplicitamente richiesto dai requisiti di progetto, tutti i caratteri di whitespace (`\t`, `\n`, ` `) presenti nei file di input devono essere ignorati. Per aderire rigidamente a questa specifica, il parser comprime preventivamente l'intero file in un unico flusso continuo di caratteri tramite la funzione `remove_all_whitespace` prima di iniziare l'analisi lexer.

**Limitazione Teorica e Ambiguità dei Token:**
Dal punto di vista della teoria dei linguaggi e dei compilatori, la rimozione *totale* dei whitespace distrugge i delimitatori logici naturali tra i nomi dei gate. Di conseguenza, la grammatica del circuito diventa intrinsecamente ambigua se l'utente definisce nomi di porte parzialmente sovrapponibili.

Ad esempio, se l'utente definisse tre porte distinte chiamate `X`, `Y` e `XY`, le due sequenze di circuito originarie:
1. `#circ X Y`
2. `#circ XY`

Una volta rimosso lo spazio vuoto, produrrebbero lo stesso identico flusso logico per il parser (ovvero `XY`).

**Soluzione Implementata (Maximal Munch):**
Per risolvere questa ambiguità in modo deterministico e conforme ai requisiti, il lexer implementato in `parser.c` adotta una strategia di **Maximal Munch (Longest Match)**. Durante la scansione del circuito, il parser analizza l'intero dizionario delle definizioni e seleziona la porta che non solo combacia con il prefisso corrente, ma che possiede la **lunghezza del nome maggiore** (evitando che un gate chiamato `G10` venga erroneamente scisso in `G1` e `0`).

**Vincolo d'Uso Riconosciuto:**
Dato questo comportamento del parser, si assume como vincolo progettuale che l'utente **non debba definire porte i cui nomi creino collisioni di prefisso immediate** quando usate in sequenza (es. evitare definizioni contemporanee di `X`, `Y` e `XY` all'interno dello stesso file se usate consecutivamente), in modo da preservare la correttezza deterministica e l'integrità della simulazione.

---

## 5. Requisiti e Dipendenze
Per compilare ed eseguire il progetto sono necessari:
* Un compilatore conforme allo standard C (es. `gcc` o `clang`).
* La libreria standard **POSIX Threads (`-pthread`)**.
* La libreria **GNU Scientific Library (`GSL`)** e relativo modulo CBLAS (`-lgsl -lgslcblas -lm`).

Su macOS, le dipendenze della GSL possono essere installate rapidamente tramite Homebrew:
```bash
brew install gsl