#define _USE_MATH_DEFINES

// [Libraries]

#include "GL/glew.h"								// Don't swap order between GL and GLFW includes!
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/epsilon.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imfiledialog/ImGuiFileDialog.h"
//#include "imnodes/imnodes.h"
#include "implot.h"

// [Image]

#include "lodepng.h"

// [Standard libraries: basic]

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <execution>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <thread>

// [Standard libraries: data structures]

#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// [Our own classes]

#include "Geometry/General/Adapter.h"
