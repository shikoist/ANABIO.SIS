@echo on
chcp 65001
setlocal EnableDelayedExpansion

echo ================================================
echo    Генерация шрифтового атласа для Glide
echo    с помощью ffmpeg (EXE должны быть в PATH)
echo    (все символы 32-126, пропущенные = пробел)
echo    (я так и не смог победить символы процента
echo             и одинарной кавычки)
echo ================================================

:: ================== НАСТРОЙКИ ==================
set FONT='C\:/Windows/Fonts/PublicPixel.ttf'
set FONTSIZE=16
set ATLAS_W=256
set ATLAS_H=256
set OUTPUT=font.tga

set TEXTCOLOR=white
set BORDERCOLOR=black
set BORDERW=0

set CHAR_W=16
set CHAR_H=16

set chars0= 
set "chars1=\^!"
set chars2=\"
set chars3=^#
set chars4=^$
set chars5=\%
set chars6=^&
set chars7=\'
set chars8=(
set chars9=)
set chars10=^*
set chars11=^+
set chars12=','
set chars13=^-
set chars14=^.
set chars15=^/
set chars16=0
set chars17=1
set chars18=2
set chars19=3
set chars20=4
set chars21=5
set chars22=6
set chars23=7
set chars24=8
set chars25=9
set chars26='\:'
set chars27='\;'
set chars28=^<
set chars29=^=
set chars30=^>
set chars31=^?
set chars32=@
set chars33=A
set chars34=B
set chars35=C
set chars36=D
set chars37=E
set chars38=F
set chars39=G
set chars40=H
set chars41=I
set chars42=J
set chars43=K
set chars44=L
set chars45=M
set chars46=N
set chars47=O
set chars48=P
set chars49=Q
set chars50=R
set chars51=S
set chars52=T
set chars53=U
set chars54=V
set chars55=W
set chars56=X
set chars57=Y
set chars58=Z
set chars59=\[
set chars60=^\\
set chars61=\]
set chars62=^^
set chars63=_
set chars64=`
set chars65=a
set chars66=b
set chars67=c
set chars68=d
set chars69=e
set chars70=f
set chars71=g
set chars72=h
set chars73=i
set chars74=j
set chars75=k
set chars76=l
set chars77=m
set chars78=n
set chars79=o
set chars80=p
set chars81=q
set chars82=r
set chars83=s
set chars84=t
set chars85=u
set chars86=v
set chars87=w
set chars88=x
set chars89=y
set chars90=z
set chars91={
set chars92='\^|'
set chars93=}
set chars94=~

:: ===============================================

echo Создаём пустой атлас %ATLAS_W%x%ATLAS_H%...
ffmpeg -y -f lavfi -i "color=black@0:s=%ATLAS_W%x%ATLAS_H%" -pix_fmt bgra -compression_level 0 -frames:v 1 -c:v targa -update 1 "%OUTPUT%"

echo Наносим символы (32-128)...

set idx=0
for /L %%i in (0,1,95) do (
	
    :: Обращаемся к переменной chars000, chars001 и т.д.
    set char=!chars%%i!
	
    set /a "col=!idx! %% 16"
    set /a "row=!idx! / 16"
    set /a "x=!col! * %CHAR_W%"
    set /a "y=!row! * %CHAR_H%"

    echo [!idx!] ASCII !char! x=!x! y=!y!

    ffmpeg -i "%OUTPUT%" -y -f lavfi -i "color=black@0:s=%ATLAS_W%x%ATLAS_H%" -vf "drawtext=fontfile=%FONT%:fontsize=%FONTSIZE%:fontcolor=%TEXTCOLOR%:borderw=%BORDERW%:bordercolor=%BORDERCOLOR%:x=!x!:y=!y!:y_align=font:text_align=C:text=!char!" -pix_fmt bgra -compression_level 0 -frames:v 1 -c:v targa -q:v 9 -update 1 "%OUTPUT%"

    set /a idx+=1
	rem pause
)
endlocal
