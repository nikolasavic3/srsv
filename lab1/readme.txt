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
c)
U lab1/lab1a je napravljena kopija lab1/lab1.c. Na liniji 150 trajanje usleep funkcije je duplo povećano, pa je tako i "simulacija procesa" duplo dulja. Tako se postiže povećano opterećenje zadataka što povećava broj prekasno obrađenih zadataka s 10%-20% na 20-30%.

d)
U lab1/lab1b je napravljena kopija lab1/lab1.c s nekoliko izmjena. Glavna izmjena je dodavanje dodatne dretve koja provjera je li došlo do više od 3 neobrađenih događaja. Ukoliko je došlo do pojave preko 3 neobrađenih događaja mijenja se uvjet za obradu zadataka u upravljačkoj dretvi. Tek nakon sto se smanji broj neobrađenih događaja na manje od 3 upravljačka dretva nastavlja sa obradom po uobičjnom redosljedu. 
