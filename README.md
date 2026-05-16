# qsim: High-Performance Multi-Threaded Quantum Circuit Simulator

`qsim` è un simulatore industriale di circuiti quantistici generici ad alte prestazioni per ambienti POSIX-compliant, ingegnerizzato interamente in C standard. Il sistema consente l'evoluzione deterministica di registri quantistici ad $n$-qubit e il campionamento statistico multithread dello stato finale tramite porte quantistiche arbitrarie espresse in forma di matrici complesse dense di dimensione $2^n \times 2^n$.

Progettato seguendo paradigmi di sviluppo software enterprise, `qsim` isola rigorosamente la gestione delle strutture dati opache, l'algebra lineare a bassa latenza e il runtime di orchestrazione concorrente, massimizzando il throughput computazionale su architetture multi-core e minimizzando il drift numerico floating-point.

---

## 1. Architettura del Sistema e Modularità

Il software adotta un'architettura disaccoppiata e modulare, suddivisa nei seguenti componenti funzionali:

* **`main.c` (Control Flow Orchestrator)**: Gestisce il ciclo di vita dell'applicazione, decodifica l'interfaccia a riga di comando (CLI) tramite standard POSIX e coordina le transizioni di stato del simulatore (I/O, allocazione, computazione unitaria, riduzione parallela, campionamento e deallocazione).
* **`engine.c / engine.h` (Concurrency Runtime)**: Contiene il motore di calcolo multithread. Sfrutta thread operanti su un modello ad albero binario per la riduzione delle matrici del circuito e gestisce il campionamento probabilistico isolato e concorrente.
* **`parser.c / parser.h` (Lexical Analysis & Sanitizer)**: Svolge l'analisi lessicale e la compressione degli stream di input, convalidando la sintassi strutturale dei file di configurazione (`.q`) ed eseguendo la tokenizzazione deterministica.
* **`qmath.c / qmath.h` (Linear Algebra Kernel)**: Kernel ottimizzato per l'algebra lineare computazionale su numeri complessi. Fornisce primitive ad alta efficienza per i prodotti matrice-matrice e matrice-vettore.
* **`types.c / types.h` (Data Structures & Memory Isolation)**: Implementa TDA (Tipi di Dato Astratti) opachi per vettori (`Vector`) e matrici (`Matrix`). Garantisce il completo incapsulamento dei dati e previene la frammentazione della memoria tramite interfacce di allocazione e distruzione esplicite.

---

## 2. Ingegneria del Software e Ottimizzazioni Concorrenti

### 2.1 Evoluzione Unitaria tramite Albero di Riduzione Binaria Parallela
Per evitare la serializzazione del calcolo (moltiplicazione lineare sequenziale dei gate), l'engine implementa una riduzione parallela ad **albero binario** nel metodo `mult_engine`. 

I thread riducono lo spazio di ricerca combinando coppie di matrici adiacenti in parallelo ad ogni livello dell'albero (es. $M_{\text{new}} = M_{2k+1} \times M_{2k}$). 
* **Efficienza di Scala:** L'approccio riduce la complessità temporale della composizione del circuito a livello logaritmico $O(\log N)$.
* **Invarianza dell'Errore Numerico:** Riducendo la profondità della catena di moltiplicazioni, si minimizza l'accumulo degli errori di arrotondamento (floating-point drift) tipici dell'aritmetica complessa a precisione finita.

### 2.2 Campionamento Pseudo-Casuale Isolato e Thread-Safe
Nella fase di misurazione statistica (`measure`), `meas_engine` partiziona linearmente il carico di campionamento (shots) tra i core logici allocati. 

Ogni worker thread esegue un setup indipendente sfruttando l'algoritmo discreto pre-elaborato della GNU Scientific Library (`gsl_ran_discrete_preproc`) basato sulla densità di probabilità ricavata dal vettore di stato finale ($P_j = | \alpha_j |^2$).
* **Isolamento degli Stati**: Al fine di eliminare la contesa di lock e garantire la massima entropia statistica senza correlazione tra thread, ogni worker alloca il proprio generatore di numeri casuali (`gsl_rng`) inizializzato dinamicamente usando come seed unico l'indirizzo del descrittore del thread nativo (`pthread_self()`).
* **Consolidamento dei Risultati**: I vettori di occorrenza locali vengono consolidati nel vettore di destinazione globale mediante una sezione critica ultra-ridotta, protetta da un mutex POSIX a bassa contesa (`my_lock`).

---

## 3. Strategia di Parsing e Analisi Lessicale Rigorosa

