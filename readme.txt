/*                      SYSTEMY OPERACYJNE 2017/2018                         *
 *                      Projekt 1 - Menadżer pamięci                         *
 *                       Autor: Piotr Maślankowski                           */

README - krótkie informacje o projekcie

Projekt składa się z następujących plików:
    - malloc_internals.h - plik nagłówkowy ze strukturami oraz wewnętrzynymi stałymi używanymi przez malloca i deklaracjami funkcji
    - malloc.c - plik z implementacją wszystkich zadanych funkcji
    - malloc.h - plik nagłówkowy z interfejsem malloca
    - munit.c - plik biblioteki munit 
    - munit.h - plik biblioteki munit
    - unit_tests.c - plik zawierający testy jednostkowe oraz testy funkcjonalne (w sumie 35 testów)
    - readme.txt

Aby zbudować projekt należy użyć po prostu polecenia make. Polecenie make clean powinno wyczyścić wszystkie pliki *.o i *.so .
Aby przestawić menadżer pamięci w tryb ułatwiający debugowanie można ustawić wartości odpowiednich flag na 1 (domyślnie są one równe 0) w pliku malloc.h:
    - flaga MALLOC_DEBUG_SAFE - włącza tryb bezpiecznego debugowania (o tym dalej)
    - flaga MALLOC_DEBUG - włącza tryb zwykłego debugowania
    - flaga OVERRIDE_SIGSEGV_HANDLER - binduje własny handler do sygnału SIGSEGV, który woła funkcję mdump. Bindowanie odbywa się w trakcie inicjalizacji malloc'a 
      (czyli podczas pierwszego wywołania którejkolwiek z funkcji alokatora), co oznacza, że jeśli dany program binduje później swój własny handler do tego sygnału,
      to handler z malloc'a zostanie nadpisany i nie zostanie nigdy użyty. Dlatego opcja ta może z niektórymi programami nie działać. W takim wypadku nie będzie 
      miała ona żadnego widocznego efektu.

Tryby bezpiecznego i zwykłego debugowania mogą być włączone równocześnie, aczkolwiek nie ma to zbyt dużego sensu. Tryby debugowania służą do logowania operacji 
wykonywanych przez menadżer pamięci na stderr. Tryb bezpieczny loguje mniejszą liczbę informacji, ale używa wywołania systemowego write zamiast funkcji fprintf.
Okazuje się, że funkcja fprintf w swojej implementacji używa malloca i niekiedy może to doprowadzić do błędów. Dzieje się tak w sytuacji, w której przesłoniliśmy 
standardowy alokator pamięci przez własny: wywołanie np. malloc'a powoduje zawołanie fprintf, które znowu powoduje zawołanie malloca itd. Może to doprowadzić do błędów
( i tak w moim przypadku rzeczywiście było ). Stąd dodałem tryb bezpiecznego debugowania, który wyświetla mniej szczegółowe dane, jednak nigdy nie doprowadzi do błędów.

Aby przesłonić standardowy alokator pamięci należy ustawić flagę OVERRIDE_STD_MALLOC na 1 w pliku malloc.h. 

Alokator korzysta z algorytmu listowego z polityką first-fit oraz wykorzystuje boundary tagi. Testowałem go z następującymi programami:
    ls, man, vim, xeyes, alsamixer, top, htop, ranger, gnome-control-center, gnome-calculator i kilkoma innymi, których niestety już nie pamiętam.


