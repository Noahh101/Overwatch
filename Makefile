CXX = g++
CXXFLAGS = -Wall -g -I./imgui
LDFLAGS = -lglfw3 -lopengl32 -lgdi32 -mwindows -static-libgcc -static-libstdc++

SOURCES = main.cpp \
          imgui/imgui.cpp imgui/imgui_demo.cpp imgui/imgui_draw.cpp \
          imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
          imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp

overwatch.exe: $(SOURCES)
	$(CXX) $(CXXFLAGS) -o overwatch.exe $(SOURCES) $(LDFLAGS)

clean:
	rm -f overwatch.exe *.o

run: overwatch.exe
	.\overwatch.exe