Il sistema implementa una politica rigorosa di insensibilità agli spazi vuoti. In conformità con le specifiche tecniche, tutti i caratteri di whitespace (`\t`, `\n`, ` `) vengono eliminati a monte tramite `remove_all_whitespace` per uniformare lo stream di input.

### 3.1 Il Problema dei Confini dei Token (Token Boundary Erasure)
La rimozione totale dei whitespace elimina i delimitatori naturali tra i nomi delle porte quantistiche. Di conseguenza, sorge un'ambiguità grammaticale intrinseca se l'utente definisce identificatori che sono l'uno il prefisso dell'altro (es. definire porte distinte chiamate `X`, `Y` e `XY` trasforma sia la sequenza `X Y` che la sequenza `XY` nella stringa complessa continua `XY`).

### 3.2 Soluzione Ingegneristica: Algoritmo Maximal Munch
Per garantire un parsing deterministico e privo di ambiguità senza violare la specifica di compressione, il lexer implementa la strategia **Maximal Munch (Longest Match)**. Durante la scansione sequenziale del circuito, l'analizzatore non si ferma alla prima corrispondenza trovata nel dizionario delle definizioni, ma esamina l'intero set di regole attive per selezionare il token valido che possiede la **lunghezza in caratteri maggiore**. Questo assicura che una stringa contenente il nome del gate `G10` venga interpretata correttamente come tale, e non erroneamente frammentata nei gate `G1` e `0`.

**System Assumption / Vincolo Operativo:**
Si assume come specifica contrattuale che l'utilizzatore finale non introduca nel file di circuito nomi di porte quantistiche che creino collisioni combinatorie di prefisso quando posizionate consecutivamente (es. evitare di usare in sequenza immediata porte chiamate `X` e `Y` se esiste una porta chiamata `XY`), salvaguardando l'integrità matematica della simulazione.

---

## 4. Interfaccia a Riga di Comando (CLI)

L'applicazione espone un'interfaccia CLI standard POSIX basata su flag operativi gestiti tramite l'utility `getopt`:

| Flag | Argomento | Descrizione | Requisito |
| :--- | :--- | :--- | :--- |
| `-s` | `string (path)` | Percorso assoluto o relativo del file `.q` contenente la definizione dello stato quantistico iniziale. | **Obbligatorio** |
| `-c` | `string (path)` | Percorso assoluto o relativo del file `.q` contenente le definizioni delle matrici e il circuito di esecuzione. | **Obbligatorio** |
| `-t` | `int` | Numero di thread POSIX da allocare nel thread pool concorrente. (Default: `1`). | Opzionale |
| `-h` | *Nessuno* | Visualizza la guida in linea con la sintassi dettagliata dei comanzdi e interrompe l'esecuzione. | Opzionale |

---

## 5. Automazione del Build System (Makefile)

Il progetto include un `Makefile` conforme allo standard GNU Make per l'ottimizzazione del processo di compilazione e linking. Il build system applica flag di ottimizzazione aggressivi e controlli di conformità rigorosi del codice (`-O2 -Wall -Wextra -Wpedantic`).

### Target Disponibili

* `make` (o `make all` / `make qsim`): Target principale. Esegue la compilazione incrementale di tutti i file sorgente (`.c`) nei rispettivi file oggetto (`.o`) e genera l'eseguibile binario ottimizzato e collegato staticamente alle librerie esterne.
* `make clean`: Target di manutenzione. Rimuove in modo sicuro tutti i file oggetto intermedi (`main.o`, `engine.o`, `parser.o`, `qmath.o`, `types.o`) e l'eseguibile finale `qsim` per pulire lo spazio di lavoro e forzare una compilazione totale (rebuild).

### Flag di Compilazione Interni
* `-g`: Mantiene i simboli di debug utili per l'analisi tramite l'uso di `lldb` o `gdb`.
* `-O2`: Abilita le ottimizzazioni del compilatore di secondo livello per massimizzare la velocità di esecuzione del codice e l'efficienza dei registri della CPU.
* `-pthread`: Abilita il supporto nativo del compilatore per la libreria POSIX Threads sia in fase di compilazione che di linking.
* `-lgsl -lgslcblas -lm`: Collega l'eseguibile alla GNU Scientific Library, al core BLAS accelerato e alla libreria matematica standard di sistema.

---

## 6. Requisiti di Sistema e Deploy

### Dipendenze Hardware & Software
* Compilatore: `gcc` (v9+) o `clang` (v11+).
* Libreria Concorrente: POSIX `pthreads`.
* Librerie Scientifiche: `GSL (GNU Scientific Library)` e `CBLAS`.

### Installazione Dipendenze su macOS
Le dipendenze per l'architettura host possono essere distribuite velocemente tramite il gestore di pacchetti Homebrew:
```bash
brew install gsl