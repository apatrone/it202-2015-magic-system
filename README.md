Pour compiler tous les fichiers de test avec notre librairie de threads, il suffit d'executer la commande "make". Le binaires seront situés dans le dossier bin/ourlib.


Le makefile possède en plus d'autres cibles permettant de tester facilement notre librairie.

"make help" affiche cette page d'aide (en anglais)
"make all" compile tous les fichiers de test avec notre librairie et avec les pthreads
"make with-ourlib" compile tous les fichiers de test avec notre librairie
"make with-pthread" compile tous les fichiers de test avec les pthreads
"make clean" supprime les fichiers binaires créés à la compilation
"make test-all" compile les fichiers de test avec notre librairie si besoin et effectue un test rapide (avec valgrind) de chaque binaire créé
"make test bin=$bin arg=$arg" effectue un test en profondeur (avec valgrind) sur le binaire passé en paramètre avec les arguments passés en paramètres (ne pas écrire "arg=" si le binaire ne nécéssite pas de paramètres). Example: 'make test bin=21-create-many arg=5'
