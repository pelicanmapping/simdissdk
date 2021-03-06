/* -*- mode: c++ -*- */
/****************************************************************************
 *****                                                                  *****
 *****                   Classification: UNCLASSIFIED                   *****
 *****                    Classified By:                                *****
 *****                    Declassify On:                                *****
 *****                                                                  *****
 ****************************************************************************
 *
 *
 * Developed by: Naval Research Laboratory, Tactical Electronic Warfare Div.
 *               EW Modeling & Simulation, Code 5773
 *               4555 Overlook Ave.
 *               Washington, D.C. 20375-5339
 *
 * License for source code can be found at:
 * https://github.com/USNavalResearchLaboratory/simdissdk/blob/master/LICENSE.txt
 *
 * The U.S. Government retains all rights to use, duplicate, distribute,
 * disclose, or release this software.
 *
 */
#include "GL/glew.h"
#include <osg/Camera>
#include <osg/RenderInfo>
#include "imgui_impl_opengl3.h"
#include "BaseGui.h"
#include "OsgImGuiHandler.h"
#include "simNotify/Notify.h"
#include "simVis/Registry.h"

namespace GUI {

GlewInitOperation::GlewInitOperation()
  : osg::Operation("GlewInitCallback", false)
{
}

void GlewInitOperation::operator()(osg::Object* object)
{
  osg::GraphicsContext* context = dynamic_cast<osg::GraphicsContext*>(object);
  if (!context)
    return;

  if (glewInit() != GLEW_OK)
  {
    SIM_ERROR << "glewInit() failed" << std::endl;
  }
}

///////////////////////////////////////////////////////////////////////////////

struct OsgImGuiHandler::ImGuiNewFrameCallback : public osg::Camera::DrawCallback
{
  explicit ImGuiNewFrameCallback(OsgImGuiHandler& handler)
    : handler_(handler)
  {
  }

  void operator()(osg::RenderInfo& renderInfo) const override
  {
    handler_.newFrame_(renderInfo);
  }

private:
  OsgImGuiHandler& handler_;
};

///////////////////////////////////////////////////////////////////////////////

struct OsgImGuiHandler::ImGuiRenderCallback : public osg::Camera::DrawCallback
{
  explicit ImGuiRenderCallback(OsgImGuiHandler& handler)
    : handler_(handler)
  {
  }

