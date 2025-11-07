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

------------
DODATNI ZADATCI
c
u lab1/lab1a je napravljena kopija lab1/lab1.c gdje na liniji 150 trajanje usleep funkcije duplo povešano sto uzrukuje dulju simulaciju procesa. To uzrokuje povećanje opterecenje zadataka i povecava broj prekasno obradjenih zadataka na 20-30%

d

u lab1/labb je napravljena kopija lab1/lab1.c sa izmjenama. Glavana izmjena je dodavanje dodatne dretve koja provjera je li doslo do vise od 3 neobradjenih dogadjaja. Ukoliko je doslo do pojave vise od 3 neobradjenih dogadjaja mjenja se uvjet za obradu za zadatka u upravljačkoj dretvi. Tek nakon sto se smanji broj neobrađenih događaja na manje od 3 upravljačka dretva nastavlja sa obradom po uobičjnom redosljedu. 
