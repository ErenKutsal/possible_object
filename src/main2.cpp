#include "InitShader.h"
#include "my_math.h"

const int num_vertices = 36;
vec3 vertices[num_vertices];
vec3 colors[num_vertices];

GLuint vao, vbo;
GLuint program;
GLint modelViewProjLoc;

float camera_radius = 0.5f;
float camera_theta = M_PI / 2.0f;
float camera_phi = M_PI / 2.0f;

bool isDragging = false;
double lastMouseX = 0.0;
double lastMouseY = 0.0;

int Index = 0;

void make_quad(vec3 a, vec3 b, vec3 c, vec3 d, vec3 color)
{
  colors[Index] = color;
  vertices[Index++] = a;

  colors[Index] = color;
  vertices[Index++] = b;

  colors[Index] = color;
  vertices[Index++] = c;

  colors[Index] = color;
  vertices[Index++] = a;

  colors[Index] = color;
  vertices[Index++] = c;

  colors[Index] = color;
  vertices[Index++] = d;
}

void create_generic_mitered_segment(float radius, float thickness)
{
  float half_thick = thickness / 2.0f;

  // The exact math to make a 60-degree corner at the specific radius
  float half_len = radius * tanf(30.0f * (M_PI / 180.0f));
  float miter_x_offset = half_thick * tanf(30.0f * (M_PI / 180.0f));

  vec3 v[8];
  // Front Face
  v[0] = vec3(-half_len + miter_x_offset, -half_thick, half_thick);  // BL
  v[1] = vec3(-half_len - miter_x_offset, half_thick, half_thick);   // TL
  v[2] = vec3(half_len + miter_x_offset, half_thick, half_thick);    // TR
  v[3] = vec3(half_len - miter_x_offset, -half_thick, half_thick);   // BR

  // Back Face
  v[4] = vec3(-half_len + miter_x_offset, -half_thick, -half_thick);
  v[5] = vec3(-half_len - miter_x_offset, half_thick, -half_thick);
  v[6] = vec3(half_len + miter_x_offset, half_thick, -half_thick);
  v[7] = vec3(half_len - miter_x_offset, -half_thick, -half_thick);

  vec3 lightGray(0.9f, 0.9f, 0.9f);  // Top face
  vec3 medGray(0.5f, 0.5f, 0.5f);    // Outer face
  vec3 darkGray(0.2f, 0.2f, 0.2f);   // Inner face

  // Stitch the 6 faces cleanly
  make_quad(v[1], v[0], v[3], v[2], medGray);    // Front
  make_quad(v[2], v[3], v[7], v[6], medGray);    // Right
  make_quad(v[6], v[7], v[4], v[5], darkGray);   // Back
  make_quad(v[5], v[4], v[0], v[1], medGray);    // Left
  make_quad(v[1], v[2], v[6], v[5], lightGray);  // Top
  make_quad(v[0], v[4], v[7], v[3], darkGray);   // Bottom
}

void create_hexagon() {}

void init()
{
  // create_hexagon();
  create_generic_mitered_segment(0.5f, 0.15f);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);

  program = InitShader("../shaders/vshader.glsl", "../shaders/fshader.glsl");
  glUseProgram(program);

  GLuint loc = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

  GLuint colLoc = glGetAttribLocation(program, "vColor");
  glEnableVertexAttribArray(colLoc);
  glVertexAttribPointer(colLoc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)sizeof(vertices));

  modelViewProjLoc = glGetUniformLocation(program, "MVP");

  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // Dark gray background

  glEnable(GL_DEPTH_TEST);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float eye_x = camera_radius * sinf(camera_phi) * cosf(camera_theta);
  float eye_y = camera_radius * cosf(camera_phi);
  float eye_z = camera_radius * sinf(camera_phi) * sinf(camera_theta);

  vec3 eye(eye_x, eye_y, eye_z);
  vec3 at(0.0f, 0.0f, 0.0f);
  vec3 up(0.0f, 1.0f, 0.0f);
  mat4 view = LookAt(eye, at, up);

  mat4 proj = Ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

  mat4 viewProj = proj * view;

  float radius = 0.5f;
  float barLength = 0.5f;
  float thickness = 0.05f;
  float zStep = 0.15f;

  int drawOrder[6] = {1, 2, 3, 4, 5, 0};

  for (int i = 0; i < 6; i++)
  {
    int barIndex = drawOrder[i];

    // *** THE ILLUSION TRIGGER ***
    if (barIndex == 0)
    {
      // Right before drawing Bar 0, we surgically clear the depth buffer.
      // This guarantees Bar 0 draws on top of Bar 1 (creating the
      // loop-over effect), even though Bar 0 is physically further away
      // from the camera.
      glClear(GL_DEPTH_BUFFER_BIT);
    }

    // Standard flat spiral math: ONLY RotateZ and Translation. NO X/Y
    // rotation!
    float angle = barIndex * 60.0f;
    float zDepth = barIndex * zStep;

    // Model Matrix: Spin it, push it out to the radius, and shift depth
    mat4 model = RotateX(30) * RotateZ(angle) * Translate(0.0f, radius, zDepth);
    mat4 mvp = viewProj * model;

    // Send to shader and draw the flawless generic segment
    glUniformMatrix4fv(modelViewProjLoc, 1, GL_FALSE, &mvp.d[0].x);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
  }

  glFinish();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  switch (key)
  {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
      break;
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
    {
      isDragging = true;
      glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
    }
    else if (action == GLFW_RELEASE)
    {
      isDragging = false;
    }
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
  if (isDragging)
  {
    double deltaX = xpos - lastMouseX;
    double deltaY = ypos - lastMouseY;

    lastMouseX = xpos;
    lastMouseY = ypos;

    camera_theta -= deltaX * 0.01f;
    camera_phi += deltaY * 0.01f;

    // Clamp the up/down angle so the camera doesn't flip upside down
    if (camera_phi < 0.01f) camera_phi = 0.01f;
    if (camera_phi > M_PI - 0.01f) camera_phi = M_PI - 0.01f;
  }
}

void update(void) {}

int main()
{
  if (!glfwInit()) return 1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

  GLFWwindow* window = glfwCreateWindow(512, 512, "possible", NULL, NULL);
  glfwMakeContextCurrent(window);

  if (!window)
  {
    glfwTerminate();
    return 1;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  init();

  double frameRate = 30, currentTime, previousTime = 0.0;
  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();  // Handles queued events
    currentTime = glfwGetTime();
    if (currentTime - previousTime >= 1 / frameRate)
    {
      previousTime = currentTime;
      update();
    }

    display();
    glfwSwapBuffers(window);
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 1;
}
