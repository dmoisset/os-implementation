GIT Readme (Versión 0.1 - Edgardo Hames)

- Instalar git. En *Ubuntu:

$ sudo aptitude install git-core 

- Configurar git con los datos del usuario

$ git config --global user.name "tu nombre"
$ git config --global user.email tu-direccion@de.email

- En github, hacer un fork del proyecto dmoisset/os-implementation

- Clonar el repositorio de github

$ git clone <la dirección que da github>

Ahora ya tenemos una copia del repositorio de github en nuestra máquina
y podemos hacerlo evolucionar con nuestros propios cambios.

- Crear una rama para trabajar en el proyecto y empezar a usarla

$ git checkout -b 'rama-proyecto-0'

Ahora podemos editar, compilar, y correr los tests en la rama nueva. Cuando
hayamos terminado nuestra tarea, podemos revisar qué cambió con el comando
status.

$ git status

Y se nos presentará una lista de archivos y directorios que presentan cambios.
También nos indicará cómo hacemos para confirmar los cambios (por ejemplo,
hacer add de los archivos nuevos o modificados, y rm de los archivos
eliminados). Luego, podremos hacer commit de los cambios con un mensaje que
los describa:

$ git commit -m 'primeros cambios terminados'

Cuando terminemos con los cambios de esta rama, podremos integrarlos a la rama
master:

$ git checkout master
$ git merge rama-proyecto-0

Y enviarlos al repositorio de github:

$ git push -u origin/master


Referencias
-----------

Git para usuarios de SVN
http://git.or.cz/course/svn.html

Libros de git
http://progit.org/book/
http://book.git-scm.com/

Una presentación que muestra cómo funciona git internamente.
http://git-plumbing-preso.heroku.com/

"Git is the next Unix" o una "Oda a git"
http://www.advogato.org/person/apenwarr/diary/371.html
