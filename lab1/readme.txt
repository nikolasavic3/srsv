LAB1. Upravljačka petlja
=====================================================

KOMPAJLIRANJE:
--------------
gcc -pthread lab1.c -o lab1

POKRETANJE:
-----------
./lab1

Program simulira kontrolnu petlju koja provjerava 38 ulaza s različitim periodama:
- 3 ulaza s periodom 1000ms (svake sekunde)
- 15 ulaza s periodom 5000ms (svakih 5 sekundi)
- 20 ulaza s periodom 20000ms (svakih 20 sekundi)

ZAUSTAVLJANJE:
--------------
Ctrl+C - završava simulaciju i ispisuje statistiku

STATISTIKA:
-----------
Za svaki ulaz:
- Broj promjena stanja
- Prosječno vrijeme reakcije
- Maksimalno vrijeme reakcije
- Broj neobrađenih događaja

TRAJANJE:
---------
Simulacija traje 20 sekundi ili do Ctrl+C