  void operator()(osg::RenderInfo& renderInfo) const override
  {
    handler_.render_(renderInfo);
  }

private:
  OsgImGuiHandler& handler_;
};

///////////////////////////////////////////////////////////////////////////////

OsgImGuiHandler::OsgImGuiHandler()
  : time_(0.0f),
  mousePressed_{false},
  mouseWheel_(0.0f),
  initialized_(false)
{
}

void OsgImGuiHandler::add(BaseGui* gui)
{
  if (gui != nullptr)
    guis_.push_back(std::unique_ptr<BaseGui>(gui));
}

/**
 * Imporant Note: Dear ImGui expects the control Keys indices not to be
 * greater thant 511. It actually uses an array of 512 elements. However,
 * OSG has indices greater than that. So here I do a conversion for special
 * keys between ImGui and OSG.
 */
static int ConvertFromOSGKey(int key)
{
  using KEY = osgGA::GUIEventAdapter::KeySymbol;

  switch (key)
  {
    case KEY::KEY_Tab:
        return ImGuiKey_Tab;
    case KEY::KEY_Left:
        return ImGuiKey_LeftArrow;
    case KEY::KEY_Right:
        return ImGuiKey_RightArrow;
    case KEY::KEY_Up:
        return ImGuiKey_UpArrow;
    case KEY::KEY_Down:
        return ImGuiKey_DownArrow;
    case KEY::KEY_Page_Up:
        return ImGuiKey_PageUp;
    case KEY::KEY_Page_Down:
        return ImGuiKey_PageDown;
    case KEY::KEY_Home:
        return ImGuiKey_Home;
    case KEY::KEY_End:
        return ImGuiKey_End;
    case KEY::KEY_Delete:
        return ImGuiKey_Delete;
    case KEY::KEY_BackSpace:
        return ImGuiKey_Backspace;
    case KEY::KEY_Return:
        return ImGuiKey_Enter;
    case KEY::KEY_Escape:
        return ImGuiKey_Escape;
    case 22:
        return osgGA::GUIEventAdapter::KeySymbol::KEY_V;
    case 3:
        return osgGA::GUIEventAdapter::KeySymbol::KEY_C;
    default: // Not found
        return key;
  }
}

void OsgImGuiHandler::init()
{
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
  io.KeyMap[ImGuiKey_Tab] = ImGuiKey_Tab;
  io.KeyMap[ImGuiKey_LeftArrow] = ImGuiKey_LeftArrow;
  io.KeyMap[ImGuiKey_RightArrow] = ImGuiKey_RightArrow;
  io.KeyMap[ImGuiKey_UpArrow] = ImGuiKey_UpArrow;
  io.KeyMap[ImGuiKey_DownArrow] = ImGuiKey_DownArrow;
  io.KeyMap[ImGuiKey_PageUp] = ImGuiKey_PageUp;
  io.KeyMap[ImGuiKey_PageDown] = ImGuiKey_PageDown;
  io.KeyMap[ImGuiKey_Home] = ImGuiKey_Home;
  io.KeyMap[ImGuiKey_End] = ImGuiKey_End;
  io.KeyMap[ImGuiKey_Delete] = ImGuiKey_Delete;
  io.KeyMap[ImGuiKey_Backspace] = ImGuiKey_Backspace;
  io.KeyMap[ImGuiKey_Enter] = ImGuiKey_Enter;
  io.KeyMap[ImGuiKey_Escape] = ImGuiKey_Escape;
  io.KeyMap[ImGuiKey_A] = osgGA::GUIEventAdapter::KeySymbol::KEY_A;
  io.KeyMap[ImGuiKey_C] = osgGA::GUIEventAdapter::KeySymbol::KEY_C;
  io.KeyMap[ImGuiKey_V] = osgGA::GUIEventAdapter::KeySymbol::KEY_V;
  io.KeyMap[ImGuiKey_X] = osgGA::GUIEventAdapter::KeySymbol::KEY_X;
  io.KeyMap[ImGuiKey_Y] = osgGA::GUIEventAdapter::KeySymbol::KEY_Y;
  io.KeyMap[ImGuiKey_Z] = osgGA::GUIEventAdapter::KeySymbol::KEY_Z;

  ImGui_ImplOpenGL3_Init();

  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
}

void OsgImGuiHandler::setCameraCallbacks_(osg::Camera* camera)
{
  // potential gotcha, need to be chained with pre-existing callbacks
  camera->setPreDrawCallback(new ImGuiNewFrameCallback(*this));
  camera->setPostDrawCallback(new ImGuiRenderCallback(*this));
}

void OsgImGuiHandler::newFrame_(osg::RenderInfo& renderInfo)
{
  ImGui_ImplOpenGL3_NewFrame();

  ImGuiIO& io = ImGui::GetIO();

  io.DisplaySize = ImVec2(renderInfo.getCurrentCamera()->getGraphicsContext()->getTraits()->width, renderInfo.getCurrentCamera()->getGraphicsContext()->getTraits()->height);

  double currentTime = renderInfo.getView()->getFrameStamp()->getSimulationTime();
  io.DeltaTime = currentTime - time_ + 0.0000001;
  time_ = currentTime;

  for (int i = 0; i < 3; i++)
  {
    io.MouseDown[i] = mousePressed_[i];
  }

  for (int i = 0; i < 3; i++)
  {
    io.MouseDoubleClicked[i] = mouseDoubleClicked_[i];
  }

  io.MouseWheel = mouseWheel_;
  mouseWheel_ = 0.0f;

  ImGui::NewFrame();
}

void OsgImGuiHandler::render_(osg::RenderInfo& ri)
{
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoDockingInCentralNode | ImGuiDockNodeFlags_PassthruCentralNode;

  auto dockSpaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockspace_flags);

