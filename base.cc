#include <gtkmm.h>

class MyWindow : public Gtk::Window
{
public:
  MyWindow();
};

MyWindow::MyWindow()
{
  set_title("Basic application");
  set_default_size(200, 200);
}

int main(int argc, char* argv[])
{
  auto app = Gtk::Application::create("org.gtkmm.examples.base");
  Gtk::Window window;
  window.set_default_size(200, 200);
  return app->run(window, argc, argv);
}
