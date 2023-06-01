# Tutorial-Game
Following game development Tutorial in C\
for a win32 Application/game

Reference:
https://youtube.com/playlist?list=PLlaINRtydtNWuRfd4Ra3KeD6L9FP_tDE7 \
Last completed episode no.13

I drew a backbuffer of blue to the screen. The white box can be moved with WASD or Up/Down/Left/Right
![Screenshot (908)](https://github.com/GlennVodra/Tutorial-Game/assets/37476686/66949207-5a8f-4cbf-bb21-cd7060e0dc33)


* Bugs
CPU Usage is quite high as the program is just busy waiting till it hits 60 FPS. \
Using sleep(1); was slightly sucessful at lowering the usage, but halfs frames, even when frame times are being calculated and the system timer resolution is taken into account.
