! Functionalitatea serverului este limitata !

Serverul contine :
- structura Subscriber care este folosita pentru a retine la ce topicuri se aboneaza un subscriber.

- functia "run_chat_multi_server" care se ocupa de rularea propriu-zisa a serverului. Aceasta seteaza poll 
"sockets" pentru citire de la tastatura, pentru TCP si UDP. In loop-ul while vad daca la socketi se produce unul dintre
cazurile de mai sus de "citire". In cazul de nou subscriber (tcp_listenfd), ma ocup de socketul acestuia, iar
apoi astept sa primesc de la el ID-ul. Urmeaza sa-l adaug in lista de subscriberi in cazul in care nu este deja
adaugat precedent si afisez ca s-a conectat. In cazul citirii de la tastatura (STDIN_FILENO) vad daca una din
comenzi este "exit". Daca da, inchid subscriberii si apoi serverul(socketii TCP si UDP). In ultimul caz ma ocup
de transmiterea de date dintre subscriber -> server.

- in functia main() incep prin a verifica daca am suficiente argumente. Urmeaza parsarea portului si creearea 
socketului TCP si set-upul necesar acestuia (comentarii in cod pentru detalii), iar apoi creearea celui UDP.
La final pornesc serverul prin apelarea functiei "run_chat_multi_server" si inchid socketurile.

Subscriber contine :

- functia "run_client" care se ocupa de rularea subscriberului. Setez poll "sockets" pentru citire de la tastatura
si primire de date. In interiorul while voi primi comenzi de abonare / dezabonare si voi printa in functie de
comanda. Am acoperit si cazul de citire de la server dar nu fac nimic.

- in functia main() incep prin a verifica daca am suficiente argumente. Urmeaza creearea socketului si set-upul
necesar acestuia, dupa care ma conectez la server si ii trimit ID-ul. La final inchid socketul creat.
