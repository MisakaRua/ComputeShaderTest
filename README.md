# ComputeShaderTest

This is a project i used to learn how to use opengl compute shader. Most codes are copied from the book OpenGL Programming Guide and the welsite LearnOpenGL. I also use some open source project, including GLFW, GLAD and GLM. I don't give the license of these libraries, which is not a good point.

If you want to run this project, you need Visual Studio 2022 because i haven't test on the other version of vs. You can just copy these codes to your project, and my project's structure is as following.

ComputeShaderTest  
  |--external  
  |    |--include  
  |    |    |--glad  
  |    |    |--GLFW  
  |    |    |--glm  
  |    |    |--KHR  
  |    |  
  |    |--lib  
  |    |    |--glfw3.lib  
  |    |  
  |    |--src  
  |         |--glad.c  
  |  
  |--shader  
  |    |--particle.comp  
  |  
  |--src  
       |--main.cpp  
