Для проекта необходимо чтобы были установлены нижеуказанные продукты по
соответствующим путям

C:\ArmGNU
C:\CMake
C:\Ninja
C:\JLink

и была правильно настроена переменная PATH

Это шаблон проекта.
Каталоги cmsis, startup, svd, cmake, linker - одинаковые для всех проектов
Файл .gitignore - одинаковый для всех проектов

В файле .\tools\jlink_flash_sam3x8e.jlink изменить путь для билдов

 В файлах lanch.json, settings.json и task.json в выражение "${workspaceFolder:empty_project}" необходимо
 будет заменить empty_project на название проекта

 Весь изменяемый код находится в каталогах params и src
 

