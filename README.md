# Golf 2D
Its a simple golf game created for Lesta Games c++ developer courses
```
You can build it for 3 platforms:Linux(Fedora),Windows,Android(10 and higher)
## Build
### Linux
sudo dnf install sdl3-devel sdl3_ttf glm
```
mkdir build
```
cd build
```
cmake ..
```
make
```
cd ..
```
cp * ./res build/res
```
After you can run executable file at build folder
### Android
Open android-project directory in Android Studio
```
Add pathes to SDL and glm at local properties file
```
Run build
### Windiws
Download SDL3 SDL3_TTF GLM for your OS in standart cmake find path
```
mkdir build
```
cd build
```
cmake ..
```
make
```
cd ..
```
copy * ./res build/res
```
After you can run executable file at build folder