  draw_(ri);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  auto centralNode = ImGui::DockBuilderGetCentralNode(dockSpaceId);

  auto io = ImGui::GetIO();

  auto camera = ri.getCurrentCamera();
  auto viewport = camera->getViewport();
  viewport->x() = centralNode->Pos.x;
  viewport->y() = io.DisplaySize.y - centralNode->Size.y - centralNode->Pos.y;
  viewport->width() = centralNode->Size.x;
  viewport->height() = centralNode->Size.y;

  const osg::Matrixd& proj = camera->getProjectionMatrix();
  bool isOrtho = osg::equivalent(proj(3, 3), 1.0);
  if (!isOrtho)
  {
    double fovy, ar, znear, zfar;
    camera->getProjectionMatrixAsPerspective(fovy, ar, znear, zfar);
    camera->setProjectionMatrixAsPerspective(fovy, viewport->width() / viewport->height(), znear, zfar);
  }
  else
  {
    double left, right, bottom, top, znear, zfar;
    camera->getProjectionMatrixAsOrtho(left, right, bottom, top, znear, zfar);
    camera->setProjectionMatrixAsOrtho(viewport->x(), viewport->x() + viewport->width(), viewport->y(), viewport->y() + viewport->height(), znear, zfar);
  }
}

bool OsgImGuiHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
  if (!initialized_)
  {
    auto view = aa.asView();
    if (view)
    {
      setCameraCallbacks_(view->getCamera());
      initialized_ = true;
    }
  }

  ImGuiIO& io = ImGui::GetIO();
  const bool wantCaptureMouse = io.WantCaptureMouse;
  const bool wantCaptureKeyboard = io.WantCaptureKeyboard;

  switch (ea.getEventType())
  {
  case osgGA::GUIEventAdapter::KEYDOWN:
  case osgGA::GUIEventAdapter::KEYUP:
  {
    const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
    const int c = ea.getKey();

    // Always update the mod key status.
    io.KeyCtrl = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL;
    io.KeyShift = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT;
    io.KeyAlt = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT;
    io.KeySuper = ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER;

    const int imgui_key = ConvertFromOSGKey(c);
    if (imgui_key > 0 && imgui_key < 512)
    {
      //assert((imgui_key >= 0 && imgui_key < 512) && "ImGui KeysMap is an array of 512");
      io.KeysDown[imgui_key] = isKeyDown;
    }

    // Not sure this < 512 is correct here....
    if (isKeyDown && imgui_key >= 32 && imgui_key < 512)
    {
      io.AddInputCharacter(static_cast<unsigned int>(c));
    }

    return wantCaptureKeyboard;
  }
  case osgGA::GUIEventAdapter::RELEASE:
  {
    io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
    mousePressed_[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    mousePressed_[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
    mousePressed_[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;

    mouseDoubleClicked_[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    mouseDoubleClicked_[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
    mouseDoubleClicked_[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
    return wantCaptureMouse;
  }
  case osgGA::GUIEventAdapter::PUSH:
  {
    io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
    mousePressed_[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    mousePressed_[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
    mousePressed_[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
    return wantCaptureMouse;
  }
  case osgGA::GUIEventAdapter::DOUBLECLICK:
  {
    io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
    mouseDoubleClicked_[0] = ea.getButtonMask() & osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON;
    mouseDoubleClicked_[1] = ea.getButtonMask() & osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON;
    mouseDoubleClicked_[2] = ea.getButtonMask() & osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON;
    return wantCaptureMouse;
  }
  case osgGA::GUIEventAdapter::DRAG:
  case osgGA::GUIEventAdapter::MOVE:
  {
    io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
    return wantCaptureMouse;
  }
  case osgGA::GUIEventAdapter::SCROLL:
  {
    mouseWheel_ = ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 1.0 : -1.0;
    return wantCaptureMouse;
  }
  default:
    break;
  }

  return false;
}

void OsgImGuiHandler::draw_(osg::RenderInfo& ri)
{
  for (auto& gui : guis_)
    gui->draw(ri);
}

}
