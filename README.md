GLDemo
======

This is a demo program for my opengl and c++ learning for
rendering terrain

To run the demo, you currently need to solve related library
dependency yourself, and execute:

* git submodule update --init glm/
* g++ -I glm/ -o main main.cpp tools/*.cpp modules/*.cpp -lGL -ltiff -lGLEW -lglfw -std=c++0x -lpthread
* ./draw

**Preview of the rendeing for hawaii islands:**
![](https://raw.githubusercontent.com/plysan/GLDemo/master/demo.jpg)

