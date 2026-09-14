@echo off

chcp 65001

echo Принудительное размонтирование...
"C:\Program Files\OSFMount\OSFMount.com" -D -m Z:

echo Монтирование образа...
"C:\Program Files\OSFMount\OSFMount.com" -a -t file -f "E:\Programs\86Box\hdd\projects.img" -m Z: -o rw

echo Копирование новых файлов...
xcopy "C:\Users\sergey\DOS\PROJECTS\ANABIO.SIS\*.*" "Z:\PROJECTS\ANABIO.SIS\" /F /E /H /R /Y

echo Размонтирование...
"C:\Program Files\OSFMount\OSFMount.com" -D -m Z:

echo Запуск 86Box...
start "" "E:\Programs\86Box\86Box.exe" --config "C:\Users\sergey\86Box VMs\win98\86box.cfg"