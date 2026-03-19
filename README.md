Merhaba Yerdal,

Bu proje senın bılgısayarda calısacak mı deneyebilir misin acaba lutfen?

Gerekenler:

1 - Gemini'ya gore asagıdaki komut ile glfw kutuphanesini indirmek gerek.
sudo apt update && sudo apt install -y build-essential cmake libglfw3-dev libgl1-mesa-dev

2 - VS Code'da su eklentileri indir:

i) C/C++ (Muhtemelen yukludur)
ii) CMake Tools 
iii) CMake (opsıyonel)

3 - Sanırım bu kadar.

Ardından sunları yapmalısın:

VS Code ile proje directorysini ac.
VS Code muhtemelen compiler soracak, GCC secebilirsin veya CLANG++.
Sol altta build ve run tuslarının cıkması gerek.

Cıkmazsa:

mkdir build
cd build
cmake ..
make

komutlarını yazarsan kodun compile olması gerek.

Tesekkurler

