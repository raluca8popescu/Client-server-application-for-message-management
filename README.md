
TCP and UDP client-server application for message management

tcp_client:
    Am pornit un socket pentru clientul tcp. Prin comanda connect, se face
legatura cu serverul.
    Parcurgand file descriptorii, se realizeaza:
    * citire de la tastatura
        - exit: iese din while-ul de parcurgere si inchide socketul clientului
        - subscribe si unsubscribe: creeaza mesajul corespunzator si il trimite catre server
    * primire mesaje
        - exit: serverul s-a inchis asa ca se inchide si socketul clientului
        - mesaj primit de la server cu continutul mesajului de la clientii udp
    
server:
    Am pornit 2 socketi, unul tcp, altul udp. Am adaugat la file descriptori ambii
socketi. Se pot receptiona:
    - cereri de conectare de la clientii tcp: Se accepta conexiunea si se primeste
id-ul ca mesaj de la acestia.
        * daca este un client nou, se creeaza o structura noua de Subscriber si se adauga
socketul nou atat in structurile cu socketi cat si in lista file descriptorilor.
        * daca este un client care se reconecteaza, se trimit mesajele udp de la topicurile
cu sf 1 care nu au fost inca receptionate.
        * daca este un client nou cu un id la fel ca al altui client conectat, nu se accepta.
    - mesaje de la clientii udp: Se receptioneaza mesajul si se creeaza formatul nou
de mesaj ce va fi trimis clientilor tcp abonati.
    - mesaje de la clientii tcp: Se da subscribe sau unsubscribe la un topic.
    - clientii tcp au inchis conexiunea: Se adauga la clienti inactivi si se elimina 
socketul atat din structurile care il contin cat si din lista file descriptorilor.
    - mesaj de la tastatura cu "exit": se inchid socketii udp si tcp.
