chcp 65001
C:
CD C:\Users\sergey\DOS\PROJECTS\ANABIO.SIS
COPY *.* C:\Users\sergey\Documents\GitHub\ANABIO.SIS
CD C:\Users\sergey\Documents\GitHub\ANABIO.SIS

set /p msg="Commit name: "

git add .
git commit -m "%msg%"
git push
REM PAUSE