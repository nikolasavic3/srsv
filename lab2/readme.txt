LAB2. Periodicki rasporedjivac zadataka
=====================================================

KOMPAJLIRANJE:
--------------
gcc -pthread lab2.c -o lab2

POKRETANJE:
-----------
./lab2

Opis:
------
Program simulira periodicni rasporedjivac zadataka koji obradjuje 38 ulaza s razlicitim periodama:
- 3 ulaza s periodom 1000ms (svake sekunde)
- 15 ulaza s periodom 5000ms (svakih 5 sekundi)
- 20 ulaza s periodom 20000ms (svakih 20 sekundi)

Prosirenje zadatka:
-------------------
- Zadatak se moze produziti na dodatnu periodu ako nedavno nije bilo prosirenja.
- Zadatak se moze prekinuti ako je u tijeku i nema dostupnog prosirenja.

ZAUSTAVLJANJE:
--------------
Simulacija se automatski zaustavlja nakon 20 sekundi (200 perioda) ili preko Ctrl+C.

STATISTIKA:
-----------
Za svaki ulaz:
- Broj promjena stanja
- Prosjecno vrijeme reakcije
- Maksimalno vrijeme reakcije
- Broj neobradjenih dogadaja

Za zadatke:
- Broj zavrsenih zadataka
- Broj prekinutih zadataka
- Broj zadataka koji su koristili dva perioda

TRAJANJE:
---------
Simulacija traje 20 sekundi (200 perioda) ili do prekida Ctrl+C
