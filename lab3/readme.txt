LAB3. Upravljanje sustavom korištenjem višedretvenosti

KOMPAJLIRANJE

gcc -pthread lab3.c -o lab3

POKRETANJE

./lab3

OPIS

Program simulira višedretveni sustav za upravljanje gdje svaki ulaz ima zasebnu upravljačku dretvu. Implementirano je 10 ulaza s različitim periodama: 5 ulaza s periodom 1000ms, 3 ulaza s periodom 2000ms i 2 ulaza s periodom 5000ms. Svaka upravljačka dretva čeka početak svoje periode pomoću sleep funkcije, provjerava je li ulaz promijenio stanje i ako jest, obrađuje zadatak korištenjem radnog čekanja.

Radno čekanje je implementirano prema uputama iz vježbe. Na početku programa izvršava se kalibracija koja određuje koliko iteracija prazne petlje je potrebno za potrošnju 10ms procesorskog vremena. Kalibracija koristi asm volatile("" ::: "memory") da spriječi kompajler da optimizira petlju. Simulacija obrade zadatka zatim poziva kalibriranu funkciju odgovarajući broj puta ovisno o trajanju obrade C.

Prioriteti dretvi dodijeljeni su prema mjeri ponavljanja kako nalaže Rate Monotonic raspoređivanje. Dretve s kraćim periodama imaju veći prioritet: T=1000ms dobiva prioritet 60, T=2000ms prioritet 50, a T=5000ms prioritet 40. Dretve koje simuliraju ulaze imaju najviši prioritet 70 i koriste sleep unutar petlje umjesto radnog čekanja.

Na Linuxu program koristi SCHED_FIFO raspoređivanje putem pthread_setschedparam funkcije što zahtijeva administratorske ovlasti (sudo ./lab3). Na macOS-u SCHED_FIFO nije dostupan na isti način pa program koristi Mach thread API. Konkretno, koristi se thread_policy_set s THREAD_PRECEDENCE_POLICY za postavljanje relativnog prioriteta dretvi. Veća vrijednost importance parametra znači veći prioritet. Ovaj mehanizam opisan je u Apple dokumentaciji: https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/About/About.html#//apple_ref/doc/uid/TP30000905-CH204-TPXREF102.

TESTIRANJE NA MACOS

Za testiranje granica sustava na macOS-u može se koristiti cpulimit alat koji ograničava postotak CPU vremena dostupnog procesu. Instalacija se vrši pomoću brew install cpulimit.

Pokretanje s ograničenim CPU resursima:

cpulimit -l 30 ./lab3

Parametar -l određuje postotak CPU vremena. Vrijednost 30 znači 30% dostupnog CPU vremena. Za ekstremnije testiranje može se koristiti cpulimit -l 1 što ograničava proces na samo 1% CPU vremena.

Dodatno se mogu povećati vrijednosti trajanja obrade C u polju inputs unutar koda. Veće vrijednosti C znače dulje radno čekanje po zadatku što povećava ukupnu iskorištenost procesora. Kada ukupna iskorištenost premaši granicu rasporedivosti sustava, počet će se javljati neobrađeni događaji i povećana vremena reakcije.

Primjer konfiguracije za preopterećeni sustav:

{1000, 0, 150, 1} umjesto {1000, 0, 50, 1}

STATISTIKA

Program na kraju ispisuje statistiku za svaki ulaz koja uključuje broj promjena stanja, prosječno vrijeme reakcije na promjenu stanja, maksimalno vrijeme reakcije i broj neobrađenih događaja. Također se ispisuje ukupna statistika za cijeli sustav.

TRAJANJE

Simulacija traje 20 sekundi ili do prekida signalom SIGINT (Ctrl+C).