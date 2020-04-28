# Równoległa analiza logów

Program do analizy logów serwera www napisany z wykorzystaniem standardu MPI.

Celem zadania jest implementacja wzorca _MapReduce_ z zastosowaniem równoległego mapowania realizowanego przez wiele procesów.

## Działanie programu

Program jest podzielony na 3 główne części.

### Wczytanie danych

Proces główny czyta plik z danymi linia po linii. Każdy wiersz jest parsowany.
Wybrane przez użytkownika pole jest zapisywane do wektora.  
Na tym etapie następuje generalizacja danych polegająca na przejściu z wybranego pola logów do ogólnego typu realizowanego przez ciąg znaków. Dzięki temu bardzo **łatwo można zmodyfikować** dostępne pola lub strukturę pliku bez ingerencji w pozostały kod.

### Mapowanie

Po wczytaniu dane są możliwie równo dzielone i rozsyłane pomiędzy procesami. W tym celu wykorzystuję utworzenie typu MPI `MPI_Type_contiguous()` oraz `MPI_Scatterv()`.

Każdy proces tworzy mapę zliczającą liczę wystąpień danego ciągu znaków (czyli konkretnej wartości w logach).

### Redukcja

Mapy utworzone przez procesy w poprzednim kroku są spłaszczane do tablic i przesyłane do procesu głównego.  
W tym celu wykorzystuję `MPI_Type_create_struct()` oraz `MPI_Gatherv()`.

Następnie proces główny łączy mapy stworzone przez poszczególne procesy w jedną wspólną mapę.
Wyniki są potem sortowane po liczbie wystąpień i wypisywane na ekran.

## Uruchomienie programu

Program będący zadania znajduje się w katalogu [solution](./solution).

Program korzysta z biblioteki MPI. Trzeba ją **zainstalować** przed kompilacją i uruchomieniem.

### Kompilacja

Do kompilacji programu można wykorzystać narzędzie makefile.

Przykład kompilacji

```bash
make
```

Alternatywnie można użyć celu _debug_ by włączyć dodatkowe informacje o działaniu programu.

```bash
make debug
```

### Uruchomienie

Program przyjmuje dwa argumenty - wybranie pole oraz nazwę pliku z logami.

```txt
Program expects 2 arguments: <field_selection> <log_file_name>
Possible fields:
addr - ip addres
time - time with minutes precision
metod - HTTP method
url - request url
protocol - protocol version
stat - request status
browser - browser fingerprint
```

Przykładowe uruchomienie:

```txt
➜ mpirun -n 4 ./log-analizer stat fragment.log
403 -> 2080
200 -> 356
304 -> 116
302 -> 6
206 -> 4
404 -> 1
301 -> 1
```
