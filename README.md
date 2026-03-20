## Hi everyone

I wrote some code to build (im)possible n-agon shapes. You can also rotate it with the cursor. There are parts that are open to improve and debug. But I still wanted to share this with you since i think we can use this logic in our project. Main.cpp is deprecated and is no longer of use but I did not want to delete it, (I am scared of merge conflicts) so you can just ignore it. Also the math library is custom. I mean i stole every function from another library but still. Probably using GLM was a better idea but i did not know this back then. The InitShader file is also stolen from the instructors util files.

## Setup & Build Instructions

### Requirements

1. Install dependencies

* cmake
* GLFW
* OpenGL
* GLEW (required if you are not using macOS)

---

2. VS Code Extensions

Make sure you have the following extensions installed in VS Code:

* C/C++
* CMake Tools
* CMake (optional)

---

### Build Instructions

Option 1 — Using VS Code

1. Open the project folder in VS Code
2. When prompted, select a compiler (GCC or Clang++)
3. You should see Build and Run buttons in the bottom bar

---

Option 2 — Manual Build

If VS Code doesn’t configure automatically, you can build manually:

mkdir build
cd build
cmake ..
make

---

The project should (hopefully) compile and run after these steps. Even if it does not compile, I added some example photos just in case. They are in the examples folder.
