# DansTonChatConsole
Permet de lire les quotes du célèbre site "danstonchat.com" dans votre terminal.

Compilation:
------------
Si vous n'avez pas "libcurl" d'installer, entrez cette commande: ``sudo apt-get install libcurl3-dev``

Pour compiler:

``gcc dtcconsole.c $(pkg-config --libs --cflags libcurl) -Wall -Wextra -o dtcconsole`

Commandes:
----------

Il n'est pas obligatoire d'entrer un nombre.

    ./dtconsole -random (-r)[N]: permet de lire aléatoirement N quote(s)
    ./dtconsole -last (-l)[N]: permet de lire les N dernières quotes.
    ./dtconsole -quote (-q)[ID]: permet de lire la quote demandé.
