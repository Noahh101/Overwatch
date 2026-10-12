CXX = g++
CXXFLAGS = -Wall -O1 -I./imgui
VENDOR_CXXFLAGS = -Wall -O0 -I./imgui
LDFLAGS = -lglfw3 -lopengl32 -lgdi32 -ldxgi -lpdh -lpowrprof -mwindows -static-libgcc -static-libstdc++
JOBS ?= 8
OUTPUT ?= overwatch.exe

SOURCES = main.cpp Telemetry.cpp \
          imgui/imgui.cpp imgui/imgui_draw.cpp \
          imgui/imgui_tables.cpp imgui/imgui_widgets.cpp \
          imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp \
          imgui/implot.cpp imgui/implot_items.cpp
OBJECTS = $(SOURCES:.cpp=.o)
DEPENDS = $(OBJECTS:.o=.d)

.PHONY: clean run

$(OUTPUT): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

imgui/%.o: imgui/%.cpp
	$(CXX) $(VENDOR_CXXFLAGS) -MMD -MP -c -o $@ $<

clean:
	rm -f overwatch.exe $(OBJECTS) $(DEPENDS)

run:
	$(MAKE) --jobs=$(JOBS) $(OUTPUT) && ./$(OUTPUT)

-include $(DEPENDS)